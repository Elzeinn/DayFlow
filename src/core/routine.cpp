#include "core/routine.hpp"

#include <cctype>
#include <cstdio>
#include <string_view>

namespace dayflow {
namespace {

std::string_view trim(std::string_view text)
{
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return text.substr(begin, end - begin);
}

}  // namespace

int weekday_bit(int iso_weekday)
{
    if (iso_weekday < 1 || iso_weekday > 7) {
        return 0;
    }
    return 1 << (iso_weekday - 1);
}

bool runs_on(const RoutineItem& item, int iso_weekday)
{
    return (item.weekday_mask & weekday_bit(iso_weekday)) != 0;
}

std::string format_hhmm(int start_minute)
{
    const int clamped = start_minute < 0 ? 0 : (start_minute > 1439 ? 1439 : start_minute);
    char buffer[8];
    std::snprintf(buffer, sizeof buffer, "%02d:%02d", clamped / 60, clamped % 60);
    return std::string{buffer};
}

Error validate(const Routine& routine)
{
    if (trim(routine.name).empty()) {
        return make_error(ErrorCode::invalid_argument, "nama rutinitas tidak boleh kosong");
    }
    return {};
}

Error validate(const RoutineItem& item)
{
    if (trim(item.title).empty()) {
        return make_error(ErrorCode::invalid_argument, "judul kegiatan tidak boleh kosong");
    }
    if (item.weekday_mask <= 0 || item.weekday_mask > k_all_weekdays_mask) {
        return make_error(ErrorCode::invalid_argument, "pilih minimal satu hari");
    }
    if (item.start_minute < 0 || item.start_minute > 1439) {
        return make_error(ErrorCode::invalid_argument, "jam mulai harus antara 00:00 dan 23:59");
    }
    if (item.duration_minutes < 1 || item.duration_minutes > 1440) {
        return make_error(ErrorCode::invalid_argument, "durasi harus antara 1 dan 1440 menit");
    }
    return {};
}

}  // namespace dayflow
