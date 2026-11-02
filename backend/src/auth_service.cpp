#include "auth_service.hpp"
#include <iostream>
#include <algorithm>

namespace cinebook {

nlohmann::json AuthService::login(
    const std::string& email, 
    const std::string& password, 
    const std::string& expectedRole, 
    std::string& outError
) {
    if (email.empty() || password.empty()) {
        outError = "Email and password are required.";
        return nullptr;
    }

    std::string lowEmail = email;
    std::transform(lowEmail.begin(), lowEmail.end(), lowEmail.begin(), ::tolower);

    auto rows = db_.query(
        "SELECT id, username, email, password_hash, role FROM USERS WHERE LOWER(email) = ?;",
        {lowEmail}
    );

    if (rows.empty()) {
        outError = "No account found matching email: " + email;
        return nullptr;
    }

    const auto& user = rows[0];
    std::string inputHash = SHA256::hash(password);
    std::string storedHash = user.at("password_hash");

    if (inputHash != storedHash) {
        outError = "Incorrect password. Please verify your credentials.";
        return nullptr;
    }

    std::string role = user.at("role");
    if (!expectedRole.empty() && expectedRole == "admin" && role != "admin") {
        outError = "Access denied: This account is a regular Customer account and lacks Administrator rights.";
        return nullptr;
    }

    nlohmann::json res;
    res["id"] = std::stoi(user.at("id"));
    res["username"] = user.at("username");
    res["email"] = user.at("email");
    res["role"] = role;
    if (role == "admin") {
        res["admin_token"] = adminAuthToken_;
    }

    return res;
}

nlohmann::json AuthService::registerUser(
    const std::string& username, 
    const std::string& email, 
    const std::string& password, 
    std::string& outError
) {
    if (username.empty()) {
        outError = "Username / Full Name is required.";
        return nullptr;
    }
    if (email.empty() || email.find('@') == std::string::npos) {
        outError = "A valid email address is required.";
        return nullptr;
    }
    if (password.length() < 4) {
        outError = "Password must be at least 4 characters long.";
        return nullptr;
    }

    std::string lowEmail = email;
    std::transform(lowEmail.begin(), lowEmail.end(), lowEmail.begin(), ::tolower);

    // Check duplicate
    auto existing = db_.query("SELECT id FROM USERS WHERE LOWER(email) = ?;", {lowEmail});
    if (!existing.empty()) {
        outError = "This email is already registered. Please log in.";
        return nullptr;
    }

    std::string hash = SHA256::hash(password);
    int64_t newId = db_.executeInsert(
        "INSERT INTO USERS (username, email, password_hash, role) VALUES (?, ?, ?, 'user');",
        {username, lowEmail, hash}
    );

    if (newId <= 0) {
        outError = "Failed to create account in database: " + db_.getLastError();
        return nullptr;
    }

    nlohmann::json res;
    res["id"] = newId;
    res["username"] = username;
    res["email"] = lowEmail;
    res["role"] = "user";
    return res;
}

} // namespace cinebook
