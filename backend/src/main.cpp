#include "database.hpp"
#include "auth_service.hpp"
#include "movie_service.hpp"
#include "booking_service.hpp"
#include "admin_service.hpp"
#include "http_server.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "========================================================\n"
              << "   CineBook - Movie Ticket Booking System (Backend)    \n"
              << "   Database Systems Design Mini-Project                \n"
              << "========================================================\n";

    std::string configPath = "config/cinebook.conf";
    if (argc > 1) {
        configPath = argv[1];
    }

    std::cout << "[Info] Loading configuration from: " << configPath << std::endl;
    cinebook::DbConfig config = cinebook::loadConfig(configPath);

    std::cout << "[Info] Target DB Engine : " << config.dbEngine << std::endl;
    if (config.dbEngine == "oracle") {
        std::cout << "[Info] Oracle Target    : " << config.oracleConnectString << std::endl;
        std::cout << "[Info] Oracle User      : " << config.oracleUser << std::endl;
    } else {
        std::cout << "[Info] Local DB File    : " << config.localDbPath << std::endl;
    }

    // Initialize Database
    auto db = cinebook::createDatabase(config);
    if (!db || !db->isConnected()) {
        std::cerr << "[Fatal Error] Could not connect to any database engine. Exiting." << std::endl;
        return 1;
    }

    // Verify and initialize schema & initial seed data
    if (!cinebook::initializeDatabase(*db)) {
        std::cerr << "[Fatal Error] Database schema initialization failed." << std::endl;
        return 1;
    }

    // Instantiate Services
    cinebook::AuthService authService(*db, config.adminAuthToken);
    cinebook::MovieService movieService(*db);
    cinebook::BookingService bookingService(*db);
    cinebook::AdminService adminService(*db, config.adminAuthToken);

    // Launch HTTP Server
    cinebook::HttpServer server(config, *db, authService, movieService, bookingService, adminService);
    server.start();

    return 0;
}
