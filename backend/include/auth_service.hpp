#ifndef AUTH_SERVICE_HPP
#define AUTH_SERVICE_HPP

#include "database.hpp"
#include "json.hpp"
#include "sha256.hpp"
#include <string>

namespace cinebook {

class AuthService {
private:
    IDatabase& db_;
    std::string adminAuthToken_;

public:
    AuthService(IDatabase& db, std::string adminAuthToken)
        : db_(db), adminAuthToken_(std::move(adminAuthToken)) {}

    nlohmann::json login(
        const std::string& email, 
        const std::string& password, 
        const std::string& expectedRole, 
        std::string& outError
    );

    nlohmann::json registerUser(
        const std::string& username, 
        const std::string& email, 
        const std::string& password, 
        std::string& outError
    );
};

} // namespace cinebook

#endif // AUTH_SERVICE_HPP
