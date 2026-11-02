#include "admin_service.hpp"
#include <iostream>

namespace cinebook {

nlohmann::json AdminService::getDashboardStats() {
    nlohmann::json stats;

    auto rev = db_.query("SELECT COALESCE(SUM(total_amount), 0) AS total_revenue FROM BOOKINGS WHERE booking_status = 'CONFIRMED';");
    stats["total_revenue"] = rev.empty() ? 0.0 : std::stod(rev[0]["total_revenue"]);

    auto bCount = db_.query("SELECT COUNT(*) AS total_bookings FROM BOOKINGS;");
    stats["total_bookings"] = bCount.empty() ? 0 : std::stoi(bCount[0]["total_bookings"]);

    auto mCount = db_.query("SELECT COUNT(*) AS total_movies FROM MOVIES;");
    stats["total_movies"] = mCount.empty() ? 0 : std::stoi(mCount[0]["total_movies"]);

    auto sCount = db_.query("SELECT COUNT(*) AS total_shows FROM SHOWS;");
    stats["total_shows"] = sCount.empty() ? 0 : std::stoi(sCount[0]["total_shows"]);

    auto tCount = db_.query("SELECT COUNT(*) AS total_theatres FROM THEATRES;");
    stats["total_theatres"] = tCount.empty() ? 0 : std::stoi(tCount[0]["total_theatres"]);

    auto scCount = db_.query("SELECT COUNT(*) AS total_screens FROM SCREENS;");
    stats["total_screens"] = scCount.empty() ? 0 : std::stoi(scCount[0]["total_screens"]);

    return stats;
}

// ----------------------------------------------------------------------------
// User Accounts Management
// ----------------------------------------------------------------------------
nlohmann::json AdminService::getAllUsers() {
    std::string sql = 
        "SELECT u.id, u.username, u.email, u.role, u.created_at, "
        "(SELECT COUNT(*) FROM BOOKINGS b WHERE b.user_id = u.id) AS booking_count "
        "FROM USERS u "
        "ORDER BY u.id DESC;";

    auto rows = db_.query(sql);
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& r : rows) {
        arr.push_back({
            {"id", std::stoi(r.at("id"))},
            {"username", r.at("username")},
            {"email", r.at("email")},
            {"role", r.at("role")},
            {"created_at", r.at("created_at")},
            {"booking_count", std::stoi(r.at("booking_count"))}
        });
    }
    return arr;
}

bool AdminService::deleteUser(int id, std::string& outError) {
    auto rows = db_.query("SELECT id, role, email FROM USERS WHERE id = ?;", {std::to_string(id)});
    if (rows.empty()) {
        outError = "User account not found.";
        return false;
    }

    if (rows[0]["email"] == "admin@cinebook.com" || id == 1) {
        outError = "Protected account: Cannot delete the primary Administrator.";
        return false;
    }

    if (!db_.beginTransaction()) {
        outError = "Failed to begin transaction.";
        return false;
    }

    std::string userIdStr = std::to_string(id);
    db_.executeUpdate("DELETE FROM BOOKING_SEATS WHERE booking_id IN (SELECT id FROM BOOKINGS WHERE user_id = ?);", {userIdStr});
    db_.executeUpdate("DELETE FROM BOOKINGS WHERE user_id = ?;", {userIdStr});
    int res = db_.executeUpdate("DELETE FROM USERS WHERE id = ?;", {userIdStr});

    if (res <= 0) {
        db_.rollbackTransaction();
        outError = "Failed to delete user record.";
        return false;
    }

    db_.commitTransaction();
    return true;
}

// ----------------------------------------------------------------------------
// Movies Management
// ----------------------------------------------------------------------------
nlohmann::json AdminService::addMovie(const nlohmann::json& d, std::string& outError) {
    if (!d.contains("title") || d["title"].get<std::string>().empty()) {
        outError = "Movie title is required.";
        return nullptr;
    }

    std::string title = d["title"].get<std::string>();
    std::string desc = d.value("description", "");
    std::string genre = d.value("genre", "Drama");
    std::string lang = d.value("language", "English");
    int duration = d.value("duration_mins", 120);
    std::string releaseDate = d.value("release_date", "2026-01-01");
    std::string cert = d.value("certificate", "UA");
    double rating = d.value("rating", 7.5);
    std::string cast = d.value("cast_details", "");
    std::string director = d.value("director", "");
    std::string poster = d.value("poster_url", "https://images.unsplash.com/photo-1489599849927-2ee91cede3ba?w=600");
    std::string banner = d.value("banner_url", "https://images.unsplash.com/photo-1518709268805-4e9042af9f23?w=1200");
    int isUpcoming = d.value("is_upcoming", 0);

    std::string sql = 
        "INSERT INTO MOVIES (title, description, genre, language, duration_mins, release_date, "
        "certificate, rating, cast_details, director, poster_url, banner_url, is_upcoming) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    int64_t id = db_.executeInsert(sql, {
        title, desc, genre, lang, std::to_string(duration), releaseDate,
        cert, std::to_string(rating), cast, director, poster, banner, std::to_string(isUpcoming)
    });

    if (id <= 0) {
        outError = "Failed to insert movie into database: " + db_.getLastError();
        return nullptr;
    }

    nlohmann::json res;
    res["id"] = id;
    res["title"] = title;
    return res;
}

nlohmann::json AdminService::updateMovie(int id, const nlohmann::json& d, std::string& outError) {
    auto existing = db_.query("SELECT id FROM MOVIES WHERE id = ?;", {std::to_string(id)});
    if (existing.empty()) {
        outError = "Movie not found.";
        return nullptr;
    }

    std::string title = d.value("title", "");
    std::string desc = d.value("description", "");
    std::string genre = d.value("genre", "Action");
    std::string lang = d.value("language", "English");
    int duration = d.value("duration_mins", 120);
    std::string releaseDate = d.value("release_date", "2026-01-01");
    std::string cert = d.value("certificate", "UA");
    double rating = d.value("rating", 7.5);
    std::string cast = d.value("cast_details", "");
    std::string director = d.value("director", "");
    std::string poster = d.value("poster_url", "");
    std::string banner = d.value("banner_url", "");
    int isUpcoming = d.value("is_upcoming", 0);

    std::string sql = 
        "UPDATE MOVIES SET title = ?, description = ?, genre = ?, language = ?, duration_mins = ?, "
        "release_date = ?, certificate = ?, rating = ?, cast_details = ?, director = ?, "
        "poster_url = ?, banner_url = ?, is_upcoming = ? WHERE id = ?;";

    int rows = db_.executeUpdate(sql, {
        title, desc, genre, lang, std::to_string(duration), releaseDate,
        cert, std::to_string(rating), cast, director, poster, banner, std::to_string(isUpcoming),
        std::to_string(id)
    });

    if (rows < 0) {
        outError = "Failed to update movie: " + db_.getLastError();
        return nullptr;
    }

    nlohmann::json res;
    res["id"] = id;
    res["success"] = true;
    return res;
}

bool AdminService::deleteMovie(int id, std::string& outError) {
    // Safety check: Cannot delete movie if scheduled shows exist
    auto shows = db_.query("SELECT COUNT(*) AS cnt FROM SHOWS WHERE movie_id = ?;", {std::to_string(id)});
    if (!shows.empty() && std::stoi(shows[0]["cnt"]) > 0) {
        outError = "Referential integrity protection: Cannot delete movie because " + 
                   shows[0]["cnt"] + " active scheduled show(s) reference it. Please remove or reschedule the shows first.";
        return false;
    }

    int rows = db_.executeUpdate("DELETE FROM MOVIES WHERE id = ?;", {std::to_string(id)});
    if (rows <= 0) {
        outError = "Movie with ID " + std::to_string(id) + " not found.";
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------
// Theatres & Screens Management
// ----------------------------------------------------------------------------
nlohmann::json AdminService::getTheatres() {
    auto rows = db_.query("SELECT id, name, location, city FROM THEATRES ORDER BY name ASC;");
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& r : rows) {
        arr.push_back({
            {"id", std::stoi(r.at("id"))},
            {"name", r.at("name")},
            {"location", r.at("location")},
            {"city", r.at("city")}
        });
    }
    return arr;
}

nlohmann::json AdminService::addTheatre(
    const std::string& name, 
    const std::string& location, 
    const std::string& city, 
    std::string& outError
) {
    if (name.empty() || city.empty()) {
        outError = "Theatre name and city are required.";
        return nullptr;
    }

    int64_t id = db_.executeInsert(
        "INSERT INTO THEATRES (name, location, city) VALUES (?, ?, ?);",
        {name, location, city}
    );

    if (id <= 0) {
        outError = "Failed to save theatre to database.";
        return nullptr;
    }

    return {{"id", id}, {"name", name}, {"city", city}};
}

nlohmann::json AdminService::getScreens(int theatreId) {
    std::string sql = 
        "SELECT sc.id, sc.theatre_id, sc.screen_name, sc.total_seats, t.name AS theatre_name, t.city "
        "FROM SCREENS sc "
        "JOIN THEATRES t ON sc.theatre_id = t.id ";
    std::vector<std::string> params;

    if (theatreId > 0) {
        sql += " WHERE sc.theatre_id = ? ";
        params.push_back(std::to_string(theatreId));
    }
    sql += " ORDER BY t.name ASC, sc.screen_name ASC;";

    auto rows = db_.query(sql, params);
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& r : rows) {
        arr.push_back({
            {"id", std::stoi(r.at("id"))},
            {"theatre_id", std::stoi(r.at("theatre_id"))},
            {"screen_name", r.at("screen_name")},
            {"total_seats", std::stoi(r.at("total_seats"))},
            {"theatre_name", r.at("theatre_name")},
            {"city", r.at("city")}
        });
    }
    return arr;
}

nlohmann::json AdminService::addScreen(
    int theatreId, 
    const std::string& screenName, 
    int totalSeats, 
    std::string& outError
) {
    if (theatreId <= 0 || screenName.empty()) {
        outError = "Valid theatre and screen name are required.";
        return nullptr;
    }

    int seats = totalSeats > 0 ? totalSeats : 25;

    int64_t screenId = db_.executeInsert(
        "INSERT INTO SCREENS (theatre_id, screen_name, total_seats) VALUES (?, ?, ?);",
        {std::to_string(theatreId), screenName, std::to_string(seats)}
    );

    if (screenId <= 0) {
        outError = "Failed to insert screen into database.";
        return nullptr;
    }

    // Automatically generate standard 25 seats (Rows A to E) for instant availability
    char rows[] = {'A', 'B', 'C', 'D', 'E'};
    for (char r : rows) {
        std::string cat = (r == 'D' || r == 'E') ? "Premium" : "Regular";
        double price = (r == 'E') ? 350.0 : (r == 'D' ? 300.0 : 180.0);
        for (int num = 1; num <= 5; ++num) {
            std::string seatNum = std::string(1, r) + std::to_string(num);
            db_.executeInsert(
                "INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (?, ?, ?, ?, ?);",
                {std::to_string(screenId), seatNum, std::string(1, r), cat, std::to_string(price)}
            );
        }
    }

    return {{"id", screenId}, {"screen_name", screenName}, {"total_seats", seats}};
}

// ----------------------------------------------------------------------------
// Seats Management
// ----------------------------------------------------------------------------
nlohmann::json AdminService::getSeats(int screenId) {
    auto rows = db_.query(
        "SELECT id, screen_id, seat_number, row_name, category, base_price FROM SEATS WHERE screen_id = ? ORDER BY row_name ASC, seat_number ASC;",
        {std::to_string(screenId)}
    );
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& r : rows) {
        arr.push_back({
            {"id", std::stoi(r.at("id"))},
            {"screen_id", std::stoi(r.at("screen_id"))},
            {"seat_number", r.at("seat_number")},
            {"row_name", r.at("row_name")},
            {"category", r.at("category")},
            {"base_price", std::stod(r.at("base_price"))}
        });
    }
    return arr;
}

nlohmann::json AdminService::addSeat(
    int screenId, 
    const std::string& seatNumber, 
    const std::string& rowName, 
    const std::string& category, 
    double price, 
    std::string& outError
) {
    if (screenId <= 0 || seatNumber.empty()) {
        outError = "Screen ID and seat number are required.";
        return nullptr;
    }

    int64_t id = db_.executeInsert(
        "INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (?, ?, ?, ?, ?);",
        {std::to_string(screenId), seatNumber, rowName, category, std::to_string(price)}
    );

    if (id <= 0) {
        outError = "Seat number may already exist on this screen.";
        return nullptr;
    }

    return {{"id", id}, {"seat_number", seatNumber}};
}

// ----------------------------------------------------------------------------
// Shows Management
// ----------------------------------------------------------------------------
nlohmann::json AdminService::getAllShows() {
    std::string sql = 
        "SELECT s.id, s.show_date, s.show_time, "
        "m.id AS movie_id, m.title AS movie_title, "
        "sc.id AS screen_id, sc.screen_name, "
        "t.name AS theatre_name, t.city, "
        "(SELECT COUNT(*) FROM BOOKING_SEATS bs WHERE bs.show_id = s.id AND bs.status = 'BOOKED') AS booked_count, "
        "sc.total_seats "
        "FROM SHOWS s "
        "JOIN MOVIES m ON s.movie_id = m.id "
        "JOIN SCREENS sc ON s.screen_id = sc.id "
        "JOIN THEATRES t ON sc.theatre_id = t.id "
        "ORDER BY s.show_date DESC, s.show_time ASC;";

    auto rows = db_.query(sql);
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& r : rows) {
        arr.push_back({
            {"id", std::stoi(r.at("id"))},
            {"show_date", r.at("show_date")},
            {"show_time", r.at("show_time")},
            {"movie_id", std::stoi(r.at("movie_id"))},
            {"movie_title", r.at("movie_title")},
            {"screen_id", std::stoi(r.at("screen_id"))},
            {"screen_name", r.at("screen_name")},
            {"theatre_name", r.at("theatre_name")},
            {"city", r.at("city")},
            {"booked_count", std::stoi(r.at("booked_count"))},
            {"total_seats", std::stoi(r.at("total_seats"))}
        });
    }
    return arr;
}

nlohmann::json AdminService::addShow(
    int movieId, 
    int screenId, 
    const std::string& showDate, 
    const std::string& showTime, 
    std::string& outError
) {
    if (movieId <= 0 || screenId <= 0 || showDate.empty() || showTime.empty()) {
        outError = "Movie, screen, show date and time are required.";
        return nullptr;
    }

    // Check unique collision on screen at same date & time
    auto collision = db_.query(
        "SELECT id FROM SHOWS WHERE screen_id = ? AND show_date = ? AND show_time = ?;",
        {std::to_string(screenId), showDate, showTime}
    );
    if (!collision.empty()) {
        outError = "Schedule collision: This screen is already booked for another show at " + showTime + " on " + showDate + ".";
        return nullptr;
    }

    int64_t showId = db_.executeInsert(
        "INSERT INTO SHOWS (movie_id, screen_id, show_date, show_time) VALUES (?, ?, ?, ?);",
        {std::to_string(movieId), std::to_string(screenId), showDate, showTime}
    );

    if (showId <= 0) {
        outError = "Failed to schedule show: " + db_.getLastError();
        return nullptr;
    }

    return {{"id", showId}, {"show_date", showDate}, {"show_time", showTime}};
}

nlohmann::json AdminService::updateShow(
    int id, 
    int movieId, 
    int screenId, 
    const std::string& showDate, 
    const std::string& showTime, 
    std::string& outError
) {
    int rows = db_.executeUpdate(
        "UPDATE SHOWS SET movie_id = ?, screen_id = ?, show_date = ?, show_time = ? WHERE id = ?;",
        {std::to_string(movieId), std::to_string(screenId), showDate, showTime, std::to_string(id)}
    );

    if (rows <= 0) {
        outError = "Show update failed: " + db_.getLastError();
        return nullptr;
    }

    return {{"id", id}, {"success", true}};
}

bool AdminService::deleteShow(int id, std::string& outError) {
    std::string showIdStr = std::to_string(id);
    if (!db_.beginTransaction()) {
        outError = "Could not start database transaction.";
        return false;
    }

    // Cancel all bookings and free seat records for this show
    db_.executeUpdate("DELETE FROM BOOKING_SEATS WHERE show_id = ?;", {showIdStr});
    db_.executeUpdate("DELETE FROM BOOKINGS WHERE show_id = ?;", {showIdStr});
    int rows = db_.executeUpdate("DELETE FROM SHOWS WHERE id = ?;", {showIdStr});

    if (rows <= 0) {
        db_.rollbackTransaction();
        outError = "Show with ID " + showIdStr + " not found.";
        return false;
    }

    db_.commitTransaction();
    return true;
}

// ----------------------------------------------------------------------------
// Bookings Management
// ----------------------------------------------------------------------------
nlohmann::json AdminService::getAllBookings() {
    std::string sql = 
        "SELECT b.id, b.booking_reference, b.total_amount, b.ticket_count, "
        "b.payment_status, b.booking_status, b.created_at, "
        "u.username AS customer_name, u.email AS customer_email, "
        "m.title AS movie_title, t.name AS theatre_name, sc.screen_name, "
        "s.show_date, s.show_time "
        "FROM BOOKINGS b "
        "JOIN USERS u ON b.user_id = u.id "
        "JOIN SHOWS s ON b.show_id = s.id "
        "JOIN MOVIES m ON s.movie_id = m.id "
        "JOIN SCREENS sc ON s.screen_id = sc.id "
        "JOIN THEATRES t ON sc.theatre_id = t.id "
        "ORDER BY b.id DESC;";

    auto rows = db_.query(sql);
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& r : rows) {
        arr.push_back({
            {"id", std::stoi(r.at("id"))},
            {"booking_reference", r.at("booking_reference")},
            {"customer_name", r.at("customer_name")},
            {"customer_email", r.at("customer_email")},
            {"movie_title", r.at("movie_title")},
            {"theatre_name", r.at("theatre_name")},
            {"screen_name", r.at("screen_name")},
            {"show_date", r.at("show_date")},
            {"show_time", r.at("show_time")},
            {"total_amount", std::stod(r.at("total_amount"))},
            {"ticket_count", std::stoi(r.at("ticket_count"))},
            {"payment_status", r.at("payment_status")},
            {"booking_status", r.at("booking_status")},
            {"created_at", r.at("created_at")}
        });
    }
    return arr;
}

bool AdminService::updateBookingStatus(
    const std::string& bookingRef, 
    const std::string& newStatus, 
    std::string& outError
) {
    if (newStatus != "CONFIRMED" && newStatus != "CANCELLED") {
        outError = "Status must be CONFIRMED or CANCELLED.";
        return false;
    }

    if (newStatus == "CANCELLED") {
        // Also release seats
        auto rows = db_.query("SELECT id FROM BOOKINGS WHERE booking_reference = ?;", {bookingRef});
        if (rows.empty()) {
            outError = "Booking not found.";
            return false;
        }
        std::string bId = rows[0]["id"];
        db_.executeUpdate("UPDATE BOOKINGS SET booking_status = 'CANCELLED', payment_status = 'REFUNDED' WHERE id = ?;", {bId});
        db_.executeUpdate("UPDATE BOOKING_SEATS SET status = 'CANCELLED' WHERE booking_id = ?;", {bId});
        return true;
    }

    int rows = db_.executeUpdate(
        "UPDATE BOOKINGS SET booking_status = ? WHERE booking_reference = ?;",
        {newStatus, bookingRef}
    );
    if (rows <= 0) {
        outError = "Booking reference not found.";
        return false;
    }
    return true;
}

} // namespace cinebook
