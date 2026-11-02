#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "http_server.hpp"
#include "httplib.h"
#include "json.hpp"
#include <iostream>

namespace cinebook {

HttpServer::HttpServer(
    DbConfig config,
    IDatabase& db,
    AuthService authService,
    MovieService movieService,
    BookingService bookingService,
    AdminService adminService
) : config_(std::move(config)),
    db_(db),
    authService_(authService),
    movieService_(movieService),
    bookingService_(bookingService),
    adminService_(adminService) {}

void HttpServer::start() {
    httplib::Server svr;

    // Static file serving from frontend directory
    svr.set_mount_point("/", "frontend");
    svr.set_file_extension_and_mimetype_mapping("html", "text/html");
    svr.set_file_extension_and_mimetype_mapping("css", "text/css");
    svr.set_file_extension_and_mimetype_mapping("js", "application/javascript");
    svr.set_file_extension_and_mimetype_mapping("json", "application/json");
    svr.set_file_extension_and_mimetype_mapping("svg", "image/svg+xml");
    svr.set_file_extension_and_mimetype_mapping("png", "image/png");
    svr.set_file_extension_and_mimetype_mapping("jpg", "image/jpeg");

    // Pre-routing CORS and Logging filter
    svr.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Admin-Token");

        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // ------------------------------------------------------------------------
    // API: Authentication (User & Admin Logins)
    // ------------------------------------------------------------------------
    svr.Post("/api/auth/login", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string email = body.value("email", "");
            std::string password = body.value("password", "");
            std::string role = body.value("role", "");

            std::string err;
            auto user = authService_.login(email, password, role, err);
            if (user.is_null()) {
                res.status = 401;
                res.set_content(nlohmann::json({{"error", err}}).dump(), "application/json");
            } else {
                res.status = 200;
                res.set_content(nlohmann::json({{"success", true}, {"user", user}}).dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    svr.Post("/api/auth/register", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string username = body.value("username", "");
            std::string email = body.value("email", "");
            std::string password = body.value("password", "");

            std::string err;
            auto user = authService_.registerUser(username, email, password, err);
            if (user.is_null()) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", err}}).dump(), "application/json");
            } else {
                res.status = 201;
                res.set_content(nlohmann::json({{"success", true}, {"user", user}}).dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    // ------------------------------------------------------------------------
    // API: Database & System Status
    // ------------------------------------------------------------------------
    svr.Get("/api/db/status", [this](const httplib::Request&, httplib::Response& res) {
        nlohmann::json obj = {
            {"status", db_.isConnected() ? "connected" : "disconnected"},
            {"engine", db_.getEngineName()},
            {"is_oracle", db_.isOracle()},
            {"oracle_target", config_.oracleConnectString},
            {"local_db_path", config_.localDbPath}
        };
        res.set_content(obj.dump(), "application/json");
    });

    // ------------------------------------------------------------------------
    // API: Movies & Filters
    // ------------------------------------------------------------------------
    svr.Get("/api/movies", [this](const httplib::Request& req, httplib::Response& res) {
        std::string search = req.has_param("search") ? req.get_param_value("search") : "";
        std::string genre = req.has_param("genre") ? req.get_param_value("genre") : "";
        std::string language = req.has_param("language") ? req.get_param_value("language") : "";
        int upcoming = req.has_param("upcoming") ? std::stoi(req.get_param_value("upcoming")) : -1;

        auto movies = movieService_.getMovies(search, genre, language, upcoming);
        res.set_content(movies.dump(), "application/json");
    });

    svr.Get(R"(/api/movies/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        auto movie = movieService_.getMovieById(id);
        if (movie.is_null()) {
            res.status = 404;
            res.set_content(R"({"error":"Movie not found"})", "application/json");
        } else {
            res.set_content(movie.dump(), "application/json");
        }
    });

    svr.Get("/api/genres", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(movieService_.getGenres().dump(), "application/json");
    });

    svr.Get("/api/languages", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(movieService_.getLanguages().dump(), "application/json");
    });

    // ------------------------------------------------------------------------
    // API: Shows & Seat Selection
    // ------------------------------------------------------------------------
    svr.Get("/api/shows", [this](const httplib::Request& req, httplib::Response& res) {
        int movieId = req.has_param("movie_id") ? std::stoi(req.get_param_value("movie_id")) : 0;
        std::string date = req.has_param("date") ? req.get_param_value("date") : "";

        auto shows = movieService_.getShows(movieId, date);
        res.set_content(shows.dump(), "application/json");
    });

    svr.Get(R"(/api/shows/(\d+)/seats)", [this](const httplib::Request& req, httplib::Response& res) {
        int showId = std::stoi(req.matches[1]);
        auto showData = movieService_.getShowDetailsAndSeats(showId);
        if (showData.is_null()) {
            res.status = 404;
            res.set_content(R"({"error":"Show not found"})", "application/json");
        } else {
            res.set_content(showData.dump(), "application/json");
        }
    });

    // ------------------------------------------------------------------------
    // API: Bookings
    // ------------------------------------------------------------------------
    svr.Post("/api/bookings", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string name = body.value("customer_name", "");
            std::string email = body.value("customer_email", "");
            std::string phone = body.value("customer_phone", "");
            int showId = body.value("show_id", 0);

            std::vector<int> seatIds;
            if (body.contains("seat_ids") && body["seat_ids"].is_array()) {
                for (const auto& item : body["seat_ids"]) {
                    seatIds.push_back(item.get<int>());
                }
            }

            std::string error;
            int statusCode = 200;
            auto result = bookingService_.createBooking(name, email, phone, showId, seatIds, error, statusCode);

            res.status = statusCode;
            if (result.is_null()) {
                nlohmann::json errObj = {{"error", error}};
                res.set_content(errObj.dump(), "application/json");
            } else {
                res.set_content(result.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            nlohmann::json errObj = {{"error", std::string("Invalid JSON payload: ") + e.what()}};
            res.set_content(errObj.dump(), "application/json");
        }
    });

    svr.Get("/api/bookings", [this](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("email")) {
            res.status = 400;
            res.set_content(R"({"error":"email query parameter is required"})", "application/json");
            return;
        }
        std::string email = req.get_param_value("email");
        auto bookings = bookingService_.getUserBookings(email);
        res.set_content(bookings.dump(), "application/json");
    });

    svr.Get(R"(/api/bookings/([A-Za-z0-9\-]+))", [this](const httplib::Request& req, httplib::Response& res) {
        std::string ref = req.matches[1];
        std::string email = req.has_param("email") ? req.get_param_value("email") : "";
        auto booking = bookingService_.getBookingByReference(ref, email);
        if (booking.is_null()) {
            res.status = 404;
            res.set_content(R"({"error":"Booking not found or email does not match"})", "application/json");
        } else {
            res.set_content(booking.dump(), "application/json");
        }
    });

    svr.Post(R"(/api/bookings/([A-Za-z0-9\-]+)/cancel)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string ref = req.matches[1];
        std::string error;
        bool ok = bookingService_.cancelBooking(ref, error);
        if (!ok) {
            res.status = 400;
            nlohmann::json errObj = {{"error", error}};
            res.set_content(errObj.dump(), "application/json");
        } else {
            nlohmann::json okObj = {
                {"success", true},
                {"message", "Booking " + ref + " has been cancelled and seats have been released."}
            };
            res.set_content(okObj.dump(), "application/json");
        }
    });

    // ------------------------------------------------------------------------
    // API: Admin Operations
    // ------------------------------------------------------------------------
    auto checkAdmin = [this](const httplib::Request& req, httplib::Response& res) -> bool {
        std::string token = "";
        if (req.has_header("X-Admin-Token")) {
            token = req.get_header_value("X-Admin-Token");
        } else if (req.has_param("token")) {
            token = req.get_param_value("token");
        }
        if (!adminService_.verifyToken(token)) {
            res.status = 401;
            res.set_content(R"({"error":"Unauthorized: Invalid admin credentials"})", "application/json");
            return false;
        }
        return true;
    };

    svr.Get("/api/admin/stats", [this, checkAdmin](const httplib::Request& req, httplib::Response& res) {
        if (!checkAdmin(req, res)) return;
        res.set_content(adminService_.getDashboardStats().dump(), "application/json");
    });

    svr.Get("/api/admin/users", [this, checkAdmin](const httplib::Request& req, httplib::Response& res) {
        if (!checkAdmin(req, res)) return;
        res.set_content(adminService_.getAllUsers().dump(), "application/json");
    });

    svr.Delete(R"(/api/admin/users/(\d+))", [this, checkAdmin](const httplib::Request& req, httplib::Response& res) {
        if (!checkAdmin(req, res)) return;
        int id = std::stoi(req.matches[1]);
        std::string err;
        bool ok = adminService_.deleteUser(id, err);
        if (!ok) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", err}}).dump(), "application/json");
        } else {
            res.set_content(R"({"success":true,"message":"User account and associated records deleted successfully"})", "application/json");
        }
    });

    svr.Post("/api/admin/movies", [this, checkAdmin](const httplib::Request& req, httplib::Response& res) {
        if (!checkAdmin(req, res)) return;
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string err;
            auto result = adminService_.addMovie(body, err);
            if (result.is_null()) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", err}}).dump(), "application/json");
            } else {
                res.status = 201;
                res.set_content(result.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    svr.Put(R"(/api/admin/movies/(\d+))", [this, checkAdmin](const httplib::Request& req, httplib::Response& res) {
        if (!checkAdmin(req, res)) return;
        try {
            int id = std::stoi(req.matches[1]);
            auto body = nlohmann::json::parse(req.body);
            std::string err;
            auto result = adminService_.updateMovie(id, body, err);
            if (result.is_null()) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", err}}).dump(), "application/json");
            } else {
                res.set_content(result.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    svr.Delete(R"(/api/admin/movies/(\d+))", [this, checkAdmin](const httplib::Request& req, httplib::Response& res) {
        if (!checkAdmin(req, res)) return;
        int id = std::stoi(req.matches[1]);
        std::string err;
        bool ok = adminService_.deleteMovie(id, err);
        if (!ok) {
            res.status = 409;
            res.set_content(nlohmann::json({{"error", err}}).dump(), "application/json");
        } else {
            res.set_content(R"({"success":true,"message":"Movie deleted successfully"})", "application/json");
        }
    });

    svr.Get("/api/admin/theatres", [this, checkAdmin](const httplib::Request& req, httplib::Response& res) {
        if (!checkAdmin(req, res)) return;
        res.set_content(adminService_.getTheatres().dump(), "application/json");
    });

    svr.Post("/api/admin/theatres", [this, checkAdmin](const httplib::Request& req, httplib::Response& res) {
        if (!checkAdmin(req, res)) return;
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string name = body.value("name", "");
            std::string location = body.value("location", "");
            std::string city = body.value("city", "");
            std::string err;
            auto result = adminService_.addTheatre(name, location, city, err);
            if (result.is_null()) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", err}}).dump(), "application/json");
            } else {
                res.status = 201;
                res.set_content(result.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    svr.Get("/api/admin/screens", [this, checkAdmin](const httplib::Request& req, httplib::Response& res) {
        if (!checkAdmin(req, res)) return;
        int tId = req.has_param("theatre_id") ? std::stoi(req.get_param_value("theatre_id")) : 0;
        res.set_content(adminService_.getScreens(tId).dump(), "application/json");
    });

    svr.Post("/api/admin/screens", [this, checkAdmin](const httplib::Request& req, httplib::Response& res) {
        if (!checkAdmin(req, res)) return;
        try {
            auto body = nlohmann::json::parse(req.body);
            int theatreId = body.value("theatre_id", 0);
            std::string screenName = body.value("screen_name", "");
            int totalSeats = body.value("total_seats", 25);
            std::string err;
            auto result = adminService_.addScreen(theatreId, screenName, totalSeats, err);
            if (result.is_null()) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", err}}).dump(), "application/json");
            } else {
                res.status = 201;
                res.set_content(result.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    svr.Get("/api/admin/shows", [this, checkAdmin](const httplib::Request& req, httplib::Response& res) {
        if (!checkAdmin(req, res)) return;
        res.set_content(adminService_.getAllShows().dump(), "application/json");
    });

    svr.Post("/api/admin/shows", [this, checkAdmin](const httplib::Request& req, httplib::Response& res) {
        if (!checkAdmin(req, res)) return;
        try {
            auto body = nlohmann::json::parse(req.body);
            int movieId = body.value("movie_id", 0);
            int screenId = body.value("screen_id", 0);
            std::string showDate = body.value("show_date", "");
            std::string showTime = body.value("show_time", "");
            std::string err;
            auto result = adminService_.addShow(movieId, screenId, showDate, showTime, err);
            if (result.is_null()) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", err}}).dump(), "application/json");
            } else {
                res.status = 201;
                res.set_content(result.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    svr.Delete(R"(/api/admin/shows/(\d+))", [this, checkAdmin](const httplib::Request& req, httplib::Response& res) {
        if (!checkAdmin(req, res)) return;
        int id = std::stoi(req.matches[1]);
        std::string err;
        bool ok = adminService_.deleteShow(id, err);
        if (!ok) {
            res.status = 409;
            res.set_content(nlohmann::json({{"error", err}}).dump(), "application/json");
        } else {
            res.set_content(R"({"success":true,"message":"Show deleted successfully"})", "application/json");
        }
    });

    svr.Get("/api/admin/bookings", [this, checkAdmin](const httplib::Request& req, httplib::Response& res) {
        if (!checkAdmin(req, res)) return;
        res.set_content(adminService_.getAllBookings().dump(), "application/json");
    });

    svr.Put(R"(/api/admin/bookings/([A-Za-z0-9\-]+)/status)", [this, checkAdmin](const httplib::Request& req, httplib::Response& res) {
        if (!checkAdmin(req, res)) return;
        try {
            std::string ref = req.matches[1];
            auto body = nlohmann::json::parse(req.body);
            std::string newStatus = body.value("status", "CONFIRMED");
            std::string err;
            bool ok = adminService_.updateBookingStatus(ref, newStatus, err);
            if (!ok) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", err}}).dump(), "application/json");
            } else {
                res.status = 201;
                res.set_content(R"({"success":true,"message":"Status updated successfully"})", "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(nlohmann::json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    std::cout << "\n======================================================\n"
              << " CineBook C++ Server Running at http://localhost:" << config_.serverPort << "\n"
              << " Database Engine: " << db_.getEngineName() << "\n"
              << " Static Frontend: Serving from ./frontend\n"
              << " Press Ctrl+C in terminal to stop.\n"
              << "======================================================\n" << std::endl;

    svr.listen(config_.serverHost, config_.serverPort);
}

} // namespace cinebook
