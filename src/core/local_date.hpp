#pragma once

#include <chrono>
#include <string>
#include <string_view>

#include "core/error.hpp"

namespace dayflow {

using LocalDate = std::chrono::sys_days;

Result<LocalDate> parse_local_date(std::string_view text);
std::string format_local_date(LocalDate date);

}  // namespace dayflow
