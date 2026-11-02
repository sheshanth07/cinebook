#include "movie_service.hpp"
#include <sstream>
#include <iostream>

namespace cinebook {

nlohmann::json MovieService::getMovies(
    const std::string& search,
    const std::string& genre,
    const std::string& language,
    int isUpcoming
) {
    std::string sql = "SELECT id, title, description, genre, language, duration_mins, release_date, "
                      "certificate, rating, cast_details, director, poster_url, banner_url, "
                      "is_upcoming, created_at FROM MOVIES WHERE 1=1 ";
    std::vector<std::string> params;

    if (!search.empty()) {
        sql += " AND (LOWER(title) LIKE ? OR LOWER(director) LIKE ? OR LOWER(cast_details) LIKE ?) ";
        std::string pattern = "%";
        for (char c : search) pattern += static_cast<char>(tolower(c));
        pattern += "%";
        params.push_back(pattern);
        params.push_back(pattern);
        params.push_back(pattern);
    }

    if (!genre.empty()) {
        sql += " AND genre = ? ";
        params.push_back(genre);
    }

    if (!language.empty()) {
        sql += " AND language = ? ";
        params.push_back(language);
    }

    if (isUpcoming >= 0) {
        sql += " AND is_upcoming = ? ";
        params.push_back(std::to_string(isUpcoming));
    }

    sql += " ORDER BY release_date DESC, rating DESC;";

    auto rows = db_.query(sql, params);
    nlohmann::json arr = nlohmann::json::array();

    for (const auto& r : rows) {
        nlohmann::json obj;
        obj["id"] = std::stoi(r.at("id"));
        obj["title"] = r.at("title");
        obj["description"] = r.at("description");
        obj["genre"] = r.at("genre");
        obj["language"] = r.at("language");
        obj["duration_mins"] = std::stoi(r.at("duration_mins"));
        obj["release_date"] = r.at("release_date");
        obj["certificate"] = r.at("certificate");
        obj["rating"] = std::stod(r.at("rating"));
        obj["cast_details"] = r.at("cast_details");
        obj["director"] = r.at("director");
        obj["poster_url"] = r.at("poster_url");
        obj["banner_url"] = r.at("banner_url");
        obj["is_upcoming"] = std::stoi(r.at("is_upcoming"));
        obj["created_at"] = r.at("created_at");
        arr.push_back(obj);
    }

    return arr;
}

nlohmann::json MovieService::getMovieById(int movieId) {
    std::string sql = "SELECT id, title, description, genre, language, duration_mins, release_date, "
                      "certificate, rating, cast_details, director, poster_url, banner_url, "
                      "is_upcoming, created_at FROM MOVIES WHERE id = ?;";
    auto rows = db_.query(sql, {std::to_string(movieId)});
    if (rows.empty()) {
        return nullptr;
    }

    const auto& r = rows[0];
    nlohmann::json obj;
    obj["id"] = std::stoi(r.at("id"));
    obj["title"] = r.at("title");
    obj["description"] = r.at("description");
    obj["genre"] = r.at("genre");
    obj["language"] = r.at("language");
    obj["duration_mins"] = std::stoi(r.at("duration_mins"));
    obj["release_date"] = r.at("release_date");
    obj["certificate"] = r.at("certificate");
    obj["rating"] = std::stod(r.at("rating"));
    obj["cast_details"] = r.at("cast_details");
    obj["director"] = r.at("director");
    obj["poster_url"] = r.at("poster_url");
    obj["banner_url"] = r.at("banner_url");
    obj["is_upcoming"] = std::stoi(r.at("is_upcoming"));
    obj["created_at"] = r.at("created_at");
    return obj;
}

nlohmann::json MovieService::getGenres() {
    auto rows = db_.query("SELECT DISTINCT genre FROM MOVIES ORDER BY genre ASC;");
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& r : rows) {
        if (!r.at("genre").empty()) {
            arr.push_back(r.at("genre"));
        }
    }
    return arr;
}

nlohmann::json MovieService::getLanguages() {
    auto rows = db_.query("SELECT DISTINCT language FROM MOVIES ORDER BY language ASC;");
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& r : rows) {
        if (!r.at("language").empty()) {
            arr.push_back(r.at("language"));
        }
    }
    return arr;
}

nlohmann::json MovieService::getShows(int movieId, const std::string& date) {
    std::string sql = 
        "SELECT s.id AS show_id, s.show_date, s.show_time, "
        "m.id AS movie_id, m.title AS movie_title, m.poster_url, m.certificate, m.duration_mins, "
        "t.id AS theatre_id, t.name AS theatre_name, t.city, "
        "sc.id AS screen_id, sc.screen_name, sc.total_seats, "
        "(sc.total_seats - (SELECT COUNT(*) FROM BOOKING_SEATS bs WHERE bs.show_id = s.id AND bs.status = 'BOOKED')) AS available_seats "
        "FROM SHOWS s "
        "JOIN MOVIES m ON s.movie_id = m.id "
        "JOIN SCREENS sc ON s.screen_id = sc.id "
        "JOIN THEATRES t ON sc.theatre_id = t.id "
        "WHERE 1=1 ";

    std::vector<std::string> params;
    if (movieId > 0) {
        sql += " AND s.movie_id = ? ";
        params.push_back(std::to_string(movieId));
    }
    if (!date.empty()) {
        sql += " AND s.show_date = ? ";
        params.push_back(date);
    }

    sql += " ORDER BY s.show_date ASC, s.show_time ASC;";

    auto rows = db_.query(sql, params);
    nlohmann::json arr = nlohmann::json::array();

    for (const auto& r : rows) {
        nlohmann::json obj;
        obj["show_id"] = std::stoi(r.at("show_id"));
        obj["show_date"] = r.at("show_date");
        obj["show_time"] = r.at("show_time");
        obj["movie_id"] = std::stoi(r.at("movie_id"));
        obj["movie_title"] = r.at("movie_title");
        obj["poster_url"] = r.at("poster_url");
        obj["certificate"] = r.at("certificate");
        obj["duration_mins"] = std::stoi(r.at("duration_mins"));
        obj["theatre_id"] = std::stoi(r.at("theatre_id"));
        obj["theatre_name"] = r.at("theatre_name");
        obj["city"] = r.at("city");
        obj["screen_id"] = std::stoi(r.at("screen_id"));
        obj["screen_name"] = r.at("screen_name");
        obj["total_seats"] = std::stoi(r.at("total_seats"));
        obj["available_seats"] = std::stoi(r.at("available_seats"));
        arr.push_back(obj);
    }

    return arr;
}

nlohmann::json MovieService::getShowDetailsAndSeats(int showId) {
    // 1. Fetch show header info
    std::string showSql = 
        "SELECT s.id AS show_id, s.show_date, s.show_time, "
        "m.id AS movie_id, m.title AS movie_title, m.poster_url, m.duration_mins, m.certificate, "
        "t.id AS theatre_id, t.name AS theatre_name, t.city, t.location, "
        "sc.id AS screen_id, sc.screen_name, sc.total_seats "
        "FROM SHOWS s "
        "JOIN MOVIES m ON s.movie_id = m.id "
        "JOIN SCREENS sc ON s.screen_id = sc.id "
        "JOIN THEATRES t ON sc.theatre_id = t.id "
        "WHERE s.id = ?;";

    auto showRows = db_.query(showSql, {std::to_string(showId)});
    if (showRows.empty()) {
        return nullptr;
    }

    const auto& s = showRows[0];
    nlohmann::json result;
    result["show"] = {
        {"show_id", std::stoi(s.at("show_id"))},
        {"show_date", s.at("show_date")},
        {"show_time", s.at("show_time")},
        {"movie_id", std::stoi(s.at("movie_id"))},
        {"movie_title", s.at("movie_title")},
        {"poster_url", s.at("poster_url")},
        {"duration_mins", std::stoi(s.at("duration_mins"))},
        {"certificate", s.at("certificate")},
        {"theatre_name", s.at("theatre_name")},
        {"theatre_city", s.at("city")},
        {"theatre_location", s.at("location")},
        {"screen_id", std::stoi(s.at("screen_id"))},
        {"screen_name", s.at("screen_name")},
        {"total_seats", std::stoi(s.at("total_seats"))}
    };

    int screenId = std::stoi(s.at("screen_id"));

    // 2. Fetch seats with booked state
    std::string seatSql = 
        "SELECT st.id, st.seat_number, st.row_name, st.category, st.base_price, "
        "CASE WHEN bs.id IS NOT NULL THEN 1 ELSE 0 END AS is_booked "
        "FROM SEATS st "
        "LEFT JOIN BOOKING_SEATS bs ON st.id = bs.seat_id AND bs.show_id = ? AND bs.status = 'BOOKED' "
        "WHERE st.screen_id = ? "
        "ORDER BY st.row_name ASC, st.seat_number ASC;";

    auto seatRows = db_.query(seatSql, {std::to_string(showId), std::to_string(screenId)});
    nlohmann::json seatArray = nlohmann::json::array();

    for (const auto& sr : seatRows) {
        nlohmann::json seatObj;
        seatObj["id"] = std::stoi(sr.at("id"));
        seatObj["seat_number"] = sr.at("seat_number");
        seatObj["row_name"] = sr.at("row_name");
        seatObj["category"] = sr.at("category");
        seatObj["base_price"] = std::stod(sr.at("base_price"));
        seatObj["is_booked"] = (sr.at("is_booked") == "1");
        seatArray.push_back(seatObj);
    }

    result["seats"] = seatArray;
    return result;
}

} // namespace cinebook
