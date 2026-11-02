#include "database.hpp"
#include "sqlite3.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <iomanip>

#ifdef USE_ORACLE
#include <occi.h>
#endif

namespace cinebook {

// ----------------------------------------------------------------------------
// Local SQLite Database Implementation
// (Enforces complete relational schema, foreign keys, constraints, transactions)
// ----------------------------------------------------------------------------
class SqliteDatabase : public IDatabase {
private:
    sqlite3* db_ = nullptr;
    std::string dbPath_;
    std::string lastError_;
    bool connected_ = false;

public:
    explicit SqliteDatabase(std::string dbPath) : dbPath_(std::move(dbPath)) {}

    ~SqliteDatabase() override {
        disconnect();
    }

    bool connect() override {
        int rc = sqlite3_open(dbPath_.c_str(), &db_);
        if (rc != SQLITE_OK) {
            lastError_ = sqlite3_errmsg(db_);
            std::cerr << "[Database Error] Failed to open SQLite DB: " << lastError_ << std::endl;
            connected_ = false;
            return false;
        }

        // Strictly enforce foreign keys in SQLite to mirror Oracle behavior
        char* errMsg = nullptr;
        rc = sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            lastError_ = errMsg ? errMsg : "Unknown error enabling foreign keys";
            sqlite3_free(errMsg);
            return false;
        }

        connected_ = true;
        std::cout << "[CineBook DB] Connected to local persistent SQL database: " << dbPath_ << std::endl;
        return true;
    }

    void disconnect() override {
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
            connected_ = false;
        }
    }

    bool isConnected() const override {
        return connected_ && db_ != nullptr;
    }

    bool execute(const std::string& sql) override {
        if (!db_) return false;
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            lastError_ = errMsg ? errMsg : "Unknown error executing statement";
            sqlite3_free(errMsg);
            std::cerr << "[Database Error] execute failed: " << lastError_ << "\nSQL: " << sql << std::endl;
            return false;
        }
        return true;
    }

    std::vector<std::map<std::string, std::string>> query(
        const std::string& sql, 
        const std::vector<std::string>& params = {}
    ) override {
        std::vector<std::map<std::string, std::string>> results;
        if (!db_) return results;

        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            lastError_ = sqlite3_errmsg(db_);
            std::cerr << "[Database Error] query prepare failed: " << lastError_ << "\nSQL: " << sql << std::endl;
            return results;
        }

        // Bind parameters safely
        for (size_t i = 0; i < params.size(); ++i) {
            sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);
        }

        int colCount = sqlite3_column_count(stmt);
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            std::map<std::string, std::string> row;
            for (int col = 0; col < colCount; ++col) {
                const char* colName = sqlite3_column_name(stmt, col);
                const unsigned char* val = sqlite3_column_text(stmt, col);
                row[colName ? colName : ""] = val ? reinterpret_cast<const char*>(val) : "";
            }
            results.push_back(std::move(row));
        }

        if (rc != SQLITE_DONE) {
            lastError_ = sqlite3_errmsg(db_);
        }

        sqlite3_finalize(stmt);
        return results;
    }

    int64_t executeInsert(
        const std::string& sql, 
        const std::vector<std::string>& params = {}
    ) override {
        if (!db_) return -1;
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            lastError_ = sqlite3_errmsg(db_);
            std::cerr << "[Database Error] insert prepare failed: " << lastError_ << "\nSQL: " << sql << std::endl;
            return -1;
        }

        for (size_t i = 0; i < params.size(); ++i) {
            sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);
        }

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE) {
            lastError_ = sqlite3_errmsg(db_);
            std::cerr << "[Database Error] insert step failed: " << lastError_ << std::endl;
            return -1;
        }

        return static_cast<int64_t>(sqlite3_last_insert_rowid(db_));
    }

    int executeUpdate(
        const std::string& sql, 
        const std::vector<std::string>& params = {}
    ) override {
        if (!db_) return -1;
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            lastError_ = sqlite3_errmsg(db_);
            std::cerr << "[Database Error] update prepare failed: " << lastError_ << "\nSQL: " << sql << std::endl;
            return -1;
        }

        for (size_t i = 0; i < params.size(); ++i) {
            sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);
        }

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE) {
            lastError_ = sqlite3_errmsg(db_);
            std::cerr << "[Database Error] update step failed: " << lastError_ << std::endl;
            return -1;
        }

        return sqlite3_changes(db_);
    }

    bool beginTransaction() override {
        return execute("BEGIN TRANSACTION;");
    }

    bool commitTransaction() override {
        return execute("COMMIT;");
    }

    bool rollbackTransaction() override {
        return execute("ROLLBACK;");
    }

    bool isOracle() const override {
        return false;
    }

    std::string getEngineName() const override {
        return "SQLite (Local Persistent Relational DB)";
    }

    std::string getLastError() const override {
        return lastError_;
    }
};

// ----------------------------------------------------------------------------
// Oracle Database Implementation (OCCI)
// ----------------------------------------------------------------------------
class OracleDatabase : public IDatabase {
private:
    DbConfig config_;
    std::string lastError_;
    bool connected_ = false;

#ifdef USE_ORACLE
    oracle::occi::Environment* env_ = nullptr;
    oracle::occi::Connection* conn_ = nullptr;
#endif

public:
    explicit OracleDatabase(DbConfig config) : config_(std::move(config)) {}

    ~OracleDatabase() override {
        disconnect();
    }

    bool connect() override {
#ifdef USE_ORACLE
        try {
            env_ = oracle::occi::Environment::createEnvironment(oracle::occi::Environment::DEFAULT);
            conn_ = env_->createConnection(
                config_.oracleUser, 
                config_.oraclePassword, 
                config_.oracleConnectString
            );
            connected_ = true;
            std::cout << "[CineBook DB] Successfully connected to Oracle Database (" 
                      << config_.oracleConnectString << ") via OCCI." << std::endl;
            return true;
        } catch (const oracle::occi::SQLException& ex) {
            lastError_ = ex.getMessage();
            std::cerr << "[Oracle DB Error] Connection failed: " << lastError_ << std::endl;
            connected_ = false;
            return false;
        }
#else
        lastError_ = "Oracle Client / OCCI libraries not compiled into current binary. To use Oracle directly, install Oracle Instant Client SDK and recompile with -DUSE_ORACLE. Falling back to local relational demonstration database.";
        std::cerr << "[Notice] " << lastError_ << std::endl;
        connected_ = false;
        return false;
#endif
    }

    void disconnect() override {
#ifdef USE_ORACLE
        if (conn_) {
            env_->terminateConnection(conn_);
            conn_ = nullptr;
        }
        if (env_) {
            oracle::occi::Environment::terminateEnvironment(env_);
            env_ = nullptr;
        }
#endif
        connected_ = false;
    }

    bool isConnected() const override {
        return connected_;
    }

    bool execute(const std::string& sql) override {
#ifdef USE_ORACLE
        if (!conn_) return false;
        try {
            oracle::occi::Statement* stmt = conn_->createStatement(sql);
            stmt->executeUpdate();
            conn_->terminateStatement(stmt);
            return true;
        } catch (const oracle::occi::SQLException& ex) {
            lastError_ = ex.getMessage();
            return false;
        }
#else
        (void)sql;
        return false;
#endif
    }

    std::vector<std::map<std::string, std::string>> query(
        const std::string& sql, 
        const std::vector<std::string>& params = {}
    ) override {
        std::vector<std::map<std::string, std::string>> results;
#ifdef USE_ORACLE
        if (!conn_) return results;
        try {
            oracle::occi::Statement* stmt = conn_->createStatement(sql);
            for (size_t i = 0; i < params.size(); ++i) {
                stmt->setString(static_cast<unsigned int>(i + 1), params[i]);
            }
            oracle::occi::ResultSet* rs = stmt->executeQuery();
            std::vector<oracle::occi::MetaData> meta = rs->getColumnListMetaData();
            while (rs->next()) {
                std::map<std::string, std::string> row;
                for (size_t col = 0; col < meta.size(); ++col) {
                    std::string colName = meta[col].getString(oracle::occi::MetaData::ATTR_NAME);
                    std::string val = rs->getString(static_cast<unsigned int>(col + 1));
                    row[colName] = val;
                }
                results.push_back(std::move(row));
            }
            stmt->closeResultSet(rs);
            conn_->terminateStatement(stmt);
        } catch (const oracle::occi::SQLException& ex) {
            lastError_ = ex.getMessage();
        }
#else
        (void)sql; (void)params;
#endif
        return results;
    }

    int64_t executeInsert(
        const std::string& sql, 
        const std::vector<std::string>& params = {}
    ) override {
#ifdef USE_ORACLE
        if (!conn_) return -1;
        try {
            oracle::occi::Statement* stmt = conn_->createStatement(sql);
            for (size_t i = 0; i < params.size(); ++i) {
                stmt->setString(static_cast<unsigned int>(i + 1), params[i]);
            }
            stmt->executeUpdate();
            conn_->terminateStatement(stmt);
            return 1;
        } catch (const oracle::occi::SQLException& ex) {
            lastError_ = ex.getMessage();
            return -1;
        }
#else
        (void)sql; (void)params;
        return -1;
#endif
    }

    int executeUpdate(
        const std::string& sql, 
        const std::vector<std::string>& params = {}
    ) override {
#ifdef USE_ORACLE
        if (!conn_) return -1;
        try {
            oracle::occi::Statement* stmt = conn_->createStatement(sql);
            for (size_t i = 0; i < params.size(); ++i) {
                stmt->setString(static_cast<unsigned int>(i + 1), params[i]);
            }
            int affected = stmt->executeUpdate();
            conn_->terminateStatement(stmt);
            return affected;
        } catch (const oracle::occi::SQLException& ex) {
            lastError_ = ex.getMessage();
            return -1;
        }
#else
        (void)sql; (void)params;
        return -1;
#endif
    }

    bool beginTransaction() override {
        // Oracle starts transactions implicitly upon first DML
        return true;
    }

    bool commitTransaction() override {
#ifdef USE_ORACLE
        if (!conn_) return false;
        try {
            conn_->commit();
            return true;
        } catch (const oracle::occi::SQLException& ex) {
            lastError_ = ex.getMessage();
            return false;
        }
#else
        return false;
#endif
    }

    bool rollbackTransaction() override {
#ifdef USE_ORACLE
        if (!conn_) return false;
        try {
            conn_->rollback();
            return true;
        } catch (const oracle::occi::SQLException& ex) {
            lastError_ = ex.getMessage();
            return false;
        }
#else
        return false;
#endif
    }

    bool isOracle() const override {
        return true;
    }

    std::string getEngineName() const override {
        return "Oracle Database (OCCI)";
    }

    std::string getLastError() const override {
        return lastError_;
    }
};

// ----------------------------------------------------------------------------
// Database Factory
// ----------------------------------------------------------------------------
std::unique_ptr<IDatabase> createDatabase(const DbConfig& config) {
    if (config.dbEngine == "oracle") {
        std::cout << "[CineBook] Attempting connection to Oracle Database (" 
                  << config.oracleConnectString << ")..." << std::endl;
        auto oracleDb = std::make_unique<OracleDatabase>(config);
        if (oracleDb->connect()) {
            return oracleDb;
        }
        std::cout << "[CineBook] Oracle client libraries or server not available in this environment.\n"
                  << "          --> Swapping to local persistent SQL engine (" << config.localDbPath 
                  << ") for local demonstration.\n"
                  << "          --> Real Oracle SQL schema and sample scripts remain available in 'database/' folder." << std::endl;
    }

    auto sqliteDb = std::make_unique<SqliteDatabase>(config.localDbPath);
    sqliteDb->connect();
    return sqliteDb;
}

// ----------------------------------------------------------------------------
// Configuration File / Environment Variable Loader
// ----------------------------------------------------------------------------
DbConfig loadConfig(const std::string& configFilePath) {
    DbConfig cfg;

    std::ifstream file(configFilePath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = line.substr(0, eqPos);
                std::string val = line.substr(eqPos + 1);
                while (!key.empty() && (key.back() == ' ' || key.back() == '\r')) key.pop_back();
                while (!val.empty() && (val.back() == ' ' || val.back() == '\r')) val.pop_back();
                while (!key.empty() && key.front() == ' ') key.erase(0, 1);
                while (!val.empty() && val.front() == ' ') val.erase(0, 1);

                if (key == "SERVER_HOST") cfg.serverHost = val;
                else if (key == "SERVER_PORT") cfg.serverPort = std::stoi(val);
                else if (key == "DB_ENGINE") cfg.dbEngine = val;
                else if (key == "ORACLE_USER") cfg.oracleUser = val;
                else if (key == "ORACLE_PASSWORD") cfg.oraclePassword = val;
                else if (key == "ORACLE_CONNECT_STRING") cfg.oracleConnectString = val;
                else if (key == "LOCAL_DB_PATH") cfg.localDbPath = val;
                else if (key == "ADMIN_AUTH_TOKEN") cfg.adminAuthToken = val;
            }
        }
    }

    // Override with environment variables if present
    if (const char* envHost = std::getenv("SERVER_HOST")) cfg.serverHost = envHost;
    if (const char* envPort = std::getenv("SERVER_PORT")) cfg.serverPort = std::stoi(envPort);
    if (const char* envEngine = std::getenv("DB_ENGINE")) cfg.dbEngine = envEngine;
    if (const char* envOUser = std::getenv("ORACLE_USER")) cfg.oracleUser = envOUser;
    if (const char* envOPass = std::getenv("ORACLE_PASSWORD")) cfg.oraclePassword = envOPass;
    if (const char* envOConn = std::getenv("ORACLE_CONNECT_STRING")) cfg.oracleConnectString = envOConn;
    if (const char* envLPath = std::getenv("LOCAL_DB_PATH")) cfg.localDbPath = envLPath;
    if (const char* envAuth = std::getenv("ADMIN_AUTH_TOKEN")) cfg.adminAuthToken = envAuth;

    return cfg;
}

// ----------------------------------------------------------------------------
// Schema Initialization and Data Seeding
// ----------------------------------------------------------------------------
bool initializeDatabase(IDatabase& db) {
    // 1. Create USERS table
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS USERS (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL,
            email TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            role TEXT NOT NULL DEFAULT 'user' CHECK (role IN ('user', 'admin')),
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )");

    // 2. Create MOVIES table
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS MOVIES (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            description TEXT,
            genre TEXT NOT NULL,
            language TEXT NOT NULL,
            duration_mins INTEGER NOT NULL CHECK (duration_mins > 0),
            release_date TEXT NOT NULL,
            certificate TEXT DEFAULT 'UA' NOT NULL,
            rating REAL DEFAULT 0.0 CHECK (rating >= 0.0 AND rating <= 10.0),
            cast_details TEXT,
            director TEXT,
            poster_url TEXT,
            banner_url TEXT,
            is_upcoming INTEGER DEFAULT 0 CHECK (is_upcoming IN (0, 1)),
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )");

    // 3. Create THEATRES table
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS THEATRES (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            location TEXT NOT NULL,
            city TEXT NOT NULL
        );
    )");

    // 4. Create SCREENS table
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS SCREENS (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            theatre_id INTEGER NOT NULL,
            screen_name TEXT NOT NULL,
            total_seats INTEGER NOT NULL CHECK (total_seats > 0),
            FOREIGN KEY (theatre_id) REFERENCES THEATRES (id) ON DELETE CASCADE
        );
    )");

    // 5. Create SEATS table
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS SEATS (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            screen_id INTEGER NOT NULL,
            seat_number TEXT NOT NULL,
            row_name TEXT NOT NULL,
            category TEXT DEFAULT 'Regular' CHECK (category IN ('Regular', 'Premium')),
            base_price REAL NOT NULL CHECK (base_price > 0),
            UNIQUE(screen_id, seat_number),
            FOREIGN KEY (screen_id) REFERENCES SCREENS (id) ON DELETE CASCADE
        );
    )");

    // 6. Create SHOWS table
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS SHOWS (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            movie_id INTEGER NOT NULL,
            screen_id INTEGER NOT NULL,
            show_date TEXT NOT NULL,
            show_time TEXT NOT NULL,
            UNIQUE(screen_id, show_date, show_time),
            FOREIGN KEY (movie_id) REFERENCES MOVIES (id) ON DELETE CASCADE,
            FOREIGN KEY (screen_id) REFERENCES SCREENS (id) ON DELETE CASCADE
        );
    )");

    // 7. Create BOOKINGS table
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS BOOKINGS (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            booking_reference TEXT UNIQUE NOT NULL,
            user_id INTEGER NOT NULL,
            show_id INTEGER NOT NULL,
            total_amount REAL NOT NULL CHECK (total_amount >= 0),
            ticket_count INTEGER NOT NULL CHECK (ticket_count > 0),
            payment_status TEXT DEFAULT 'PENDING' CHECK (payment_status IN ('PENDING', 'SUCCESS', 'FAILED', 'REFUNDED')),
            booking_status TEXT DEFAULT 'CONFIRMED' CHECK (booking_status IN ('CONFIRMED', 'CANCELLED')),
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES USERS (id) ON DELETE RESTRICT,
            FOREIGN KEY (show_id) REFERENCES SHOWS (id) ON DELETE RESTRICT
        );
    )");

    // 8. Create BOOKING_SEATS table
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS BOOKING_SEATS (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            booking_id INTEGER NOT NULL,
            show_id INTEGER NOT NULL,
            seat_id INTEGER NOT NULL,
            price_snapshot REAL NOT NULL CHECK (price_snapshot >= 0),
            status TEXT DEFAULT 'BOOKED' CHECK (status IN ('BOOKED', 'CANCELLED')),
            FOREIGN KEY (booking_id) REFERENCES BOOKINGS (id) ON DELETE CASCADE,
            FOREIGN KEY (show_id) REFERENCES SHOWS (id) ON DELETE CASCADE,
            FOREIGN KEY (seat_id) REFERENCES SEATS (id) ON DELETE RESTRICT
        );
    )");

    // Double-booking prevention index: unique active seat per show!
    db.execute(R"(
        CREATE UNIQUE INDEX IF NOT EXISTS UQ_ACTIVE_SHOW_SEAT 
        ON BOOKING_SEATS(show_id, seat_id) 
        WHERE status = 'BOOKED';
    )");

    // Foreign key and lookup performance indexes
    db.execute("CREATE INDEX IF NOT EXISTS IDX_SCREENS_THEATRE ON SCREENS(theatre_id);");
    db.execute("CREATE INDEX IF NOT EXISTS IDX_SEATS_SCREEN ON SEATS(screen_id);");
    db.execute("CREATE INDEX IF NOT EXISTS IDX_SHOWS_MOVIE ON SHOWS(movie_id);");
    db.execute("CREATE INDEX IF NOT EXISTS IDX_SHOWS_SCREEN ON SHOWS(screen_id);");
    db.execute("CREATE INDEX IF NOT EXISTS IDX_SHOWS_DATE ON SHOWS(show_date);");
    db.execute("CREATE INDEX IF NOT EXISTS IDX_BOOKINGS_USER ON BOOKINGS(user_id);");
    db.execute("CREATE INDEX IF NOT EXISTS IDX_BOOKINGS_SHOW ON BOOKINGS(show_id);");
    db.execute("CREATE INDEX IF NOT EXISTS IDX_BOOKINGS_REF ON BOOKINGS(booking_reference);");
    db.execute("CREATE INDEX IF NOT EXISTS IDX_BSEATS_BOOKING ON BOOKING_SEATS(booking_id);");

    // Check if database needs seeding
    auto check = db.query("SELECT COUNT(*) AS count FROM MOVIES;");
    if (!check.empty() && check[0]["count"] != "0") {
        std::cout << "[CineBook DB] Schema verified. Found " << check[0]["count"] << " movies already in database." << std::endl;
        return true;
    }

    std::cout << "[CineBook DB] Seeding initial data for CineBook mini-project..." << std::endl;

    // Seed Users
    db.execute(R"(
        INSERT INTO USERS (id, username, email, password_hash, role) VALUES
        (1, 'admin', 'admin@cinebook.com', '240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9', 'admin'),
        (2, 'rahul_sharma', 'rahul@example.com', 'e606e38b0d8c19b24cf0ee3808183162ea7cd63ff7912dbb22b5e803286b4446', 'user'),
        (3, 'priya_patel', 'priya@example.com', 'e606e38b0d8c19b24cf0ee3808183162ea7cd63ff7912dbb22b5e803286b4446', 'user');
    )");

    // Seed Movies
    db.execute(R"(
        INSERT INTO MOVIES (id, title, description, genre, language, duration_mins, release_date, certificate, rating, cast_details, director, poster_url, banner_url, is_upcoming) VALUES
        (1, 'Dune: Part Two', 'Paul Atreides unites with Chani and the Fremen while seeking revenge against the conspirators who destroyed his family.', 'Sci-Fi', 'English', 166, '2024-03-01', 'UA', 8.6, 'Timothée Chalamet, Zendaya, Rebecca Ferguson', 'Denis Villeneuve', 'https://images.unsplash.com/photo-1534447677768-be436bb09401?w=600&auto=format&fit=crop&q=80', 'https://images.unsplash.com/photo-1509198397868-475647b2a1e5?w=1200&auto=format&fit=crop&q=80', 0),
        (2, 'Oppenheimer', 'The story of American scientist J. Robert Oppenheimer and his role in the development of the atomic bomb during World War II.', 'Biography', 'English', 180, '2023-07-21', 'A', 8.9, 'Cillian Murphy, Emily Blunt, Matt Damon', 'Christopher Nolan', 'https://images.unsplash.com/photo-1440404653325-ab127d49abc1?w=600&auto=format&fit=crop&q=80', 'https://images.unsplash.com/photo-1478760329108-5c3ed9d495a0?w=1200&auto=format&fit=crop&q=80', 0),
        (3, 'Inception', 'A thief who steals corporate secrets through dream-sharing technology is given the inverse task of planting an idea.', 'Sci-Fi', 'English', 148, '2010-07-16', 'UA', 8.8, 'Leonardo DiCaprio, Joseph Gordon-Levitt, Elliot Page', 'Christopher Nolan', 'https://images.unsplash.com/photo-1489599849927-2ee91cede3ba?w=600&auto=format&fit=crop&q=80', 'https://images.unsplash.com/photo-1518709268805-4e9042af9f23?w=1200&auto=format&fit=crop&q=80', 0),
        (4, 'RRR', 'A fearless warrior on a perilous mission confronts a charismatic British police officer in 1920s pre-independent India.', 'Action', 'Telugu', 187, '2022-03-25', 'UA', 8.1, 'N.T. Rama Rao Jr., Ram Charan, Alia Bhatt', 'S.S. Rajamouli', 'https://images.unsplash.com/photo-1579783902614-a3fb3927b675?w=600&auto=format&fit=crop&q=80', 'https://images.unsplash.com/photo-1514306191717-452ec28c7814?w=1200&auto=format&fit=crop&q=80', 0),
        (5, 'Spider-Man: Across the Spider-Verse', 'Miles Morales catapults across the Multiverse, where he encounters a team of Spider-People.', 'Animation', 'English', 140, '2023-06-02', 'U', 8.7, 'Shameik Moore, Hailee Steinfeld, Oscar Isaac', 'Joaquim Dos Santos', 'https://images.unsplash.com/photo-1635805737707-575885ab0820?w=600&auto=format&fit=crop&q=80', 'https://images.unsplash.com/photo-1534447677768-be436bb09401?w=1200&auto=format&fit=crop&q=80', 0),
        (6, 'Interstellar', 'A team of explorers travel through a wormhole in space in an attempt to ensure humanity survival.', 'Sci-Fi', 'English', 169, '2014-11-07', 'UA', 8.7, 'Matthew McConaughey, Anne Hathaway, Jessica Chastain', 'Christopher Nolan', 'https://images.unsplash.com/photo-1451187580459-43490279c0fa?w=600&auto=format&fit=crop&q=80', 'https://images.unsplash.com/photo-1446776811953-b23d57bd21aa?w=1200&auto=format&fit=crop&q=80', 0),
        (7, 'Doraemon: Nobita''s Sky Utopia', 'Nobita and Doraemon embark on an extraordinary journey through time and space in a time airship to find Utopia, a legendary crescent-shaped island in the sky.', 'Animation', 'Japanese', 108, '2023-03-03', 'U', 8.2, 'Wasabi Mizuta, Megumi Oohara, Yumi Kakazu, Tomokazu Seki', 'Takumi Doyama', 'https://images.unsplash.com/photo-1578632767115-351597cf2477?w=600&auto=format&fit=crop&q=80', 'https://images.unsplash.com/photo-1534447677768-be436bb09401?w=1200&auto=format&fit=crop&q=80', 0),
        (8, 'Jailer 2', 'Tiger Muthuvel Pandian returns in this high-octane action thriller, facing a dangerous international syndicate that threatens law and order.', 'Action', 'Tamil', 168, '2025-08-10', 'UA', 8.9, 'Rajinikanth, Mohanlal, Shiva Rajkumar, Ramya Krishnan', 'Nelson Dilipkumar', 'https://images.unsplash.com/photo-1514306191717-452ec28c7814?w=600&auto=format&fit=crop&q=80', 'https://images.unsplash.com/photo-1509198397868-475647b2a1e5?w=1200&auto=format&fit=crop&q=80', 0);
    )");

    // Seed Theatres
    db.execute(R"(
        INSERT INTO THEATRES (id, name, location, city) VALUES
        (1, 'PVR Superplex Mall of India', 'Sector 18', 'Noida'),
        (2, 'INOX Megaplex Forum', 'Koramangala', 'Bengaluru'),
        (3, 'Cinepolis Grand Central', 'Seawoods', 'Navi Mumbai');
    )");

    // Seed Screens
    db.execute(R"(
        INSERT INTO SCREENS (id, theatre_id, screen_name, total_seats) VALUES
        (1, 1, 'Screen 1 (IMAX Laser)', 25),
        (2, 1, 'Screen 2 (Dolby Atmos)', 25),
        (3, 2, 'Audi 1 (Koramangala 4DX)', 25);
    )");

    // Seed Seats for Screens 1, 2, 3
    for (int screenId = 1; screenId <= 3; ++screenId) {
        char rows[] = {'A', 'B', 'C', 'D', 'E'};
        for (char r : rows) {
            std::string cat = (r == 'D' || r == 'E') ? "Premium" : "Regular";
            double price = (r == 'E') ? 350.0 : (r == 'D' ? 300.0 : 180.0);
            for (int num = 1; num <= 5; ++num) {
                std::string seatNum = std::string(1, r) + std::to_string(num);
                std::string insertSeatSql = "INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (" 
                    + std::to_string(screenId) + ", '" + seatNum + "', '" + std::string(1, r) + "', '" + cat + "', " + std::to_string(price) + ");";
                db.execute(insertSeatSql);
            }
        }
    }

    // Dynamic dates for shows (today, tomorrow, day after)
    auto now = std::chrono::system_clock::now();
    std::time_t t_now = std::chrono::system_clock::to_time_t(now);
    std::time_t t_tomorrow = t_now + 86400;
    std::time_t t_dayafter = t_now + 172800;

    auto formatDate = [](std::time_t t) {
        std::tm tm_buf;
#if defined(_WIN32)
        localtime_s(&tm_buf, &t);
#else
        localtime_r(&t, &tm_buf);
#endif
        std::ostringstream ss;
        ss << std::put_time(&tm_buf, "%Y-%m-%d");
        return ss.str();
    };

    std::string dToday = formatDate(t_now);
    std::string dTomorrow = formatDate(t_tomorrow);
    std::string dDayAfter = formatDate(t_dayafter);

    // Seed Shows
    std::vector<std::string> showInserts = {
        // Today
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (1, 1, 1, '" + dToday + "', '10:30');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (2, 1, 1, '" + dToday + "', '14:30');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (3, 1, 1, '" + dToday + "', '18:45');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (4, 2, 2, '" + dToday + "', '11:00');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (5, 2, 2, '" + dToday + "', '15:30');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (10, 3, 1, '" + dToday + "', '22:00');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (11, 6, 2, '" + dToday + "', '19:30');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (12, 4, 3, '" + dToday + "', '12:00');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (13, 5, 3, '" + dToday + "', '16:30');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (14, 4, 3, '" + dToday + "', '20:30');",

        // Tomorrow
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (6, 3, 1, '" + dTomorrow + "', '14:00');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (7, 4, 3, '" + dTomorrow + "', '18:00');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (8, 5, 1, '" + dTomorrow + "', '20:30');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (15, 2, 1, '" + dTomorrow + "', '10:30');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (16, 1, 1, '" + dTomorrow + "', '17:30');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (17, 4, 2, '" + dTomorrow + "', '11:30');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (18, 6, 2, '" + dTomorrow + "', '15:30');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (19, 2, 2, '" + dTomorrow + "', '19:45');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (20, 5, 3, '" + dTomorrow + "', '13:00');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (21, 3, 3, '" + dTomorrow + "', '22:00');",

        // Day After
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (9, 6, 2, '" + dDayAfter + "', '17:15');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (22, 1, 1, '" + dDayAfter + "', '11:00');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (23, 3, 1, '" + dDayAfter + "', '15:00');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (24, 4, 1, '" + dDayAfter + "', '19:00');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (25, 5, 2, '" + dDayAfter + "', '12:30');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (26, 2, 2, '" + dDayAfter + "', '21:00');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (27, 6, 3, '" + dDayAfter + "', '14:00');",
        "INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time) VALUES (28, 1, 3, '" + dDayAfter + "', '18:30');"
    };

    for (const auto& sql : showInserts) {
        db.execute(sql);
    }

    // Seed Sample Bookings
    db.execute(R"(
        INSERT INTO BOOKINGS (id, booking_reference, user_id, show_id, total_amount, ticket_count, payment_status, booking_status) VALUES
        (1, 'CB-20261014-8801', 2, 1, 360.00, 2, 'SUCCESS', 'CONFIRMED'),
        (2, 'CB-20261014-9914', 3, 1, 650.00, 2, 'SUCCESS', 'CONFIRMED');
    )");

    db.execute(R"(
        INSERT INTO BOOKING_SEATS (booking_id, show_id, seat_id, price_snapshot, status) VALUES
        (1, 1, 1, 180.00, 'BOOKED'),
        (1, 1, 2, 180.00, 'BOOKED'),
        (2, 1, 16, 300.00, 'BOOKED'),
        (2, 1, 21, 350.00, 'BOOKED');
    )");

    std::cout << "[CineBook DB] Database seeded successfully." << std::endl;
    return true;
}

} // namespace cinebook
