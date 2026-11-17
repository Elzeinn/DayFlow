#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "core/error.hpp"
#include "core/local_date.hpp"
#include "core/timestamp.hpp"

namespace dayflow {

struct Habit {
    std::int64_t id = 0;
    std::string name;
    bool active = true;
    Timestamp created_at{};
};

Error validate(const Habit& habit);

int current_streak(const std::set<LocalDate>& done_days, LocalDate today);

}  // namespace dayflow
