#include "data/routine_repository.hpp"

#include <string>

#include "data/database.hpp"

namespace dayflow {
namespace {

Result<Routine> read_routine(Statement& statement)
{
    Routine routine;
    routine.id = statement.column_int64(0);
    routine.name = statement.column_text(1);
    routine.active = statement.column_int(2) != 0;

    auto created_at = statement.column_optional_timestamp(3);
    if (!created_at.ok()) {
        return created_at.error();
    }
    if (created_at.value().has_value()) {
        routine.created_at = *created_at.value();
    }
    return routine;
}

Result<RoutineItem> read_item(Statement& statement)
{
    RoutineItem item;
    item.id = statement.column_int64(0);
    item.routine_id = statement.column_int64(1);
    item.title = statement.column_text(2);
    item.weekday_mask = statement.column_int(3);
    item.start_minute = statement.column_int(4);
    item.duration_minutes = statement.column_int(5);
    item.sort_order = statement.column_int(6);
    return item;
}

}  // namespace

Result<std::vector<Routine>> RoutineRepository::list_routines()
{
    auto guard = database_.lock();
    auto statement = database_.prepare("SELECT id, name, active, created_at FROM routines ORDER BY name");
    if (!statement.ok()) {
        return statement.error();
    }

    std::vector<Routine> routines;
    for (;;) {
        auto row = statement.value().step();
        if (!row.ok()) {
            return row.error();
        }
        if (!row.value()) {
            break;
        }
        auto routine = read_routine(statement.value());
        if (!routine.ok()) {
            return routine.error();
        }
        routines.push_back(std::move(routine.value()));
    }
    return routines;
}

Result<Routine> RoutineRepository::create_routine(Routine routine)
{
    if (Error error = validate(routine)) {
        return error;
    }

    auto guard = database_.lock();
    auto statement = database_.prepare("INSERT INTO routines (name, active, created_at) VALUES (?1, ?2, ?3)");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, routine.name)) {
        return error;
    }
    if (Error error = statement.value().bind(2, routine.active ? 1 : 0)) {
        return error;
    }
    if (Error error = statement.value().bind(3, to_iso8601(now_utc()))) {
        return error;
    }

    auto completed = statement.value().step();
    if (!completed.ok()) {
        return completed.error();
    }

    auto inserted_id = database_.last_insert_rowid();
    if (!inserted_id.ok()) {
        return inserted_id.error();
    }
    routine.id = inserted_id.value();
    routine.created_at = now_utc();
    return routine;
}

Error RoutineRepository::update_routine(Routine routine)
{
    if (routine.id <= 0) {
        return make_error(ErrorCode::invalid_argument, "rutinitas tanpa id tidak bisa diupdate");
    }
    if (Error error = validate(routine)) {
        return error;
    }

    auto guard = database_.lock();
    auto statement = database_.prepare("UPDATE routines SET name = ?1, active = ?2 WHERE id = ?3");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, routine.name)) {
        return error;
    }
    if (Error error = statement.value().bind(2, routine.active ? 1 : 0)) {
        return error;
    }
    if (Error error = statement.value().bind(3, routine.id)) {
        return error;
    }

    auto completed = statement.value().step();
    if (!completed.ok()) {
        return completed.error();
    }
    if (database_.changes() == 0) {
        return make_error(ErrorCode::not_found, "rutinitas tidak ditemukan");
    }
    return {};
}

Error RoutineRepository::remove_routine(std::int64_t id)
{
    auto guard = database_.lock();
    auto statement = database_.prepare("DELETE FROM routines WHERE id = ?1");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, id)) {
        return error;
    }

    auto completed = statement.value().step();
    if (!completed.ok()) {
        return completed.error();
    }
    if (database_.changes() == 0) {
        return make_error(ErrorCode::not_found, "rutinitas tidak ditemukan");
    }
    return {};
}

Result<std::vector<RoutineItem>> RoutineRepository::list_items(std::int64_t routine_id)
{
    auto guard = database_.lock();
    auto statement = database_.prepare(
        "SELECT id, routine_id, title, weekday_mask, start_minute, duration_minutes, sort_order "
        "FROM routine_items WHERE routine_id = ?1 ORDER BY sort_order, start_minute, id");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, routine_id)) {
        return error;
    }

    std::vector<RoutineItem> items;
    for (;;) {
        auto row = statement.value().step();
        if (!row.ok()) {
            return row.error();
        }
        if (!row.value()) {
            break;
        }
        auto item = read_item(statement.value());
        if (!item.ok()) {
            return item.error();
        }
        items.push_back(std::move(item.value()));
    }
    return items;
}

Result<RoutineItem> RoutineRepository::create_item(RoutineItem item)
{
    if (Error error = validate(item)) {
        return error;
    }
    if (item.routine_id <= 0) {
        return make_error(ErrorCode::invalid_argument, "kegiatan harus milik satu rutinitas");
    }

    auto guard = database_.lock();
    auto statement = database_.prepare(
        "INSERT INTO routine_items (routine_id, title, weekday_mask, start_minute, duration_minutes, sort_order) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6)");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, item.routine_id)) {
        return error;
    }
    if (Error error = statement.value().bind(2, item.title)) {
        return error;
    }
    if (Error error = statement.value().bind(3, item.weekday_mask)) {
        return error;
    }
    if (Error error = statement.value().bind(4, item.start_minute)) {
        return error;
    }
    if (Error error = statement.value().bind(5, item.duration_minutes)) {
        return error;
    }
    if (Error error = statement.value().bind(6, item.sort_order)) {
        return error;
    }

    auto completed = statement.value().step();
    if (!completed.ok()) {
        return completed.error();
    }

    auto inserted_id = database_.last_insert_rowid();
    if (!inserted_id.ok()) {
        return inserted_id.error();
    }
    item.id = inserted_id.value();
    return item;
}

Error RoutineRepository::update_item(RoutineItem item)
{
    if (item.id <= 0) {
        return make_error(ErrorCode::invalid_argument, "kegiatan tanpa id tidak bisa diupdate");
    }
    if (Error error = validate(item)) {
        return error;
    }

    auto guard = database_.lock();
    auto statement = database_.prepare(
        "UPDATE routine_items SET title = ?1, weekday_mask = ?2, start_minute = ?3, duration_minutes = ?4, "
        "sort_order = ?5 WHERE id = ?6");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, item.title)) {
        return error;
    }
    if (Error error = statement.value().bind(2, item.weekday_mask)) {
        return error;
    }
    if (Error error = statement.value().bind(3, item.start_minute)) {
        return error;
    }
    if (Error error = statement.value().bind(4, item.duration_minutes)) {
        return error;
    }
    if (Error error = statement.value().bind(5, item.sort_order)) {
        return error;
    }
    if (Error error = statement.value().bind(6, item.id)) {
        return error;
    }

    auto completed = statement.value().step();
    if (!completed.ok()) {
        return completed.error();
    }
    if (database_.changes() == 0) {
        return make_error(ErrorCode::not_found, "kegiatan tidak ditemukan");
    }
    return {};
}

Error RoutineRepository::remove_item(std::int64_t id)
{
    auto guard = database_.lock();
    auto statement = database_.prepare("DELETE FROM routine_items WHERE id = ?1");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, id)) {
        return error;
    }

    auto completed = statement.value().step();
    if (!completed.ok()) {
        return completed.error();
    }
    if (database_.changes() == 0) {
        return make_error(ErrorCode::not_found, "kegiatan tidak ditemukan");
    }
    return {};
}

Result<std::map<std::int64_t, RoutineLogStatus>> RoutineRepository::logs_for_date(const std::string& local_date)
{
    auto guard = database_.lock();
    auto statement = database_.prepare("SELECT item_id, status FROM routine_logs WHERE local_date = ?1");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, local_date)) {
        return error;
    }

    std::map<std::int64_t, RoutineLogStatus> logs;
    for (;;) {
        auto row = statement.value().step();
        if (!row.ok()) {
            return row.error();
        }
        if (!row.value()) {
            break;
        }
        const auto item_id = statement.value().column_int64(0);
        const int status = statement.value().column_int(1);
        if (status >= 0 && status <= 3) {
            logs[item_id] = static_cast<RoutineLogStatus>(status);
        }
    }
    return logs;
}

Error RoutineRepository::set_log(std::int64_t item_id, const std::string& local_date, RoutineLogStatus status)
{
    auto guard = database_.lock();
    auto statement = database_.prepare(
        "INSERT INTO routine_logs (item_id, local_date, status) VALUES (?1, ?2, ?3) "
        "ON CONFLICT(item_id, local_date) DO UPDATE SET status = excluded.status, snoozed_until = NULL");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, item_id)) {
        return error;
    }
    if (Error error = statement.value().bind(2, local_date)) {
        return error;
    }
    if (Error error = statement.value().bind(3, static_cast<int>(status))) {
        return error;
    }

    auto completed = statement.value().step();
    if (!completed.ok()) {
        return completed.error();
    }
    return {};
}

Error RoutineRepository::clear_log(std::int64_t item_id, const std::string& local_date)
{
    auto guard = database_.lock();
    auto statement = database_.prepare("DELETE FROM routine_logs WHERE item_id = ?1 AND local_date = ?2");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, item_id)) {
        return error;
    }
    if (Error error = statement.value().bind(2, local_date)) {
        return error;
    }

    auto completed = statement.value().step();
    if (!completed.ok()) {
        return completed.error();
    }
    return {};
}

Result<std::vector<Habit>> HabitRepository::list_all()
{
    auto guard = database_.lock();
    auto statement = database_.prepare("SELECT id, name, active, created_at FROM habits ORDER BY name");
    if (!statement.ok()) {
        return statement.error();
    }

    std::vector<Habit> habits;
    for (;;) {
        auto row = statement.value().step();
        if (!row.ok()) {
            return row.error();
        }
        if (!row.value()) {
            break;
        }
        Habit habit;
        habit.id = statement.value().column_int64(0);
        habit.name = statement.value().column_text(1);
        habit.active = statement.value().column_int(2) != 0;

        auto created_at = statement.value().column_optional_timestamp(3);
        if (!created_at.ok()) {
            return created_at.error();
        }
        if (created_at.value().has_value()) {
            habit.created_at = *created_at.value();
        }
        habits.push_back(std::move(habit));
    }
    return habits;
}

Result<Habit> HabitRepository::create(Habit habit)
{
    if (Error error = validate(habit)) {
        return error;
    }

    auto guard = database_.lock();
    auto statement = database_.prepare("INSERT INTO habits (name, active, created_at) VALUES (?1, ?2, ?3)");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, habit.name)) {
        return error;
    }
    if (Error error = statement.value().bind(2, habit.active ? 1 : 0)) {
        return error;
    }
    if (Error error = statement.value().bind(3, to_iso8601(now_utc()))) {
        return error;
    }

    auto completed = statement.value().step();
    if (!completed.ok()) {
        return completed.error();
    }

    auto inserted_id = database_.last_insert_rowid();
    if (!inserted_id.ok()) {
        return inserted_id.error();
    }
    habit.id = inserted_id.value();
    habit.created_at = now_utc();
    return habit;
}

Error HabitRepository::update(Habit habit)
{
    if (habit.id <= 0) {
        return make_error(ErrorCode::invalid_argument, "kebiasaan tanpa id tidak bisa diupdate");
    }
    if (Error error = validate(habit)) {
        return error;
    }

    auto guard = database_.lock();
    auto statement = database_.prepare("UPDATE habits SET name = ?1, active = ?2 WHERE id = ?3");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, habit.name)) {
        return error;
    }
    if (Error error = statement.value().bind(2, habit.active ? 1 : 0)) {
        return error;
    }
    if (Error error = statement.value().bind(3, habit.id)) {
        return error;
    }

    auto completed = statement.value().step();
    if (!completed.ok()) {
        return completed.error();
    }
    if (database_.changes() == 0) {
        return make_error(ErrorCode::not_found, "kebiasaan tidak ditemukan");
    }
    return {};
}

Error HabitRepository::remove(std::int64_t id)
{
    auto guard = database_.lock();
    auto statement = database_.prepare("DELETE FROM habits WHERE id = ?1");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, id)) {
        return error;
    }

    auto completed = statement.value().step();
    if (!completed.ok()) {
        return completed.error();
    }
    if (database_.changes() == 0) {
        return make_error(ErrorCode::not_found, "kebiasaan tidak ditemukan");
    }
    return {};
}

Result<std::vector<std::string>> HabitRepository::dates_for(
    std::int64_t habit_id, const std::string& from, const std::string& to)
{
    auto guard = database_.lock();
    auto statement = database_.prepare(
        "SELECT local_date FROM habit_logs WHERE habit_id = ?1 AND local_date BETWEEN ?2 AND ?3 ORDER BY local_date");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, habit_id)) {
        return error;
    }
    if (Error error = statement.value().bind(2, from)) {
        return error;
    }
    if (Error error = statement.value().bind(3, to)) {
        return error;
    }

    std::vector<std::string> dates;
    for (;;) {
        auto row = statement.value().step();
        if (!row.ok()) {
            return row.error();
        }
        if (!row.value()) {
            break;
        }
        dates.push_back(statement.value().column_text(0));
    }
    return dates;
}

Error HabitRepository::set_done(std::int64_t habit_id, const std::string& local_date, bool done)
{
    auto guard = database_.lock();

    if (done) {
        auto statement = database_.prepare(
            "INSERT INTO habit_logs (habit_id, local_date) VALUES (?1, ?2) ON CONFLICT DO NOTHING");
        if (!statement.ok()) {
            return statement.error();
        }
        if (Error error = statement.value().bind(1, habit_id)) {
            return error;
        }
        if (Error error = statement.value().bind(2, local_date)) {
            return error;
        }
        auto completed = statement.value().step();
        if (!completed.ok()) {
            return completed.error();
        }
        return {};
    }

    auto statement = database_.prepare("DELETE FROM habit_logs WHERE habit_id = ?1 AND local_date = ?2");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, habit_id)) {
        return error;
    }
    if (Error error = statement.value().bind(2, local_date)) {
        return error;
    }
    auto completed = statement.value().step();
    if (!completed.ok()) {
        return completed.error();
    }
    return {};
}

}  // namespace dayflow
