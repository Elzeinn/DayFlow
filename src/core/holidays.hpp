#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace dayflow {

struct Holiday {
    std::chrono::sys_days day;
    std::string name;
};

std::vector<Holiday> holidays_in_range(std::chrono::sys_days from, std::chrono::sys_days to);

}  // namespace dayflow
