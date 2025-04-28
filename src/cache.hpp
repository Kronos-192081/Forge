#ifndef CACHE_HPP
#define CACHE_HPP

#include <sqlite3.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

/**
 * @brief A class to manage a file cache using an SQLite database.
 *
 * The Cache class provides functionality to store file hashes in an SQLite database,
 * compute file hashes using SHA256, and check if a file's hash matches the stored hash.
 */
class Cache {
private:
    sqlite3* db; ///< Pointer to the SQLite database connection.
    std::string db_path; ///< Path to the SQLite database file.

    /**
     * @brief Computes the SHA256 hash of a file.
     *
     * @param file_path The path to the file whose hash is to be computed.
     * @return The computed SHA256 hash as a hexadecimal string.
     * @throws std::runtime_error If the file cannot be opened.
     */
    std::string computeHash(const std::string& file_path) {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Unable to open file: " + file_path);
        }

        SHA256_CTX sha256;
        SHA256_Init(&sha256);

        char buffer[8192];
        while (file.read(buffer, sizeof(buffer))) {
            SHA256_Update(&sha256, buffer, file.gcount());
        }
        SHA256_Update(&sha256, buffer, file.gcount());

        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_Final(hash, &sha256);

        std::ostringstream oss;
        for (unsigned char c : hash) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
        return oss.str();
    }

    /**
     * @brief Initializes the SQLite database and creates the required table if it does not exist.
     *
     * @throws std::runtime_error If the table creation fails.
     */
    void initializeDB() {
        const char* create_table_query = R"(
            CREATE TABLE IF NOT EXISTS cache (
                file_addr TEXT PRIMARY KEY,
                file_hash TEXT
            );
        )";

        char* err_msg = nullptr;
        if (sqlite3_exec(db, create_table_query, nullptr, nullptr, &err_msg) != SQLITE_OK) {
            std::string error = "Failed to create table: ";
            error += err_msg;
            sqlite3_free(err_msg);
            throw std::runtime_error(error);
        }
    }

public:
    /**
     * @brief Constructs a Cache object and initializes the SQLite database.
     *
     * The database is created in the user's home directory as `.forgecache` if it does not already exist.
     *
     * @throws std::runtime_error If the HOME environment variable is not set or the database cannot be opened.
     */
    Cache() {
        const char* home = std::getenv("HOME");
        if (!home) {
            throw std::runtime_error("HOME environment variable not set");
        }

        db_path = std::string(home) + "/.forgecache";

        if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Failed to open SQLite database");
        }

        initializeDB();
    }

    /**
     * @brief Destroys the Cache object and closes the SQLite database connection.
     */
    ~Cache() {
        if (db) {
            sqlite3_close(db);
        }
    }

    /**
     * @brief Adds or updates a file's hash in the cache.
     *
     * @param file_addr The path to the file to be added or updated in the cache.
     * @throws std::runtime_error If the insert operation fails.
     */
    void add(const std::string& file_addr) {
        std::string file_hash = computeHash(file_addr);

        const char* insert_query = R"(
            INSERT OR REPLACE INTO cache (file_addr, file_hash)
            VALUES (?, ?);
        )";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, insert_query, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare insert statement");
        }

        sqlite3_bind_text(stmt, 1, file_addr.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, file_hash.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw std::runtime_error("Failed to execute insert statement");
        }

        sqlite3_finalize(stmt);
    }

    /**
     * @brief Finds the hash of a file in the cache.
     *
     * @param file_addr The path to the file whose hash is to be retrieved.
     * @return The hash of the file as a string, or an empty string if the file is not in the cache.
     * @throws std::runtime_error If the select operation fails.
     */
    std::string find(const std::string& file_addr) {
        const char* select_query = R"(
            SELECT file_hash FROM cache WHERE file_addr = ?;
        )";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, select_query, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare select statement");
        }

        sqlite3_bind_text(stmt, 1, file_addr.c_str(), -1, SQLITE_STATIC);

        std::string file_hash;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            file_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        }

        sqlite3_finalize(stmt);
        return file_hash;
    }

    /**
     * @brief Checks if a file's current hash matches the hash stored in the cache.
     *
     * @param file_addr The path to the file to be checked.
     * @return True if the file's current hash matches the stored hash, false otherwise.
     */
    bool check(const std::string& file_addr) {
        std::string file_hash = find(file_addr);
        if (file_hash.empty()) {
            return false;
        }

        std::string current_hash = computeHash(file_addr);
        return file_hash == current_hash;
    }
};

#endif // CACHE_HPP