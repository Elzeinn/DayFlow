#include <catch2/catch_test_macros.hpp>

#include "core/local_date.hpp"

using namespace dayflow;

TEST_CASE("local date: parse dan format bolak-balik")
{
    auto parsed = parse_local_date("2026-11-17");
    REQUIRE(parsed.ok());
    CHECK(format_local_date(parsed.value()) == "2026-11-17");
}

TEST_CASE("local date: input tidak valid ditolak")
{
    CHECK_FALSE(parse_local_date("2026-11-1").ok());
    CHECK_FALSE(parse_local_date("2026/11/17").ok());
    CHECK_FALSE(parse_local_date("2026-13-01").ok());
    CHECK_FALSE(parse_local_date("2026-02-30").ok());
    CHECK_FALSE(parse_local_date("").ok());
}

TEST_CASE("local date: urutan leksikografis sama dengan urutan tanggal")
{
    auto first = parse_local_date("2026-01-31");
    auto second = parse_local_date("2026-02-01");
    REQUIRE(first.ok());
    REQUIRE(second.ok());
    CHECK(format_local_date(first.value()) < format_local_date(second.value()));
    CHECK(first.value() < second.value());
}
