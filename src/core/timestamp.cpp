#include "core/timestamp.hpp"

#include <cstdio>

namespace dayflow {
namespace {

bool is_digit(char value)
{
    return value >= '0' && value <= '9';
}

bool parse_fixed(std::string_view text, std::size_t position, int digits, int& out)
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

std::string to_iso8601(Timestamp value)
{
    const std::chrono::sys_days day_point = std::chrono::floor<std::chrono::days>(value);
    const std::chrono::year_month_day date{day_point};
    const std::chrono::hh_mm_ss time_of_day{value - day_point};

    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%04d-%02u-%02uT%02d:%02d:%02dZ",
        static_cast<int>(date.year()),
        static_cast<unsigned>(date.month()),
        static_cast<unsigned>(date.day()),
        static_cast<int>(time_of_day.hours().count()),
        static_cast<int>(time_of_day.minutes().count()),
        static_cast<int>(time_of_day.seconds().count()));
    return std::string{buffer};
}

Result<Timestamp> parse_iso8601(std::string_view text)
{
    if (text.size() != 20) {
        return make_error(ErrorCode::invalid_argument,
            "timestamp harus berformat YYYY-MM-DDTHH:MM:SSZ (20 karakter)");
    }
    if (text[4] != '-' || text[7] != '-' || text[10] != 'T' || text[13] != ':' || text[16] != ':'
        || text[19] != 'Z') {
        return make_error(ErrorCode::invalid_argument, "pemisah timestamp tidak sesuai format");
    }

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    if (!parse_fixed(text, 0, 4, year) || !parse_fixed(text, 5, 2, month) || !parse_fixed(text, 8, 2, day)
        || !parse_fixed(text, 11, 2, hour) || !parse_fixed(text, 14, 2, minute)
        || !parse_fixed(text, 17, 2, second)) {
        return make_error(ErrorCode::invalid_argument, "timestamp mengandung karakter bukan angka");
    }

    if (hour > 23 || minute > 59 || second > 59) {
        return make_error(ErrorCode::invalid_argument, "jam, menit, atau detik di luar rentang");
    }

    const std::chrono::year_month_day date{std::chrono::year{year},
        std::chrono::month{static_cast<unsigned>(month)}, std::chrono::day{static_cast<unsigned>(day)}};
    if (!date.ok()) {
        return make_error(ErrorCode::invalid_argument, "tanggal tidak valid");
    }

    const auto offset = std::chrono::hours{hour} + std::chrono::minutes{minute} + std::chrono::seconds{second};
    return Timestamp{std::chrono::sys_days{date}.time_since_epoch() + offset};
}

Timestamp now_utc()
{
    return std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
}

}  // namespace dayflow
