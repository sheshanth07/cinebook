#ifndef BOOKING_SERVICE_HPP
#define BOOKING_SERVICE_HPP

#include "database.hpp"
#include "json.hpp"
#include <string>
#include <vector>

namespace cinebook {

class BookingService {
private:
    IDatabase& db_;

    std::string generateBookingReference();

public:
    explicit BookingService(IDatabase& db) : db_(db) {}

    nlohmann::json createBooking(
        const std::string& customerName,
        const std::string& customerEmail,
        const std::string& customerPhone,
        int showId,
        const std::vector<int>& seatIds,
        std::string& outError,
        int& outStatusCode
    );

    nlohmann::json getBookingByReference(
        const std::string& bookingRef, 
        const std::string& email = ""
    );

    nlohmann::json getUserBookings(const std::string& email);

    bool cancelBooking(const std::string& bookingRef, std::string& outError);
};

} // namespace cinebook

#endif // BOOKING_SERVICE_HPP
