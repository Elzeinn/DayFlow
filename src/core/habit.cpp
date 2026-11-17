#include "core/habit.hpp"

#include <cctype>
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

Error validate(const Habit& habit)
{
    if (trim(habit.name).empty()) {
        return make_error(ErrorCode::invalid_argument, "nama kebiasaan tidak boleh kosong");
    }
    if (habit.name.size() > 120) {
        return make_error(ErrorCode::invalid_argument, "nama kebiasaan melebihi 120 karakter");
    }
    return {};
}

int current_streak(const std::set<LocalDate>& done_days, LocalDate today)
{
    if (done_days.empty()) {
        return 0;
    }

    LocalDate cursor = today;
    if (done_days.find(cursor) == done_days.end()) {
        cursor -= std::chrono::days{1};
    }

    int streak = 0;
    while (done_days.find(cursor) != done_days.end()) {
        ++streak;
        cursor -= std::chrono::days{1};
    }
    return streak;
}

}  // namespace dayflow
