#pragma once

#include <cstdint>
#include <string>

#include "core/error.hpp"
#include "core/timestamp.hpp"

namespace dayflow {

struct Routine {
    std::int64_t id = 0;
    std::string name;
    bool active = true;
    Timestamp created_at{};
};

struct RoutineItem {
    std::int64_t id = 0;
    std::int64_t routine_id = 0;
    std::string title;
    int weekday_mask = 127;
    int start_minute = 0;
    int duration_minutes = 30;
    int sort_order = 0;
};

enum class RoutineLogStatus {
    done = 0,
    skipped = 1,
    snoozed = 2,
    missed = 3,
};

inline constexpr int k_all_weekdays_mask = 127;

int weekday_bit(int iso_weekday);
bool runs_on(const RoutineItem& item, int iso_weekday);
std::string format_hhmm(int start_minute);

Error validate(const Routine& routine);
Error validate(const RoutineItem& item);

}  // namespace dayflow
