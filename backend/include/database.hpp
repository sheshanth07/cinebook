#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>

namespace cinebook {

struct DbConfig {
    std::string serverHost = "0.0.0.0";
    int serverPort = 8080;
    std::string dbEngine = "local"; // "oracle" or "local"
    std::string oracleUser = "cinebook";
    std::string oraclePassword = "cinebook_secret";
    std::string oracleConnectString = "localhost:1521/XEPDB1";
    std::string localDbPath = "cinebook.db";
    std::string adminAuthToken = "cinebook_admin_secure_key_2026";
};

// Abstract Database Interface
class IDatabase {
public:
    virtual ~IDatabase() = default;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // Execute DDL or statements without result set
    virtual bool execute(const std::string& sql) = 0;

    // Parameterized SELECT query returning vector of key-value row maps
    virtual std::vector<std::map<std::string, std::string>> query(
        const std::string& sql, 
        const std::vector<std::string>& params = {}
    ) = 0;

    // Parameterized INSERT returning generated primary key ID
    virtual int64_t executeInsert(
        const std::string& sql, 
        const std::vector<std::string>& params = {}
    ) = 0;

    // Parameterized UPDATE or DELETE returning number of affected rows
    virtual int executeUpdate(
        const std::string& sql, 
        const std::vector<std::string>& params = {}
    ) = 0;

    // Transaction management
    virtual bool beginTransaction() = 0;
    virtual bool commitTransaction() = 0;
    virtual bool rollbackTransaction() = 0;

    // Engine identification
    virtual bool isOracle() const = 0;
    virtual std::string getEngineName() const = 0;
    virtual std::string getLastError() const = 0;
};

// Database Factory
std::unique_ptr<IDatabase> createDatabase(const DbConfig& config);

// Schema initializer and seed loader
bool initializeDatabase(IDatabase& db);

// Helper to load config from file or environment
DbConfig loadConfig(const std::string& configFilePath = "config/cinebook.conf");

} // namespace cinebook

#endif // DATABASE_HPP
