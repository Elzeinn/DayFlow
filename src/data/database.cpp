#include "data/database.hpp"

#include <cstdio>
#include <system_error>
#include <utility>

#include <sqlite3.h>

namespace dayflow {
namespace {

constexpr char k_schema_v1[] = R"sql(
CREATE TABLE event_categories (
    id          INTEGER PRIMARY KEY,
    name        TEXT NOT NULL UNIQUE,
    color       TEXT NOT NULL,
    sort_order  INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE recurrence_rules (
    id          INTEGER PRIMARY KEY,
    freq        TEXT NOT NULL CHECK (freq IN ('daily','weekly','monthly','yearly')),
    interval    INTEGER NOT NULL DEFAULT 1 CHECK (interval >= 1),
    by_weekday  TEXT,
    by_monthday TEXT,
    count       INTEGER,
    until_utc   TEXT
);

CREATE TABLE events (
    id            INTEGER PRIMARY KEY,
    title         TEXT NOT NULL,
    description   TEXT,
    category_id   INTEGER REFERENCES event_categories(id) ON DELETE SET NULL,
    starts_at_utc TEXT NOT NULL,
    ends_at_utc   TEXT NOT NULL,
    all_day       INTEGER NOT NULL DEFAULT 0,
    recurrence_id INTEGER REFERENCES recurrence_rules(id) ON DELETE CASCADE,
    created_at    TEXT NOT NULL,
    updated_at    TEXT NOT NULL,
    CHECK (ends_at_utc >= starts_at_utc)
);

CREATE TABLE tasks (
    id           INTEGER PRIMARY KEY,
    title        TEXT NOT NULL,
    notes        TEXT,
    priority     INTEGER NOT NULL DEFAULT 1 CHECK (priority BETWEEN 0 AND 3),
    status       INTEGER NOT NULL DEFAULT 0 CHECK (status BETWEEN 0 AND 3),
    due_at_utc   TEXT,
    completed_at TEXT,
    sort_order   INTEGER NOT NULL DEFAULT 0,
    created_at   TEXT NOT NULL,
    updated_at   TEXT NOT NULL
);

CREATE TABLE subtasks (
    id         INTEGER PRIMARY KEY,
    task_id    INTEGER NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,
    title      TEXT NOT NULL,
    done       INTEGER NOT NULL DEFAULT 0,
    sort_order INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE routines (
    id         INTEGER PRIMARY KEY,
    name       TEXT NOT NULL,
    active     INTEGER NOT NULL DEFAULT 1,
    created_at TEXT NOT NULL
);

CREATE TABLE routine_items (
    id               INTEGER PRIMARY KEY,
    routine_id       INTEGER NOT NULL REFERENCES routines(id) ON DELETE CASCADE,
    title            TEXT NOT NULL,
    weekday_mask     INTEGER NOT NULL DEFAULT 127,
    start_minute     INTEGER NOT NULL CHECK (start_minute BETWEEN 0 AND 1439),
    duration_minutes INTEGER NOT NULL CHECK (duration_minutes > 0),
    sort_order       INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE routine_logs (
    id            INTEGER PRIMARY KEY,
    item_id       INTEGER NOT NULL REFERENCES routine_items(id) ON DELETE CASCADE,
    local_date    TEXT NOT NULL,
    status        INTEGER NOT NULL,
    snoozed_until TEXT,
    UNIQUE (item_id, local_date)
);

CREATE TABLE habits (
    id         INTEGER PRIMARY KEY,
    name       TEXT NOT NULL,
    active     INTEGER NOT NULL DEFAULT 1,
    created_at TEXT NOT NULL
);

CREATE TABLE habit_logs (
    id         INTEGER PRIMARY KEY,
    habit_id   INTEGER NOT NULL REFERENCES habits(id) ON DELETE CASCADE,
    local_date TEXT NOT NULL,
    UNIQUE (habit_id, local_date)
);

CREATE TABLE reminders (
    id            INTEGER PRIMARY KEY,
    subject       TEXT NOT NULL,
    fire_at_utc   TEXT NOT NULL,
    source_kind   TEXT NOT NULL CHECK (source_kind IN ('task','event','routine','habit','custom')),
    source_id     INTEGER,
    status        INTEGER NOT NULL DEFAULT 0,
    snoozed_until TEXT,
    fired_at_utc  TEXT,
    created_at    TEXT NOT NULL
);

CREATE TABLE notes (
    id         INTEGER PRIMARY KEY,
    body       TEXT NOT NULL,
    pinned     INTEGER NOT NULL DEFAULT 0,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE TABLE settings (
    key   TEXT PRIMARY KEY,
    value TEXT NOT NULL
);

CREATE INDEX idx_tasks_due        ON tasks (status, due_at_utc);
CREATE INDEX idx_subtasks_task    ON subtasks (task_id);
CREATE INDEX idx_events_start     ON events (starts_at_utc);
CREATE INDEX idx_routine_items_r  ON routine_items (routine_id, weekday_mask);
CREATE INDEX idx_routine_logs_day ON routine_logs (local_date);
CREATE INDEX idx_habit_logs_day   ON habit_logs (local_date);
CREATE INDEX idx_reminders_due    ON reminders (status, fire_at_utc);
)sql";

Error apply_migrations(Database& database)
{
    auto version = database.user_version();
    if (!version.ok()) {
        return version.error();
    }
    if (version.value() > k_schema_version) {
        return make_error(ErrorCode::database, "database dibuat oleh versi DayFlow yang lebih baru");
    }
    if (version.value() < 1) {
        if (Error error = database.exec(k_schema_v1)) {
            return error;
        }
        if (Error error = database.set_user_version(1)) {
            return error;
        }
    }
    return {};
}

}  // namespace

std::string like_pattern(std::string_view text)
{
    std::string pattern{"%"};
    for (const char character : text) {
        if (character == '\\' || character == '%' || character == '_') {
            pattern += '\\';
        }
        pattern += character;
    }
    pattern += '%';
    return pattern;
}

Result<std::unique_ptr<Database>> Database::open(const std::filesystem::path& path)
{
    if (path != ":memory:") {
        const std::filesystem::path parent = path.parent_path();
        if (!parent.empty()) {
            std::error_code code;
            std::filesystem::create_directories(parent, code);
            if (code) {
                return make_error(ErrorCode::io,
                    "tidak bisa membuat folder " + parent.string() + ": " + code.message());
            }
        }
    }

    auto database = std::unique_ptr<Database>(new Database());
    const int result = sqlite3_open_v2(
        path.string().c_str(), &database->handle_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (result != SQLITE_OK) {
        const std::string message = database->handle_ ? sqlite3_errmsg(database->handle_)
                                                      : "gagal membuka database";
        return make_error(ErrorCode::database, message + " (" + path.string() + ")");
    }

    for (const char* pragma :
        {"PRAGMA foreign_keys = ON", "PRAGMA journal_mode = WAL", "PRAGMA synchronous = NORMAL",
            "PRAGMA busy_timeout = 5000"}) {
        if (Error error = database->exec(pragma)) {
            return error;
        }
    }

    if (Error error = apply_migrations(*database)) {
        return error;
    }
    return std::move(database);
}

Result<std::unique_ptr<Database>> Database::open_in_memory()
{
    return open(":memory:");
}

Database::~Database()
{
    if (handle_ != nullptr) {
        sqlite3_close(handle_);
    }
}

Error Database::exec(std::string_view sql)
{
    const std::string query{sql};
    char* message = nullptr;
    const int result = sqlite3_exec(handle_, query.c_str(), nullptr, nullptr, &message);
    if (result != SQLITE_OK) {
        const std::string text = message != nullptr ? message : sqlite3_errmsg(handle_);
        sqlite3_free(message);
        return make_error(ErrorCode::database, text);
    }
    return {};
}

Result<Statement> Database::prepare(std::string_view sql)
{
    const std::string query{sql};
    sqlite3_stmt* statement = nullptr;
    const int result = sqlite3_prepare_v2(handle_, query.c_str(), static_cast<int>(query.size()), &statement, nullptr);
    if (result != SQLITE_OK) {
        return make_error(ErrorCode::database, sqlite3_errmsg(handle_));
    }
    return Statement(handle_, statement);
}

Result<int> Database::user_version()
{
    auto statement = prepare("PRAGMA user_version");
    if (!statement.ok()) {
        return statement.error();
    }
    auto row = statement.value().step();
    if (!row.ok()) {
        return row.error();
    }
    if (!row.value()) {
        return make_error(ErrorCode::database, "PRAGMA user_version tidak mengembalikan baris");
    }
    return statement.value().column_int(0);
}

Error Database::set_user_version(int version)
{
    char sql[32];
    std::snprintf(sql, sizeof sql, "PRAGMA user_version = %d", version);
    return exec(sql);
}

Result<std::int64_t> Database::last_insert_rowid()
{
    return static_cast<std::int64_t>(sqlite3_last_insert_rowid(handle_));
}

int Database::changes() const
{
    return sqlite3_changes(handle_);
}

Error Database::backup_to(const std::filesystem::path& path)
{
    std::error_code code;
    std::filesystem::remove(path, code);

    std::string escaped;
    for (const char character : path.string()) {
        if (character == '\'') {
            escaped += '\'';
        }
        escaped += character;
    }

    auto guard = lock();
    return exec("VACUUM INTO '" + escaped + "'");
}

std::unique_lock<std::mutex> Database::lock()
{
    return std::unique_lock<std::mutex>{mutex_};
}

Statement::Statement(Statement&& other) noexcept : db_(other.db_), stmt_(other.stmt_)
{
    other.db_ = nullptr;
    other.stmt_ = nullptr;
}

Statement& Statement::operator=(Statement&& other) noexcept
{
    if (this != &other) {
        if (stmt_ != nullptr) {
            sqlite3_finalize(stmt_);
        }
        db_ = other.db_;
        stmt_ = other.stmt_;
        other.db_ = nullptr;
        other.stmt_ = nullptr;
    }
    return *this;
}

Statement::~Statement()
{
    if (stmt_ != nullptr) {
        sqlite3_finalize(stmt_);
    }
}

Error Statement::bind_error() const
{
    return make_error(ErrorCode::database, db_ != nullptr ? sqlite3_errmsg(db_) : "gagal bind parameter");
}

Error Statement::bind(int index, std::int64_t value)
{
    if (sqlite3_bind_int64(stmt_, index, value) != SQLITE_OK) {
        return bind_error();
    }
    return {};
}

Error Statement::bind(int index, int value)
{
    return bind(index, static_cast<std::int64_t>(value));
}

Error Statement::bind(int index, std::string_view value)
{
    if (sqlite3_bind_text(stmt_, index, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
        return bind_error();
    }
    return {};
}

Error Statement::bind(int index, const std::optional<Timestamp>& value)
{
    if (!value.has_value()) {
        return bind_null(index);
    }
    return bind(index, to_iso8601(*value));
}

Error Statement::bind_null(int index)
{
    if (sqlite3_bind_null(stmt_, index) != SQLITE_OK) {
        return bind_error();
    }
    return {};
}

Result<bool> Statement::step()
{
    const int result = sqlite3_step(stmt_);
    if (result == SQLITE_ROW) {
        return true;
    }
    if (result == SQLITE_DONE) {
        return false;
    }
    return make_error(ErrorCode::database, db_ != nullptr ? sqlite3_errmsg(db_) : "gagal menjalankan query");
}

void Statement::reset()
{
    sqlite3_reset(stmt_);
    sqlite3_clear_bindings(stmt_);
}

std::int64_t Statement::column_int64(int index)
{
    return sqlite3_column_int64(stmt_, index);
}

int Statement::column_int(int index)
{
    return sqlite3_column_int(stmt_, index);
}

std::string Statement::column_text(int index)
{
    const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, index));
    if (text == nullptr) {
        return {};
    }
    return std::string{text, static_cast<std::size_t>(sqlite3_column_bytes(stmt_, index))};
}

bool Statement::is_null(int index)
{
    return sqlite3_column_type(stmt_, index) == SQLITE_NULL;
}

std::optional<int> Statement::column_optional_int(int index)
{
    if (is_null(index)) {
        return std::nullopt;
    }
    return sqlite3_column_int(stmt_, index);
}

std::optional<std::int64_t> Statement::column_optional_int64(int index)
{
    if (is_null(index)) {
        return std::nullopt;
    }
    return sqlite3_column_int64(stmt_, index);
}

std::optional<std::string> Statement::column_optional_text(int index)
{
    if (is_null(index)) {
        return std::nullopt;
    }
    return column_text(index);
}

Result<std::optional<Timestamp>> Statement::column_optional_timestamp(int index)
{
    if (is_null(index)) {
        return std::optional<Timestamp>{};
    }
    const std::string text = column_text(index);
    auto parsed = parse_iso8601(text);
    if (!parsed.ok()) {
        return parsed.error();
    }
    return std::optional<Timestamp>{parsed.value()};
}

Transaction::Transaction(Database& database) : database_(database)
{
    database_.exec("BEGIN IMMEDIATE");
}

Transaction::~Transaction()
{
    if (active_) {
        database_.exec("ROLLBACK");
    }
}

Error Transaction::commit()
{
    if (Error error = database_.exec("COMMIT")) {
        return error;
    }
    active_ = false;
    return {};
}

}  // namespace dayflow
