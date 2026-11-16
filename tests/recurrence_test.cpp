#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "core/event.hpp"
#include "core/recurrence.hpp"

using namespace dayflow;

namespace {

Timestamp at(const char* text)
{
    auto parsed = parse_iso8601(text);
    REQUIRE(parsed.ok());
    return parsed.value();
}

Event make_event(const char* start, const char* end)
{
    Event event;
    event.id = 1;
    event.title = "contoh";
    event.starts_at = at(start);
    event.ends_at = at(end);
    return event;
}

std::vector<std::string> starts_of(const std::vector<Occurrence>& items)
{
    std::vector<std::string> result;
    for (const Occurrence& item : items) {
        result.push_back(to_iso8601(item.starts_at));
    }
    return result;
}

}

TEST_CASE("recurrence: event tunggal hanya muncul kalau beririsan")
{
    Event event = make_event("2026-01-10T09:00:00Z", "2026-01-10T10:00:00Z");

    auto inside = expand_occurrences(event, at("2026-01-01T00:00:00Z"), at("2026-02-01T00:00:00Z"));
    REQUIRE(inside.ok());
    CHECK(inside.value().size() == 1);

    auto before = expand_occurrences(event, at("2026-01-01T00:00:00Z"), at("2026-01-05T00:00:00Z"));
    REQUIRE(before.ok());
    CHECK(before.value().empty());
}

TEST_CASE("recurrence: harian dengan interval 2")
{
    Event event = make_event("2026-01-01T09:00:00Z", "2026-01-01T10:00:00Z");
    RecurrenceRule rule;
    rule.freq = RecurrenceFreq::daily;
    rule.interval = 2;
    event.recurrence = rule;

    auto result = expand_occurrences(event, at("2026-01-01T00:00:00Z"), at("2026-01-06T00:00:00Z"));
    REQUIRE(result.ok());
    CHECK(starts_of(result.value())
        == std::vector<std::string>{
            "2026-01-01T09:00:00Z", "2026-01-03T09:00:00Z", "2026-01-05T09:00:00Z"});
}

TEST_CASE("recurrence: mingguan pada Senin, Rabu, Jumat")
{
    Event event = make_event("2026-01-05T19:00:00Z", "2026-01-05T20:00:00Z");
    RecurrenceRule rule;
    rule.freq = RecurrenceFreq::weekly;
    rule.by_weekday = {1, 3, 5};
    event.recurrence = rule;

    auto result = expand_occurrences(event, at("2026-01-05T00:00:00Z"), at("2026-01-10T00:00:00Z"));
    REQUIRE(result.ok());
    CHECK(starts_of(result.value())
        == std::vector<std::string>{
            "2026-01-05T19:00:00Z", "2026-01-07T19:00:00Z", "2026-01-09T19:00:00Z"});
}

TEST_CASE("recurrence: mingguan dengan interval 2")
{
    Event event = make_event("2026-01-05T08:00:00Z", "2026-01-05T09:00:00Z");
    RecurrenceRule rule;
    rule.freq = RecurrenceFreq::weekly;
    rule.interval = 2;
    event.recurrence = rule;

    auto result = expand_occurrences(event, at("2026-01-05T00:00:00Z"), at("2026-02-03T00:00:00Z"));
    REQUIRE(result.ok());
    CHECK(starts_of(result.value())
        == std::vector<std::string>{
            "2026-01-05T08:00:00Z", "2026-01-19T08:00:00Z", "2026-02-02T08:00:00Z"});
}

TEST_CASE("recurrence: bulanan tanggal 31 melewati bulan pendek")
{
    Event event = make_event("2026-01-31T10:00:00Z", "2026-01-31T11:00:00Z");
    RecurrenceRule rule;
    rule.freq = RecurrenceFreq::monthly;
    event.recurrence = rule;

    auto result = expand_occurrences(event, at("2026-01-01T00:00:00Z"), at("2026-05-01T00:00:00Z"));
    REQUIRE(result.ok());
    CHECK(starts_of(result.value()) == std::vector<std::string>{"2026-01-31T10:00:00Z", "2026-03-31T10:00:00Z"});
}

TEST_CASE("recurrence: bulanan hari terakhir")
{
    Event event = make_event("2026-01-31T10:00:00Z", "2026-01-31T11:00:00Z");
    RecurrenceRule rule;
    rule.freq = RecurrenceFreq::monthly;
    rule.by_monthday = {-1};
    event.recurrence = rule;

    auto result = expand_occurrences(event, at("2026-01-01T00:00:00Z"), at("2026-04-01T00:00:00Z"));
    REQUIRE(result.ok());
    CHECK(starts_of(result.value())
        == std::vector<std::string>{
            "2026-01-31T10:00:00Z", "2026-02-28T10:00:00Z", "2026-03-31T10:00:00Z"});
}

TEST_CASE("recurrence: count membatasi jumlah kemunculan")
{
    Event event = make_event("2026-01-01T09:00:00Z", "2026-01-01T10:00:00Z");
    RecurrenceRule rule;
    rule.freq = RecurrenceFreq::daily;
    rule.count = 3;
    event.recurrence = rule;

    auto result = expand_occurrences(event, at("2026-01-01T00:00:00Z"), at("2026-02-01T00:00:00Z"));
    REQUIRE(result.ok());
    CHECK(result.value().size() == 3);
}

TEST_CASE("recurrence: until menghentikan pengulangan")
{
    Event event = make_event("2026-01-05T08:00:00Z", "2026-01-05T09:00:00Z");
    RecurrenceRule rule;
    rule.freq = RecurrenceFreq::weekly;
    rule.until = at("2026-01-20T00:00:00Z");
    event.recurrence = rule;

    auto result = expand_occurrences(event, at("2026-01-05T00:00:00Z"), at("2026-02-01T00:00:00Z"));
    REQUIRE(result.ok());
    CHECK(starts_of(result.value())
        == std::vector<std::string>{
            "2026-01-05T08:00:00Z", "2026-01-12T08:00:00Z", "2026-01-19T08:00:00Z"});
}

TEST_CASE("recurrence: tahunan")
{
    Event event = make_event("2024-03-15T07:00:00Z", "2024-03-15T08:00:00Z");
    RecurrenceRule rule;
    rule.freq = RecurrenceFreq::yearly;
    event.recurrence = rule;

    auto result = expand_occurrences(event, at("2024-01-01T00:00:00Z"), at("2028-01-01T00:00:00Z"));
    REQUIRE(result.ok());
    CHECK(result.value().size() == 4);
    CHECK(to_iso8601(result.value().front().starts_at) == "2024-03-15T07:00:00Z");
    CHECK(to_iso8601(result.value().back().starts_at) == "2027-03-15T07:00:00Z");
}

TEST_CASE("recurrence: aturan tidak valid ditolak")
{
    Event event = make_event("2026-01-01T09:00:00Z", "2026-01-01T10:00:00Z");
    RecurrenceRule rule;
    rule.freq = RecurrenceFreq::daily;
    rule.interval = 0;
    event.recurrence = rule;

    auto result = expand_occurrences(event, at("2026-01-01T00:00:00Z"), at("2026-02-01T00:00:00Z"));
    CHECK_FALSE(result.ok());
}
