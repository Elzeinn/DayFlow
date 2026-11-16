#pragma once

#include <chrono>
#include <string>
#include <string_view>

#include "core/error.hpp"

namespace dayflow {

using Timestamp = std::chrono::sys_seconds;

std::string to_iso8601(Timestamp value);
Result<Timestamp> parse_iso8601(std::string_view text);
Timestamp now_utc();

}  // namespace dayflow
