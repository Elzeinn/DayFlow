#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include "core/error.hpp"
#include "core/timestamp.hpp"

struct sqlite3;
struct sqlite3_stmt;

namespace dayflow {

inline constexpr int k_schema_version = 1;

class Statement {
public:
    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    Error bind(int index, std::int64_t value);
    Error bind(int index, int value);
    Error bind(int index, std::string_view value);
    Error bind(int index, const std::optional<Timestamp>& value);
    Error bind_null(int index);

    Result<bool> step();
    void reset();

    std::int64_t column_int64(int index);
    int column_int(int index);
    std::string column_text(int index);
    Result<std::optional<Timestamp>> column_optional_timestamp(int index);

private:
    friend class Database;
    Statement(sqlite3* db, sqlite3_stmt* stmt) : db_(db), stmt_(stmt) {}

    Error bind_error() const;

    sqlite3* db_ = nullptr;
    sqlite3_stmt* stmt_ = nullptr;
};

class Database {
public:
    static Result<std::unique_ptr<Database>> open(const std::filesystem::path& path);
    static Result<std::unique_ptr<Database>> open_in_memory();

    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    Error exec(std::string_view sql);
    Result<Statement> prepare(std::string_view sql);

    Result<int> user_version();
    Error set_user_version(int version);

    Result<std::int64_t> last_insert_rowid();
    int changes() const;

    std::unique_lock<std::mutex> lock();

private:
    Database() = default;

    sqlite3* handle_ = nullptr;
    std::mutex mutex_;
};

}  // namespace dayflow
