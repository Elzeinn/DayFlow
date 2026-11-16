#include <catch2/catch_test_macros.hpp>

#include <memory>

#include "core/event.hpp"
#include "data/database.hpp"
#include "data/event_repository.hpp"

using namespace dayflow;

namespace {

Timestamp at(const char* text)
{
    auto parsed = parse_iso8601(text);
    REQUIRE(parsed.ok());
    return parsed.value();
}

struct Repo {
    std::unique_ptr<Database> database;
    std::unique_ptr<EventRepository> events;
    std::unique_ptr<EventCategoryRepository> categories;
};

Repo make_repo()
{
    Repo repo;
    auto opened = Database::open_in_memory();
    REQUIRE(opened.ok());
    repo.database = std::move(opened.value());
    repo.events = std::make_unique<EventRepository>(*repo.database);
    repo.categories = std::make_unique<EventCategoryRepository>(*repo.database);
    return repo;
}

Event make_event(const char* title, const char* start, const char* end)
{
    Event event;
    event.title = title;
    event.starts_at = at(start);
    event.ends_at = at(end);
    return event;
}

}

TEST_CASE("event repository: create dan find")
{
    Repo repo = make_repo();

    auto created = repo.events->create(make_event("rapat", "2026-01-10T09:00:00Z", "2026-01-10T10:00:00Z"));
    REQUIRE(created.ok());
    CHECK(created.value().id > 0);
    CHECK(created.value().created_at != Timestamp{});

    auto found = repo.events->find(created.value().id);
    REQUIRE(found.ok());
    REQUIRE(found.value().has_value());
    CHECK(found.value()->title == "rapat");
    CHECK(to_iso8601(found.value()->starts_at) == "2026-01-10T09:00:00Z");
    CHECK_FALSE(found.value()->recurrence.has_value());
}

TEST_CASE("event repository: create menolak event tidak valid")
{
    Repo repo = make_repo();

    Event event = make_event("", "2026-01-10T09:00:00Z", "2026-01-10T10:00:00Z");
    auto created = repo.events->create(event);
    CHECK_FALSE(created.ok());
    CHECK(created.error().code == ErrorCode::invalid_argument);

    Event backwards = make_event("mundur", "2026-01-10T10:00:00Z", "2026-01-10T09:00:00Z");
    CHECK_FALSE(repo.events->create(backwards).ok());
}

TEST_CASE("event repository: list_for_range memfilter dan menyertakan yang berulang")
{
    Repo repo = make_repo();

    REQUIRE(repo.events->create(make_event("januari", "2026-01-10T09:00:00Z", "2026-01-10T10:00:00Z")).ok());
    REQUIRE(repo.events->create(make_event("maret", "2026-03-10T09:00:00Z", "2026-03-10T10:00:00Z")).ok());

    Event weekly = make_event("mingguan", "2026-01-05T19:00:00Z", "2026-01-05T20:00:00Z");
    RecurrenceRule rule;
    rule.freq = RecurrenceFreq::weekly;
    rule.by_weekday = {1};
    weekly.recurrence = rule;
    REQUIRE(repo.events->create(weekly).ok());

    auto january = repo.events->list_for_range(at("2026-01-01T00:00:00Z"), at("2026-02-01T00:00:00Z"));
    REQUIRE(january.ok());
    CHECK(january.value().size() == 2);
    CHECK(january.value().front().title == "mingguan");

    auto march = repo.events->list_for_range(at("2026-03-01T00:00:00Z"), at("2026-04-01T00:00:00Z"));
    REQUIRE(march.ok());
    CHECK(march.value().size() == 2);
}

TEST_CASE("event repository: aturan pengulangan tersimpan dan bisa dibaca")
{
    Repo repo = make_repo();

    Event event = make_event("senam", "2026-01-05T06:00:00Z", "2026-01-05T06:30:00Z");
    RecurrenceRule rule;
    rule.freq = RecurrenceFreq::weekly;
    rule.interval = 2;
    rule.by_weekday = {1, 3};
    rule.count = 10;
    rule.until = at("2026-06-01T00:00:00Z");
    event.recurrence = rule;

    auto created = repo.events->create(event);
    REQUIRE(created.ok());

    auto found = repo.events->find(created.value().id);
    REQUIRE(found.ok());
    REQUIRE(found.value().has_value());
    REQUIRE(found.value()->recurrence.has_value());
    CHECK(found.value()->recurrence->freq == RecurrenceFreq::weekly);
    CHECK(found.value()->recurrence->interval == 2);
    CHECK(found.value()->recurrence->by_weekday == std::vector<int>{1, 3});
    REQUIRE(found.value()->recurrence->count.has_value());
    CHECK(*found.value()->recurrence->count == 10);
    REQUIRE(found.value()->recurrence->until.has_value());
    CHECK(to_iso8601(*found.value()->recurrence->until) == "2026-06-01T00:00:00Z");
}

TEST_CASE("event repository: update mengganti aturan pengulangan")
{
    Repo repo = make_repo();

    Event event = make_event("latihan", "2026-01-05T06:00:00Z", "2026-01-05T06:30:00Z");
    RecurrenceRule daily;
    daily.freq = RecurrenceFreq::daily;
    event.recurrence = daily;

    auto created = repo.events->create(event);
    REQUIRE(created.ok());

    Event changed = created.value();
    changed.title = "latihan pagi";
    changed.recurrence->freq = RecurrenceFreq::monthly;
    CHECK(repo.events->update(changed).code == ErrorCode::none);

    auto found = repo.events->find(created.value().id);
    REQUIRE(found.ok());
    REQUIRE(found.value().has_value());
    CHECK(found.value()->title == "latihan pagi");
    REQUIRE(found.value()->recurrence.has_value());
    CHECK(found.value()->recurrence->freq == RecurrenceFreq::monthly);

    changed.recurrence.reset();
    CHECK(repo.events->update(changed).code == ErrorCode::none);

    auto without_rule = repo.events->find(created.value().id);
    REQUIRE(without_rule.ok());
    REQUIRE(without_rule.value().has_value());
    CHECK_FALSE(without_rule.value()->recurrence.has_value());
}

TEST_CASE("event repository: remove menghapus event dan aturannya")
{
    Repo repo = make_repo();

    Event event = make_event("hapus", "2026-01-05T06:00:00Z", "2026-01-05T06:30:00Z");
    RecurrenceRule rule;
    rule.freq = RecurrenceFreq::daily;
    event.recurrence = rule;

    auto created = repo.events->create(event);
    REQUIRE(created.ok());

    CHECK(repo.events->remove(created.value().id).code == ErrorCode::none);

    auto found = repo.events->find(created.value().id);
    REQUIRE(found.ok());
    CHECK_FALSE(found.value().has_value());

    auto rules = repo.database->prepare("SELECT count(*) FROM recurrence_rules");
    REQUIRE(rules.ok());
    auto row = rules.value().step();
    REQUIRE(row.ok());
    REQUIRE(row.value());
    CHECK(rules.value().column_int(0) == 0);
}

TEST_CASE("kategori: default dibuat sekali saja dan idempoten")
{
    Repo repo = make_repo();

    CHECK(repo.categories->ensure_defaults().code == ErrorCode::none);
    CHECK(repo.categories->ensure_defaults().code == ErrorCode::none);

    auto categories = repo.categories->list_all();
    REQUIRE(categories.ok());
    CHECK(categories.value().size() == 4);
    CHECK(categories.value().front().name == "Pribadi");
    CHECK(categories.value().front().color == "#BEEE11");
}

TEST_CASE("kategori: create dan remove")
{
    Repo repo = make_repo();

    EventCategory category;
    category.name = "Proyek";
    category.color = "#FF5555";
    category.sort_order = 9;

    auto created = repo.categories->create(category);
    REQUIRE(created.ok());
    CHECK(created.value().id > 0);

    auto listed = repo.categories->list_all();
    REQUIRE(listed.ok());
    CHECK(listed.value().size() == 1);
    CHECK(listed.value().front().name == "Proyek");

    CHECK(repo.categories->remove(created.value().id).code == ErrorCode::none);
    CHECK(repo.categories->remove(created.value().id).code == ErrorCode::not_found);
}
