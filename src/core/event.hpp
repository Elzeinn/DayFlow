#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "core/error.hpp"
#include "core/timestamp.hpp"

namespace dayflow {

enum class RecurrenceFreq {
    none = 0,
    daily = 1,
    weekly = 2,
    monthly = 3,
    yearly = 4,
};

struct RecurrenceRule {
    RecurrenceFreq freq = RecurrenceFreq::none;
    int interval = 1;
    std::vector<int> by_weekday;
    std::vector<int> by_monthday;
    std::optional<int> count;
    std::optional<Timestamp> until;
};

struct EventCategory {
    std::int64_t id = 0;
    std::string name;
    std::string color;
    int sort_order = 0;
};

struct Event {
    std::int64_t id = 0;
    std::string title;
    std::string description;
    std::optional<std::int64_t> category_id;
    Timestamp starts_at{};
    Timestamp ends_at{};
    bool all_day = false;
    std::optional<RecurrenceRule> recurrence;
    Timestamp created_at{};
    Timestamp updated_at{};
};

inline constexpr std::size_t k_max_event_title_length = 200;

Error validate(const Event& event);
bool is_recurring(const Event& event);

}  // namespace dayflow
