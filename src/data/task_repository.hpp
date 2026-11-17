#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "core/error.hpp"
#include "core/task.hpp"

namespace dayflow {

class Database;

class TaskRepository {
public:
    explicit TaskRepository(Database& database) : database_(database) {}

    Result<std::vector<Task>> list_all();
    Result<std::vector<Task>> list_by_status(TaskStatus status);
    Result<std::vector<Task>> search(std::string_view query, std::size_t limit);
    Result<std::optional<Task>> find(std::int64_t id);
    Result<Task> create(Task task);
    Error update(Task task);
    Error set_status(std::int64_t id, TaskStatus status);
    Error remove(std::int64_t id);

private:
    Database& database_;
};

}  // namespace dayflow
