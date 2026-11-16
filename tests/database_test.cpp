#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <memory>

#include "data/database.hpp"

using namespace dayflow;

TEST_CASE("database: skema v1 diterapkan saat open")
{
    auto opened = Database::open_in_memory();
    REQUIRE(opened.ok());

    Database& database = *opened.value();

    auto version = database.user_version();
    REQUIRE(version.ok());
    CHECK(version.value() == k_schema_version);

    auto statement = database.prepare(
        "SELECT count(*) FROM sqlite_master WHERE type = 'table' AND name IN "
        "('tasks','subtasks','events','event_categories','recurrence_rules','routines',"
        "'routine_items','routine_logs','habits','habit_logs','reminders','notes','settings')");
    REQUIRE(statement.ok());

    auto row = statement.value().step();
    REQUIRE(row.ok());
    REQUIRE(row.value());
    CHECK(statement.value().column_int(0) == 13);
}

TEST_CASE("database: foreign key aktif")
{
    auto opened = Database::open_in_memory();
    REQUIRE(opened.ok());
    Database& database = *opened.value();

    auto statement = database.prepare("INSERT INTO subtasks (task_id, title) VALUES (999, 'yatim')");
    REQUIRE(statement.ok());

    auto step = statement.value().step();
    CHECK_FALSE(step.ok());
}

TEST_CASE("database: migrasi idempoten pada file dan data bertahan")
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "dayflow_test_migration.db";
    std::filesystem::remove(path);

    {
        auto opened = Database::open(path);
        REQUIRE(opened.ok());

        auto version = opened.value()->user_version();
        REQUIRE(version.ok());
        CHECK(version.value() == k_schema_version);

        CHECK(opened.value()->exec("INSERT INTO settings (key, value) VALUES ('tema', 'gelap')").code
            == ErrorCode::none);
    }

    {
        auto opened = Database::open(path);
        REQUIRE(opened.ok());

        auto version = opened.value()->user_version();
        REQUIRE(version.ok());
        CHECK(version.value() == k_schema_version);

        auto statement = opened.value()->prepare("SELECT value FROM settings WHERE key = 'tema'");
        REQUIRE(statement.ok());
        auto row = statement.value().step();
        REQUIRE(row.ok());
        REQUIRE(row.value());
        CHECK(statement.value().column_text(0) == "gelap");
    }

    std::filesystem::remove(path);
    std::filesystem::remove(path.string() + "-wal");
    std::filesystem::remove(path.string() + "-shm");
}

TEST_CASE("database: task dengan judul NULL ditolak skema")
{
    auto opened = Database::open_in_memory();
    REQUIRE(opened.ok());
    Database& database = *opened.value();

    auto statement = database.prepare(
        "INSERT INTO tasks (title, created_at, updated_at) VALUES (NULL, '2026-01-01T00:00:00Z', "
        "'2026-01-01T00:00:00Z')");
    REQUIRE(statement.ok());
    CHECK_FALSE(statement.value().step().ok());
}
