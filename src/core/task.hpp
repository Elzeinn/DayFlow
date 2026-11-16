#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "core/error.hpp"
#include "core/timestamp.hpp"

namespace dayflow {

enum class Priority : int {
    low = 0,
    normal = 1,
    high = 2,
    urgent = 3,
};

enum class TaskStatus : int {
    todo = 0,
    doing = 1,
    done = 2,
    cancelled = 3,
};


inline constexpr std::size_t k_max_title_length = 200;

struct Task {
    std::int64_t id = 0;
    std::string title;
    std::string notes;
    Priority priority = Priority::normal;
    TaskStatus status = TaskStatus::todo;
    std::optional<Timestamp> due_at;
    std::optional<Timestamp> completed_at;
    int sort_order = 0;
    Timestamp created_at{};
    Timestamp updated_at{};
};

std::string to_string(Priority priority);
std::string to_string(TaskStatus taskstatus);

Error validate(const Task& task);
bool is_finished(TaskStatus status);

}  // namespace dayflow
