#include <catch2/catch_test_macros.hpp>

#include <memory>

#include "core/routine.hpp"
#include "data/database.hpp"
#include "data/routine_repository.hpp"

using namespace dayflow;

namespace {

std::unique_ptr<RoutineRepository> make_repo(std::unique_ptr<Database>& database)
{
    auto opened = Database::open_in_memory();
    REQUIRE(opened.ok());
    database = std::move(opened.value());
    return std::make_unique<RoutineRepository>(*database);
}

RoutineItem item(std::int64_t routine_id, const char* title, int mask, int start_minute)
{
    RoutineItem value;
    value.routine_id = routine_id;
    value.title = title;
    value.weekday_mask = mask;
    value.start_minute = start_minute;
    value.duration_minutes = 30;
    return value;
}

}

TEST_CASE("routine: bit hari dan runs_on")
{
    CHECK(weekday_bit(1) == 1);
    CHECK(weekday_bit(7) == 64);
    CHECK(weekday_bit(0) == 0);
    CHECK(weekday_bit(8) == 0);

    RoutineItem weekday_only;
    weekday_only.weekday_mask = weekday_bit(1) | weekday_bit(3);
    CHECK(runs_on(weekday_only, 1) == true);
    CHECK(runs_on(weekday_only, 2) == false);
    CHECK(runs_on(weekday_only, 3) == true);
}

TEST_CASE("routine: validasi kegiatan")
{
    RoutineItem value;
    value.routine_id = 1;
    value.title = "minum air";
    CHECK(validate(value).code == ErrorCode::none);

    value.title = "   ";
    CHECK(validate(value).code == ErrorCode::invalid_argument);

    value.title = "minum air";
    value.weekday_mask = 0;
    CHECK(validate(value).code == ErrorCode::invalid_argument);

    value.weekday_mask = 127;
    value.start_minute = 1440;
    CHECK(validate(value).code == ErrorCode::invalid_argument);

    value.start_minute = 0;
    value.duration_minutes = 0;
    CHECK(validate(value).code == ErrorCode::invalid_argument);
}

TEST_CASE("routine repository: buat, edit, dan hapus kegiatan")
{
    std::unique_ptr<Database> database;
    auto repo = make_repo(database);

    Routine routine;
    routine.name = "Pagi";
    auto created = repo->create_routine(routine);
    REQUIRE(created.ok());
    CHECK(created.value().id > 0);

    auto created_item = repo->create_item(item(created.value().id, "minum air", 31, 420));
    REQUIRE(created_item.ok());
    CHECK(created_item.value().id > 0);

    auto items = repo->list_items(created.value().id);
    REQUIRE(items.ok());
    REQUIRE(items.value().size() == 1);
    CHECK(items.value().front().title == "minum air");
    CHECK(items.value().front().start_minute == 420);

    RoutineItem changed = items.value().front();
    changed.title = "minum air 500ml";
    changed.duration_minutes = 5;
    CHECK(repo->update_item(changed).code == ErrorCode::none);

    auto reloaded = repo->list_items(created.value().id);
    REQUIRE(reloaded.ok());
    REQUIRE(reloaded.value().size() == 1);
    CHECK(reloaded.value().front().title == "minum air 500ml");
    CHECK(reloaded.value().front().duration_minutes == 5);

    CHECK(repo->remove_routine(created.value().id).code == ErrorCode::none);

    auto after_delete = repo->list_items(created.value().id);
    REQUIRE(after_delete.ok());
    CHECK(after_delete.value().empty());
}

TEST_CASE("routine repository: log harian tersimpan dan bisa dibatalkan")
{
    std::unique_ptr<Database> database;
    auto repo = make_repo(database);

    Routine routine;
    routine.name = "Malam";
    auto created = repo->create_routine(routine);
    REQUIRE(created.ok());

    auto created_item = repo->create_item(item(created.value().id, "baca buku", 127, 1290));
    REQUIRE(created_item.ok());

    const std::string day = "2026-11-17";

    CHECK(repo->set_log(created_item.value().id, day, RoutineLogStatus::done).code == ErrorCode::none);
    auto logs = repo->logs_for_date(day);
    REQUIRE(logs.ok());
    REQUIRE(logs.value().count(created_item.value().id) == 1);
    CHECK(logs.value().at(created_item.value().id) == RoutineLogStatus::done);

    CHECK(repo->set_log(created_item.value().id, day, RoutineLogStatus::skipped).code == ErrorCode::none);
    logs = repo->logs_for_date(day);
    REQUIRE(logs.ok());
    CHECK(logs.value().at(created_item.value().id) == RoutineLogStatus::skipped);

    CHECK(repo->clear_log(created_item.value().id, day).code == ErrorCode::none);
    logs = repo->logs_for_date(day);
    REQUIRE(logs.ok());
    CHECK(logs.value().empty());
}
