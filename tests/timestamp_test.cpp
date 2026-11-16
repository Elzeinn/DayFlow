#include <catch2/catch_test_macros.hpp>

#include "core/timestamp.hpp"

using dayflow::parse_iso8601;
using dayflow::Timestamp;
using dayflow::to_iso8601;

TEST_CASE("iso8601: format dasar")
{
    CHECK(to_iso8601(Timestamp{}) == "1970-01-01T00:00:00Z");

    auto parsed = parse_iso8601("2026-11-16T07:30:00Z");
    REQUIRE(parsed.ok());
    CHECK(to_iso8601(parsed.value()) == "2026-11-16T07:30:00Z");
}

TEST_CASE("iso8601: round trip")
{
    const char* samples[] = {
        "1970-01-01T00:00:01Z",
        "2000-02-29T23:59:59Z",
        "2024-12-31T00:00:00Z",
        "2038-01-19T03:14:08Z",
    };
    for (const char* sample : samples) {
        auto parsed = parse_iso8601(sample);
        REQUIRE(parsed.ok());
        CHECK(to_iso8601(parsed.value()) == sample);
    }
}

TEST_CASE("iso8601: urutan leksikografis sama dengan urutan waktu")
{
    auto earlier = parse_iso8601("2026-01-31T23:59:59Z");
    auto later = parse_iso8601("2026-02-01T00:00:00Z");
    REQUIRE(earlier.ok());
    REQUIRE(later.ok());
    CHECK(earlier.value() < later.value());
    CHECK(to_iso8601(earlier.value()) < to_iso8601(later.value()));
}

TEST_CASE("iso8601: input tidak valid ditolak")
{
    const char* invalid[] = {
        "",
        "2026-11-16",
        "2026-11-16T07:30:00",
        "2026-13-01T00:00:00Z",
        "2026-02-30T00:00:00Z",
        "2026-11-16T24:00:00Z",
        "2026-11-16T07:60:00Z",
        "abc6-11-16T07:30:00Z",
        "2026/11/16T07:30:00Z",
    };
    for (const char* sample : invalid) {
        CHECK_FALSE(parse_iso8601(sample).ok());
    }
}
