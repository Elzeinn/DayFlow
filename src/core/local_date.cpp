#include "core/local_date.hpp"

#include <cstdio>

namespace dayflow {
namespace {

bool is_digit(char value)
{
    return value >= '0' && value <= '9';
}

bool parse_number(std::string_view text, std::size_t position, int digits, int& out)
{
    if (position + static_cast<std::size_t>(digits) > text.size()) {
        return false;
    }
    int value = 0;
    for (int index = 0; index < digits; ++index) {
        const char character = text[position + static_cast<std::size_t>(index)];
        if (!is_digit(character)) {
            return false;
        }
        value = value * 10 + (character - '0');
    }
    out = value;
    return true;
}

}  // namespace

Result<LocalDate> parse_local_date(std::string_view text)
{
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') {
        return make_error(ErrorCode::invalid_argument, "tanggal harus berformat YYYY-MM-DD");
    }

    int year = 0;
    int month = 0;
    int day = 0;
    if (!parse_number(text, 0, 4, year) || !parse_number(text, 5, 2, month) || !parse_number(text, 8, 2, day)) {
        return make_error(ErrorCode::invalid_argument, "tanggal mengandung karakter bukan angka");
    }

    const std::chrono::year_month_day date{std::chrono::year{year},
        std::chrono::month{static_cast<unsigned>(month)}, std::chrono::day{static_cast<unsigned>(day)}};
    if (!date.ok()) {
        return make_error(ErrorCode::invalid_argument, "tanggal tidak valid");
    }
    return LocalDate{date};
}

std::string format_local_date(LocalDate date)
{
    const std::chrono::year_month_day value{date};
    char buffer[16];
    std::snprintf(buffer, sizeof buffer, "%04d-%02u-%02u", static_cast<int>(value.year()),
        static_cast<unsigned>(value.month()), static_cast<unsigned>(value.day()));
    return std::string{buffer};
}

}  // namespace dayflow
