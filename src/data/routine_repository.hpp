#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "core/error.hpp"
#include "core/habit.hpp"
#include "core/routine.hpp"

namespace dayflow {

class Database;

class RoutineRepository {
public:
    explicit RoutineRepository(Database& database) : database_(database) {}

    Result<std::vector<Routine>> list_routines();
    Result<Routine> create_routine(Routine routine);
    Error update_routine(Routine routine);
    Error remove_routine(std::int64_t id);

    Result<std::vector<RoutineItem>> list_items(std::int64_t routine_id);
    Result<RoutineItem> create_item(RoutineItem item);
    Error update_item(RoutineItem item);
    Error remove_item(std::int64_t id);

    Result<std::map<std::int64_t, RoutineLogStatus>> logs_for_date(const std::string& local_date);
    Error set_log(std::int64_t item_id, const std::string& local_date, RoutineLogStatus status);
    Error clear_log(std::int64_t item_id, const std::string& local_date);

private:
    Database& database_;
};

class HabitRepository {
public:
    explicit HabitRepository(Database& database) : database_(database) {}

    Result<std::vector<Habit>> list_all();
    Result<Habit> create(Habit habit);
    Error update(Habit habit);
    Error remove(std::int64_t id);

    Result<std::vector<std::string>> dates_for(std::int64_t habit_id, const std::string& from, const std::string& to);
    Error set_done(std::int64_t habit_id, const std::string& local_date, bool done);

private:
    Database& database_;
};

}  // namespace dayflow
