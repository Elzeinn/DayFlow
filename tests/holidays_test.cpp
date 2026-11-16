#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>
#include <vector>

#include "core/holidays.hpp"

using namespace dayflow;

namespace {

std::chrono::sys_days day(int year, unsigned month, unsigned dom)
{
    return std::chrono::sys_days{
        std::chrono::year_month_day{std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{dom}}};
}

std::vector<std::string> names_of(const std::vector<Holiday>& holidays)
{
    std::vector<std::string> names;
    for (const Holiday& holiday : holidays) {
        names.push_back(holiday.name);
    }
    return names;
}

}

TEST_CASE("hari libur: natal muncul di desember")
{
    auto holidays = holidays_in_range(day(2026, 12, 1), day(2026, 12, 31));
    REQUIRE(holidays.size() == 1);
    CHECK(holidays.front().name == "Hari Natal");
    CHECK(holidays.front().day == day(2026, 12, 25));
}

TEST_CASE("hari libur: rentang penuh setahun mengembalikan lima hari libur tetap")
{
    auto holidays = holidays_in_range(day(2026, 1, 1), day(2026, 12, 31));
    REQUIRE(holidays.size() == 5);
    CHECK(names_of(holidays)
        == std::vector<std::string>{"Tahun Baru Masehi", "Hari Buruh Internasional", "Hari Lahir Pancasila",
            "Hari Kemerdekaan RI", "Hari Natal"});
}

TEST_CASE("hari libur: rentang tanpa hari libur kosong")
{
    auto holidays = holidays_in_range(day(2026, 2, 1), day(2026, 2, 28));
    CHECK(holidays.empty());
}

TEST_CASE("hari libur: rentang lintas tahun menghitung dua kali")
{
    auto holidays = holidays_in_range(day(2026, 12, 20), day(2027, 1, 5));
    REQUIRE(holidays.size() == 2);
    CHECK(holidays.front().name == "Hari Natal");
    CHECK(holidays.back().name == "Tahun Baru Masehi");
}
