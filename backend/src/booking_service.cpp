#include "booking_service.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <random>
#include <iostream>

namespace cinebook {

std::string BookingService::generateBookingReference() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif

    std::ostringstream ss;
    ss << "CB-" << std::put_time(&tm_buf, "%Y%m%d") << "-";

    // Append 4 pseudo-random hex digits
    static std::mt19937 rng(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<int> dist(0x1000, 0xFFFF);
    ss << std::hex << std::uppercase << dist(rng);

    return ss.str();
}

nlohmann::json BookingService::createBooking(
    const std::string& customerName,
    const std::string& customerEmail,
    const std::string& customerPhone,
    int showId,
    const std::vector<int>& seatIds,
    std::string& outError,
    int& outStatusCode
) {
    (void)customerPhone; // Recorded with user if needed

    // 1. Basic validation
    if (customerName.empty()) {
        outError = "Customer name is required.";
        outStatusCode = 400;
        return nullptr;
    }
    if (customerEmail.empty() || customerEmail.find('@') == std::string::npos) {
        outError = "A valid customer email is required.";
        outStatusCode = 400;
        return nullptr;
    }
    if (showId <= 0) {
        outError = "Invalid show selected.";
        outStatusCode = 400;
        return nullptr;
    }
    if (seatIds.empty()) {
        outError = "At least one seat must be selected.";
        outStatusCode = 400;
        return nullptr;
    }

    // 2. Start atomic transaction
    if (!db_.beginTransaction()) {
        outError = "Failed to begin database transaction.";
        outStatusCode = 500;
        return nullptr;
    }

    // 3. Resolve or insert user in USERS table
    int64_t userId = 0;
    auto userRows = db_.query("SELECT id FROM USERS WHERE email = ?;", {customerEmail});
    if (!userRows.empty()) {
        userId = std::stoll(userRows[0]["id"]);
    } else {
        std::string dummyHash = "guest_booking_auto_generated";
        userId = db_.executeInsert(
            "INSERT INTO USERS (username, email, password_hash, role) VALUES (?, ?, ?, 'user');",
            {customerName, customerEmail, dummyHash}
        );
        if (userId <= 0) {
            db_.rollbackTransaction();
            outError = "Failed to register customer record.";
            outStatusCode = 500;
            return nullptr;
        }
    }

    // 4. Validate show exists
    std::string showSql = 
        "SELECT s.id, s.movie_id, s.screen_id, s.show_date, s.show_time, "
        "m.title AS movie_title, m.poster_url, "
        "t.name AS theatre_name, t.city, sc.screen_name "
        "FROM SHOWS s "
        "JOIN MOVIES m ON s.movie_id = m.id "
        "JOIN SCREENS sc ON s.screen_id = sc.id "
        "JOIN THEATRES t ON sc.theatre_id = t.id "
        "WHERE s.id = ?;";

    auto showRows = db_.query(showSql, {std::to_string(showId)});
    if (showRows.empty()) {
        db_.rollbackTransaction();
        outError = "The selected movie show does not exist.";
        outStatusCode = 404;
        return nullptr;
    }
    const auto& showInfo = showRows[0];
    int screenId = std::stoi(showInfo.at("screen_id"));

    // 5. Atomic seat verification to prevent double-booking
    double totalAmount = 0.0;
    struct SeatDetail {
        int id;
        std::string number;
        std::string row;
        std::string category;
        double price;
    };
    std::vector<SeatDetail> verifiedSeats;

    for (int seatId : seatIds) {
        std::string seatCheckSql = 
            "SELECT st.id, st.seat_number, st.row_name, st.category, st.base_price, "
            "bs.id AS booked_id "
            "FROM SEATS st "
            "LEFT JOIN BOOKING_SEATS bs ON st.id = bs.seat_id AND bs.show_id = ? AND bs.status = 'BOOKED' "
            "WHERE st.id = ? AND st.screen_id = ?;";

        auto seatRows = db_.query(seatCheckSql, {std::to_string(showId), std::to_string(seatId), std::to_string(screenId)});
        if (seatRows.empty()) {
            db_.rollbackTransaction();
            outError = "Seat ID " + std::to_string(seatId) + " does not belong to this auditorium.";
            outStatusCode = 400;
            return nullptr;
        }

        const auto& sr = seatRows[0];
        if (!sr.at("booked_id").empty()) {
            // Seat is already booked! Concurrency conflict.
            db_.rollbackTransaction();
            outError = "Seat " + sr.at("seat_number") + " is already booked by another user for this show. Please select a different seat.";
            outStatusCode = 409; // Conflict
            return nullptr;
        }

        double price = std::stod(sr.at("base_price"));
        totalAmount += price;
        verifiedSeats.push_back({
            seatId, 
            sr.at("seat_number"), 
            sr.at("row_name"), 
            sr.at("category"), 
            price
        });
    }

    // 6. Generate unique booking reference and insert into BOOKINGS
    std::string bookingRef = generateBookingReference();
    std::string insertBookingSql = 
        "INSERT INTO BOOKINGS (booking_reference, user_id, show_id, total_amount, ticket_count, payment_status, booking_status) "
        "VALUES (?, ?, ?, ?, ?, 'SUCCESS', 'CONFIRMED');";

    int64_t bookingId = db_.executeInsert(insertBookingSql, {
        bookingRef,
        std::to_string(userId),
        std::to_string(showId),
        std::to_string(totalAmount),
        std::to_string(verifiedSeats.size())
    });

    if (bookingId <= 0) {
        db_.rollbackTransaction();
        outError = "Failed to create booking order.";
        outStatusCode = 500;
        return nullptr;
    }

    // 7. Insert each verified seat into BOOKING_SEATS
    for (const auto& seat : verifiedSeats) {
        std::string insertSeatSql = 
            "INSERT INTO BOOKING_SEATS (booking_id, show_id, seat_id, price_snapshot, status) "
            "VALUES (?, ?, ?, ?, 'BOOKED');";

        int64_t bSeatId = db_.executeInsert(insertSeatSql, {
            std::to_string(bookingId),
            std::to_string(showId),
            std::to_string(seat.id),
            std::to_string(seat.price)
        });

        if (bSeatId <= 0) {
            db_.rollbackTransaction();
            outError = "Failed to record seat reservation.";
            outStatusCode = 500;
            return nullptr;
        }
    }

    // 8. Commit the transaction
    if (!db_.commitTransaction()) {
        db_.rollbackTransaction();
        outError = "Failed to commit booking transaction.";
        outStatusCode = 500;
        return nullptr;
    }

    // 9. Build response payload
    nlohmann::json res;
    res["success"] = true;
    res["booking_reference"] = bookingRef;
    res["booking_id"] = bookingId;
    res["movie_title"] = showInfo.at("movie_title");
    res["poster_url"] = showInfo.at("poster_url");
    res["theatre_name"] = showInfo.at("theatre_name");
    res["city"] = showInfo.at("city");
    res["screen_name"] = showInfo.at("screen_name");
    res["show_date"] = showInfo.at("show_date");
    res["show_time"] = showInfo.at("show_time");
    res["ticket_count"] = verifiedSeats.size();
    res["total_amount"] = totalAmount;
    res["payment_status"] = "SUCCESS";
    res["booking_status"] = "CONFIRMED";
    res["customer_name"] = customerName;
    res["customer_email"] = customerEmail;

    nlohmann::json seatsJson = nlohmann::json::array();
    for (const auto& seat : verifiedSeats) {
        seatsJson.push_back({
            {"seat_id", seat.id},
            {"seat_number", seat.number},
            {"category", seat.category},
            {"price", seat.price}
        });
    }
    res["seats"] = seatsJson;

    outStatusCode = 201;
    return res;
}

nlohmann::json BookingService::getBookingByReference(
    const std::string& bookingRef, 
    const std::string& email
) {
    std::string sql = 
        "SELECT b.id AS booking_id, b.booking_reference, b.total_amount, b.ticket_count, "
        "b.payment_status, b.booking_status, b.created_at, "
        "u.username AS customer_name, u.email AS customer_email, "
        "s.id AS show_id, s.show_date, s.show_time, "
        "m.id AS movie_id, m.title AS movie_title, m.poster_url, m.duration_mins, m.certificate, "
        "t.id AS theatre_id, t.name AS theatre_name, t.city, "
        "sc.id AS screen_id, sc.screen_name "
        "FROM BOOKINGS b "
        "JOIN USERS u ON b.user_id = u.id "
        "JOIN SHOWS s ON b.show_id = s.id "
        "JOIN MOVIES m ON s.movie_id = m.id "
        "JOIN SCREENS sc ON s.screen_id = sc.id "
        "JOIN THEATRES t ON sc.theatre_id = t.id "
        "WHERE b.booking_reference = ? ";

    std::vector<std::string> params = {bookingRef};
    if (!email.empty()) {
        sql += " AND LOWER(u.email) = ? ";
        std::string lowEmail = "";
        for (char c : email) lowEmail += static_cast<char>(tolower(c));
        params.push_back(lowEmail);
    }

    auto rows = db_.query(sql, params);
    if (rows.empty()) {
        return nullptr;
    }

    const auto& r = rows[0];
    nlohmann::json obj;
    obj["booking_id"] = std::stoi(r.at("booking_id"));
    obj["booking_reference"] = r.at("booking_reference");
    obj["customer_name"] = r.at("customer_name");
    obj["customer_email"] = r.at("customer_email");
    obj["movie_id"] = std::stoi(r.at("movie_id"));
    obj["movie_title"] = r.at("movie_title");
    obj["poster_url"] = r.at("poster_url");
    obj["duration_mins"] = std::stoi(r.at("duration_mins"));
    obj["certificate"] = r.at("certificate");
    obj["theatre_name"] = r.at("theatre_name");
    obj["theatre_city"] = r.at("city");
    obj["screen_name"] = r.at("screen_name");
    obj["show_date"] = r.at("show_date");
    obj["show_time"] = r.at("show_time");
    obj["total_amount"] = std::stod(r.at("total_amount"));
    obj["ticket_count"] = std::stoi(r.at("ticket_count"));
    obj["payment_status"] = r.at("payment_status");
    obj["booking_status"] = r.at("booking_status");
    obj["created_at"] = r.at("created_at");

    // Fetch seats
    std::string seatSql = 
        "SELECT bs.id, bs.seat_id, bs.price_snapshot, bs.status, "
        "st.seat_number, st.row_name, st.category "
        "FROM BOOKING_SEATS bs "
        "JOIN SEATS st ON bs.seat_id = st.id "
        "WHERE bs.booking_id = ? "
        "ORDER BY st.row_name ASC, st.seat_number ASC;";

    auto seatRows = db_.query(seatSql, {r.at("booking_id")});
    nlohmann::json seatsJson = nlohmann::json::array();
    for (const auto& sr : seatRows) {
        seatsJson.push_back({
            {"seat_id", std::stoi(sr.at("seat_id"))},
            {"seat_number", sr.at("seat_number")},
            {"category", sr.at("category")},
            {"price", std::stod(sr.at("price_snapshot"))},
            {"status", sr.at("status")}
        });
    }
    obj["seats"] = seatsJson;

    return obj;
}

nlohmann::json BookingService::getUserBookings(const std::string& email) {
    std::string sql = 
        "SELECT b.id AS booking_id, b.booking_reference, b.total_amount, b.ticket_count, "
        "b.payment_status, b.booking_status, b.created_at, "
        "u.username AS customer_name, u.email AS customer_email, "
        "s.show_date, s.show_time, "
        "m.title AS movie_title, m.poster_url, "
        "t.name AS theatre_name, t.city, sc.screen_name "
        "FROM BOOKINGS b "
        "JOIN USERS u ON b.user_id = u.id "
        "JOIN SHOWS s ON b.show_id = s.id "
        "JOIN MOVIES m ON s.movie_id = m.id "
        "JOIN SCREENS sc ON s.screen_id = sc.id "
        "JOIN THEATRES t ON sc.theatre_id = t.id "
        "WHERE LOWER(u.email) = ? "
        "ORDER BY b.id DESC;";

    std::string lowEmail = "";
    for (char c : email) lowEmail += static_cast<char>(tolower(c));

    auto rows = db_.query(sql, {lowEmail});
    nlohmann::json arr = nlohmann::json::array();

    for (const auto& r : rows) {
        nlohmann::json obj;
        obj["booking_id"] = std::stoi(r.at("booking_id"));
        obj["booking_reference"] = r.at("booking_reference");
        obj["customer_name"] = r.at("customer_name");
        obj["customer_email"] = r.at("customer_email");
        obj["movie_title"] = r.at("movie_title");
        obj["poster_url"] = r.at("poster_url");
        obj["theatre_name"] = r.at("theatre_name");
        obj["theatre_city"] = r.at("city");
        obj["screen_name"] = r.at("screen_name");
        obj["show_date"] = r.at("show_date");
        obj["show_time"] = r.at("show_time");
        obj["total_amount"] = std::stod(r.at("total_amount"));
        obj["ticket_count"] = std::stoi(r.at("ticket_count"));
        obj["payment_status"] = r.at("payment_status");
        obj["booking_status"] = r.at("booking_status");
        obj["created_at"] = r.at("created_at");

        // Fetch seat labels
        auto seatRows = db_.query(
            "SELECT st.seat_number FROM BOOKING_SEATS bs JOIN SEATS st ON bs.seat_id = st.id WHERE bs.booking_id = ?;",
            {r.at("booking_id")}
        );
        std::string seatsStr = "";
        for (size_t i = 0; i < seatRows.size(); ++i) {
            if (i > 0) seatsStr += ", ";
            seatsStr += seatRows[i].at("seat_number");
        }
        obj["seats_summary"] = seatsStr;

        arr.push_back(obj);
    }

    return arr;
}

bool BookingService::cancelBooking(const std::string& bookingRef, std::string& outError) {
    if (!db_.beginTransaction()) {
        outError = "Could not start cancellation transaction.";
        return false;
    }

    auto rows = db_.query("SELECT id, booking_status FROM BOOKINGS WHERE booking_reference = ?;", {bookingRef});
    if (rows.empty()) {
        db_.rollbackTransaction();
        outError = "Booking not found with reference: " + bookingRef;
        return false;
    }

    const auto& b = rows[0];
    if (b.at("booking_status") == "CANCELLED") {
        db_.rollbackTransaction();
        outError = "This booking is already cancelled.";
        return false;
    }

    std::string bookingId = b.at("id");

    // 1. Update BOOKINGS table
    int updatedBooking = db_.executeUpdate(
        "UPDATE BOOKINGS SET booking_status = 'CANCELLED', payment_status = 'REFUNDED' WHERE id = ?;",
        {bookingId}
    );

    if (updatedBooking <= 0) {
        db_.rollbackTransaction();
        outError = "Failed to update booking status.";
        return false;
    }

    // 2. Update BOOKING_SEATS table to release the seats
    int updatedSeats = db_.executeUpdate(
        "UPDATE BOOKING_SEATS SET status = 'CANCELLED' WHERE booking_id = ?;",
        {bookingId}
    );

    if (updatedSeats <= 0) {
        db_.rollbackTransaction();
        outError = "Failed to release booked seats.";
        return false;
    }

    // 3. Commit the transaction
    if (!db_.commitTransaction()) {
        db_.rollbackTransaction();
        outError = "Failed to commit cancellation transaction.";
        return false;
    }

    return true;
}

} // namespace cinebook
