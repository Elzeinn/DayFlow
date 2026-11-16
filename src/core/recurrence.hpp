#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/error.hpp"
#include "core/event.hpp"
#include "core/timestamp.hpp"

namespace dayflow {

struct Occurrence {
    std::int64_t event_id = 0;
    Timestamp starts_at{};
    Timestamp ends_at{};
};

Result<std::vector<Occurrence>> expand_occurrences(
    const Event& event, Timestamp range_start, Timestamp range_end, std::size_t limit = 400);

bool overlaps(const Occurrence& occurrence, Timestamp range_start, Timestamp range_end);

}  // namespace dayflow
