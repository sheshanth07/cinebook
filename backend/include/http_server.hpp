#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include "database.hpp"
#include "auth_service.hpp"
#include "movie_service.hpp"
#include "booking_service.hpp"
#include "admin_service.hpp"
#include <string>

namespace cinebook {

class HttpServer {
private:
    DbConfig config_;
    IDatabase& db_;
    AuthService authService_;
    MovieService movieService_;
    BookingService bookingService_;
    AdminService adminService_;

public:
    HttpServer(
        DbConfig config,
        IDatabase& db,
        AuthService authService,
        MovieService movieService,
        BookingService bookingService,
        AdminService adminService
    );

    void start();
};

} // namespace cinebook

#endif // HTTP_SERVER_HPP
