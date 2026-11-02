#ifndef ADMIN_SERVICE_HPP
#define ADMIN_SERVICE_HPP

#include "database.hpp"
#include "json.hpp"
#include <string>

namespace cinebook {

class AdminService {
private:
    IDatabase& db_;
    std::string authToken_;

public:
    AdminService(IDatabase& db, std::string authToken)
        : db_(db), authToken_(std::move(authToken)) {}

    bool verifyToken(const std::string& token) const {
        return !authToken_.empty() && token == authToken_;
    }

    nlohmann::json getDashboardStats();

    // Accounts / Users Management
    nlohmann::json getAllUsers();
    bool deleteUser(int id, std::string& outError);

    // Movies
    nlohmann::json addMovie(const nlohmann::json& data, std::string& outError);
    nlohmann::json updateMovie(int id, const nlohmann::json& data, std::string& outError);
    bool deleteMovie(int id, std::string& outError);

    // Theatres & Screens
    nlohmann::json getTheatres();
    nlohmann::json addTheatre(const std::string& name, const std::string& location, const std::string& city, std::string& outError);
    nlohmann::json getScreens(int theatreId = 0);
    nlohmann::json addScreen(int theatreId, const std::string& screenName, int totalSeats, std::string& outError);

    // Seats
    nlohmann::json getSeats(int screenId);
    nlohmann::json addSeat(int screenId, const std::string& seatNumber, const std::string& rowName, const std::string& category, double price, std::string& outError);

    // Shows
    nlohmann::json getAllShows();
    nlohmann::json addShow(int movieId, int screenId, const std::string& showDate, const std::string& showTime, std::string& outError);
    nlohmann::json updateShow(int id, int movieId, int screenId, const std::string& showDate, const std::string& showTime, std::string& outError);
    bool deleteShow(int id, std::string& outError);

    // Bookings
    nlohmann::json getAllBookings();
    bool updateBookingStatus(const std::string& bookingRef, const std::string& newStatus, std::string& outError);
};

} // namespace cinebook

#endif // ADMIN_SERVICE_HPP
