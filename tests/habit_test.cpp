#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <set>

#include "core/habit.hpp"
#include "data/database.hpp"
#include "data/routine_repository.hpp"

using namespace dayflow;

namespace {

LocalDate day(const char* text)
{
    auto parsed = parse_local_date(text);
    REQUIRE(parsed.ok());
    return parsed.value();
}

std::set<LocalDate> days(std::initializer_list<const char*> values)
{
    std::set<LocalDate> result;
    for (const char* value : values) {
        result.insert(day(value));
    }
    return result;
}

std::unique_ptr<HabitRepository> make_repo(std::unique_ptr<Database>& database)
{
    auto opened = Database::open_in_memory();
    REQUIRE(opened.ok());
    database = std::move(opened.value());
    return std::make_unique<HabitRepository>(*database);
}

}

TEST_CASE("habit: streak kosong")
{
    CHECK(current_streak({}, day("2026-11-17")) == 0);
}

TEST_CASE("habit: streak berhitung mundur dari hari ini")
{
    CHECK(current_streak(days({"2026-11-17"}), day("2026-11-17")) == 1);
    CHECK(current_streak(days({"2026-11-17", "2026-11-16", "2026-11-15"}), day("2026-11-17")) == 3);
}

TEST_CASE("habit: hari ini belum check-in tidak memutus streak")
{
    CHECK(current_streak(days({"2026-11-16", "2026-11-15"}), day("2026-11-17")) == 2);
}

TEST_CASE("habit: celah memutus streak")
{
    CHECK(current_streak(days({"2026-11-17", "2026-11-15", "2026-11-14"}), day("2026-11-17")) == 1);
}

TEST_CASE("habit: streak kemarin saja tetap dihitung")
{
    CHECK(current_streak(days({"2026-11-14"}), day("2026-11-17")) == 0);
}

TEST_CASE("habit: validasi nama")
{
    Habit habit;
    CHECK(validate(habit).code == ErrorCode::invalid_argument);

    habit.name = "   ";
    CHECK(validate(habit).code == ErrorCode::invalid_argument);

    habit.name = "jalan kaki";
    CHECK(validate(habit).code == ErrorCode::none);
}

TEST_CASE("habit repository: check-in dan riwayat")
{
    std::unique_ptr<Database> database;
    auto repo = make_repo(database);

    Habit habit;
    habit.name = "minum 2L";
    auto created = repo->create(habit);
    REQUIRE(created.ok());
    CHECK(created.value().id > 0);

    CHECK(repo->set_done(created.value().id, "2026-11-16", true).code == ErrorCode::none);
    CHECK(repo->set_done(created.value().id, "2026-11-17", true).code == ErrorCode::none);
    CHECK(repo->set_done(created.value().id, "2026-11-17", true).code == ErrorCode::none);

    auto dates = repo->dates_for(created.value().id, "2026-11-01", "2026-11-30");
    REQUIRE(dates.ok());
    REQUIRE(dates.value().size() == 2);
    CHECK(dates.value().front() == "2026-11-16");
    CHECK(dates.value().back() == "2026-11-17");

    CHECK(repo->set_done(created.value().id, "2026-11-17", false).code == ErrorCode::none);
    auto after = repo->dates_for(created.value().id, "2026-11-01", "2026-11-30");
    REQUIRE(after.ok());
    REQUIRE(after.value().size() == 1);
    CHECK(after.value().front() == "2026-11-16");

    auto listed = repo->list_all();
    REQUIRE(listed.ok());
    REQUIRE(listed.value().size() == 1);
    CHECK(listed.value().front().name == "minum 2L");

    CHECK(repo->remove(created.value().id).code == ErrorCode::none);
    auto empty = repo->list_all();
    REQUIRE(empty.ok());
    CHECK(empty.value().empty());
}
