#include <catch2/catch_test_macros.hpp>

#include <memory>

#include "core/event.hpp"
#include "core/note.hpp"
#include "core/task.hpp"
#include "data/database.hpp"
#include "data/event_repository.hpp"
#include "data/note_repository.hpp"
#include "data/task_repository.hpp"

using namespace dayflow;

namespace {

std::unique_ptr<Database> open_database()
{
    auto opened = Database::open_in_memory();
    REQUIRE(opened.ok());
    return std::move(opened.value());
}

Timestamp at(const char* text)
{
    auto parsed = parse_iso8601(text);
    REQUIRE(parsed.ok());
    return parsed.value();
}

}

TEST_CASE("like pattern: karakter khusus di-escape")
{
    CHECK(like_pattern("baca") == "%baca%");
    CHECK(like_pattern("50%") == "%50\\%%");
    CHECK(like_pattern("a_b") == "%a\\_b%");
    CHECK(like_pattern("a\\b") == "%a\\\\b%");
}

TEST_CASE("search tugas: cocok sebagian dan tidak peka huruf besar")
{
    auto database = open_database();
    TaskRepository tasks{*database};

    Task first;
    first.title = "Belajar SQLite";
    REQUIRE(tasks.create(first).ok());

    Task second;
    second.title = "Belanja bulanan";
    REQUIRE(tasks.create(second).ok());

    auto found = tasks.search("bel", 10);
    REQUIRE(found.ok());
    CHECK(found.value().size() == 2);

    auto narrow = tasks.search("sql", 10);
    REQUIRE(narrow.ok());
    REQUIRE(narrow.value().size() == 1);
    CHECK(narrow.value().front().title == "Belajar SQLite");

    auto limited = tasks.search("bel", 1);
    REQUIRE(limited.ok());
    CHECK(limited.value().size() == 1);
}

TEST_CASE("search tugas: karakter persen diperlakukan literal")
{
    auto database = open_database();
    TaskRepository tasks{*database};

    Task with_percent;
    with_percent.title = "diskon 50% saja";
    REQUIRE(tasks.create(with_percent).ok());

    Task without;
    without.title = "diskon besar";
    REQUIRE(tasks.create(without).ok());

    auto found = tasks.search("50%", 10);
    REQUIRE(found.ok());
    REQUIRE(found.value().size() == 1);
    CHECK(found.value().front().title == "diskon 50% saja");
}

TEST_CASE("search catatan: mencari di seluruh isi")
{
    auto database = open_database();
    NoteRepository notes{*database};

    Note note;
    note.body = "ide: aplikasi pengingat minum air";
    REQUIRE(notes.create(note).ok());

    Note other;
    other.body = "daftar belanja";
    REQUIRE(notes.create(other).ok());

    auto found = notes.search("minum", 10);
    REQUIRE(found.ok());
    REQUIRE(found.value().size() == 1);
    CHECK(found.value().front().body.find("minum") != std::string::npos);
}

TEST_CASE("search event: mencari judul")
{
    auto database = open_database();
    EventRepository events{*database};

    Event event;
    event.title = "Rapat mingguan";
    event.starts_at = at("2026-11-17T09:00:00Z");
    event.ends_at = at("2026-11-17T10:00:00Z");
    REQUIRE(events.create(event).ok());

    Event other;
    other.title = "Olahraga pagi";
    other.starts_at = at("2026-11-18T06:00:00Z");
    other.ends_at = at("2026-11-18T07:00:00Z");
    REQUIRE(events.create(other).ok());

    auto found = events.search("rapat", 10);
    REQUIRE(found.ok());
    REQUIRE(found.value().size() == 1);
    CHECK(found.value().front().title == "Rapat mingguan");

    auto none = events.search("tidak ada", 10);
    REQUIRE(none.ok());
    CHECK(none.value().empty());
}
