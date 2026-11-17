#include "data/task_repository.hpp"

#include <string>

#include "data/database.hpp"

namespace dayflow {
namespace {

constexpr char k_columns[] =
    "id, title, notes, priority, status, due_at_utc, completed_at, sort_order, created_at, updated_at";

Result<Priority> to_priority(int value)
{
    if (value < 0 || value > 3) {
        return make_error(ErrorCode::database, "nilai priority tidak dikenal: " + std::to_string(value));
    }
    return static_cast<Priority>(value);
}

Result<TaskStatus> to_status(int value)
{
    if (value < 0 || value > 3) {
        return make_error(ErrorCode::database, "nilai status tidak dikenal: " + std::to_string(value));
    }
    return static_cast<TaskStatus>(value);
}

Result<Task> read_task(Statement& statement)
{
    Task task;
    task.id = statement.column_int64(0);
    task.title = statement.column_text(1);
    task.notes = statement.column_text(2);

    auto priority = to_priority(statement.column_int(3));
    if (!priority.ok()) {
        return priority.error();
    }
    task.priority = priority.value();

    auto status = to_status(statement.column_int(4));
    if (!status.ok()) {
        return status.error();
    }
    task.status = status.value();

    auto due_at = statement.column_optional_timestamp(5);
    if (!due_at.ok()) {
        return due_at.error();
    }
    task.due_at = due_at.value();

    auto completed_at = statement.column_optional_timestamp(6);
    if (!completed_at.ok()) {
        return completed_at.error();
    }
    task.completed_at = completed_at.value();

    task.sort_order = statement.column_int(7);

    auto created_at = statement.column_optional_timestamp(8);
    if (!created_at.ok()) {
        return created_at.error();
    }
    if (!created_at.value().has_value()) {
        return make_error(ErrorCode::database, "kolom created_at kosong pada baris tasks");
    }
    task.created_at = *created_at.value();

    auto updated_at = statement.column_optional_timestamp(9);
    if (!updated_at.ok()) {
        return updated_at.error();
    }
    if (!updated_at.value().has_value()) {
        return make_error(ErrorCode::database, "kolom updated_at kosong pada baris tasks");
    }
    task.updated_at = *updated_at.value();

    return task;
}

}  // namespace

Result<std::vector<Task>> TaskRepository::list_all()
{
    auto guard = database_.lock();
    auto statement = database_.prepare(std::string{"SELECT "} + k_columns + " FROM tasks ORDER BY sort_order, id");
    if (!statement.ok()) {
        return statement.error();
    }

    std::vector<Task> tasks;
    for (;;) {
        auto row = statement.value().step();
        if (!row.ok()) {
            return row.error();
        }
        if (!row.value()) {
            break;
        }
        auto task = read_task(statement.value());
        if (!task.ok()) {
            return task.error();
        }
        tasks.push_back(std::move(task.value()));
    }
    return tasks;
}

Result<std::vector<Task>> TaskRepository::list_by_status(TaskStatus status)
{
    auto guard = database_.lock();
    auto statement = database_.prepare(
        std::string{"SELECT "} + k_columns + " FROM tasks WHERE status = ?1 ORDER BY sort_order, id");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, static_cast<int>(status))) {
        return error;
    }

    std::vector<Task> tasks;
    for (;;) {
        auto row = statement.value().step();
        if (!row.ok()) {
            return row.error();
        }
        if (!row.value()) {
            break;
        }
        auto task = read_task(statement.value());
        if (!task.ok()) {
            return task.error();
        }
        tasks.push_back(std::move(task.value()));
    }
    return tasks;
}

Result<std::vector<Task>> TaskRepository::search(std::string_view query, std::size_t limit)
{
    auto guard = database_.lock();
    auto statement = database_.prepare(std::string{"SELECT "} + k_columns
        + " FROM tasks WHERE title LIKE ?1 ESCAPE '\\' ORDER BY sort_order, id LIMIT ?2");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, like_pattern(query))) {
        return error;
    }
    if (Error error = statement.value().bind(2, static_cast<int>(limit))) {
        return error;
    }

    std::vector<Task> tasks;
    for (;;) {
        auto row = statement.value().step();
        if (!row.ok()) {
            return row.error();
        }
        if (!row.value()) {
            break;
        }
        auto task = read_task(statement.value());
        if (!task.ok()) {
            return task.error();
        }
        tasks.push_back(std::move(task.value()));
    }
    return tasks;
}

Result<std::optional<Task>> TaskRepository::find(std::int64_t id)
{
    auto guard = database_.lock();
    auto statement = database_.prepare(std::string{"SELECT "} + k_columns + " FROM tasks WHERE id = ?1");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, id)) {
        return error;
    }

    auto row = statement.value().step();
    if (!row.ok()) {
        return row.error();
    }
    if (!row.value()) {
        return std::optional<Task>{};
    }

    auto task = read_task(statement.value());
    if (!task.ok()) {
        return task.error();
    }
    return std::optional<Task>{std::move(task.value())};
}

Result<Task> TaskRepository::create(Task task)
{
    if (Error error = validate(task)) {
        return error;
    }

    const Timestamp now = now_utc();
    if (task.created_at == Timestamp{}) {
        task.created_at = now;
    }
    task.updated_at = now;

    auto guard = database_.lock();
    auto statement = database_.prepare(
        "INSERT INTO tasks (title, notes, priority, status, due_at_utc, completed_at, sort_order, created_at, updated_at) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)");
    if (!statement.ok()) {
        return statement.error();
    }

    Statement& insert = statement.value();
    if (Error error = insert.bind(1, task.title)) {
        return error;
    }
    if (Error error = insert.bind(2, task.notes)) {
        return error;
    }
    if (Error error = insert.bind(3, static_cast<int>(task.priority))) {
        return error;
    }
    if (Error error = insert.bind(4, static_cast<int>(task.status))) {
        return error;
    }
    if (Error error = insert.bind(5, task.due_at)) {
        return error;
    }
    if (Error error = insert.bind(6, task.completed_at)) {
        return error;
    }
    if (Error error = insert.bind(7, task.sort_order)) {
        return error;
    }
    if (Error error = insert.bind(8, to_iso8601(task.created_at))) {
        return error;
    }
    if (Error error = insert.bind(9, to_iso8601(task.updated_at))) {
        return error;
    }

    auto completed = insert.step();
    if (!completed.ok()) {
        return completed.error();
    }

    auto inserted_id = database_.last_insert_rowid();
    if (!inserted_id.ok()) {
        return inserted_id.error();
    }
    task.id = inserted_id.value();
    return task;
}

Error TaskRepository::update(Task task)
{
    if (task.id <= 0) {
        return make_error(ErrorCode::invalid_argument, "task tanpa id tidak bisa diupdate");
    }
    if (Error error = validate(task)) {
        return error;
    }
    task.updated_at = now_utc();

    auto guard = database_.lock();
    auto statement = database_.prepare(
        "UPDATE tasks SET title = ?1, notes = ?2, priority = ?3, status = ?4, due_at_utc = ?5, "
        "completed_at = ?6, sort_order = ?7, updated_at = ?8 WHERE id = ?9");
    if (!statement.ok()) {
        return statement.error();
    }

    Statement& update = statement.value();
    if (Error error = update.bind(1, task.title)) {
        return error;
    }
    if (Error error = update.bind(2, task.notes)) {
        return error;
    }
    if (Error error = update.bind(3, static_cast<int>(task.priority))) {
        return error;
    }
    if (Error error = update.bind(4, static_cast<int>(task.status))) {
        return error;
    }
    if (Error error = update.bind(5, task.due_at)) {
        return error;
    }
    if (Error error = update.bind(6, task.completed_at)) {
        return error;
    }
    if (Error error = update.bind(7, task.sort_order)) {
        return error;
    }
    if (Error error = update.bind(8, to_iso8601(task.updated_at))) {
        return error;
    }
    if (Error error = update.bind(9, task.id)) {
        return error;
    }

    auto completed = update.step();
    if (!completed.ok()) {
        return completed.error();
    }
    if (database_.changes() == 0) {
        return make_error(ErrorCode::not_found, "task dengan id " + std::to_string(task.id) + " tidak ditemukan");
    }
    return {};
}

Error TaskRepository::set_status(std::int64_t id, TaskStatus status)
{
    const Timestamp now = now_utc();

    auto guard = database_.lock();
    auto statement = database_.prepare(
        "UPDATE tasks SET status = ?1, "
        "completed_at = CASE WHEN ?1 = 2 THEN COALESCE(completed_at, ?2) ELSE NULL END, "
        "updated_at = ?2 WHERE id = ?3");
    if (!statement.ok()) {
        return statement.error();
    }

    Statement& update = statement.value();
    if (Error error = update.bind(1, static_cast<int>(status))) {
        return error;
    }
    if (Error error = update.bind(2, to_iso8601(now))) {
        return error;
    }
    if (Error error = update.bind(3, id)) {
        return error;
    }

    auto completed = update.step();
    if (!completed.ok()) {
        return completed.error();
    }
    if (database_.changes() == 0) {
        return make_error(ErrorCode::not_found, "task dengan id " + std::to_string(id) + " tidak ditemukan");
    }
    return {};
}

Error TaskRepository::remove(std::int64_t id)
{
    auto guard = database_.lock();
    auto statement = database_.prepare("DELETE FROM tasks WHERE id = ?1");
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
        return make_error(ErrorCode::not_found, "task dengan id " + std::to_string(id) + " tidak ditemukan");
    }
    return {};
}

}  // namespace dayflow
