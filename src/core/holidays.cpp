#include "core/holidays.hpp"

#include <algorithm>

namespace dayflow {
namespace {

struct FixedHoliday {
    unsigned month;
    unsigned day;
    const char* name;
};

constexpr FixedHoliday k_fixed_holidays[] = {
    {1, 1, "Tahun Baru Masehi"},
    {5, 1, "Hari Buruh Internasional"},
    {6, 1, "Hari Lahir Pancasila"},
    {8, 17, "Hari Kemerdekaan RI"},
    {12, 25, "Hari Natal"},
};

}  // namespace

std::vector<Holiday> holidays_in_range(std::chrono::sys_days from, std::chrono::sys_days to)
{
    std::vector<Holiday> holidays;
    if (to < from) {
        return holidays;
    }

    const std::chrono::year_month_day first{from};
    const std::chrono::year_month_day last{to};
    const int first_year = static_cast<int>(first.year());
    const int last_year = static_cast<int>(last.year());

    for (int year = first_year; year <= last_year; ++year) {
        for (const FixedHoliday& fixed : k_fixed_holidays) {
            const std::chrono::year_month_day date{std::chrono::year{year},
                std::chrono::month{fixed.month}, std::chrono::day{fixed.day}};
            if (!date.ok()) {
                continue;
            }
            const std::chrono::sys_days day{date};
            if (day < from || day > to) {
                continue;
            }
            holidays.push_back(Holiday{day, fixed.name});
        }
    }

    std::sort(holidays.begin(), holidays.end(),
        [](const Holiday& left, const Holiday& right) { return left.day < right.day; });
    return holidays;
}

}  // namespace dayflow
