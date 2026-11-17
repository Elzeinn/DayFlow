#include "data/note_repository.hpp"

#include <string>

#include "data/database.hpp"

namespace dayflow {
namespace {

constexpr char k_select[] = "SELECT id, body, pinned, created_at, updated_at FROM notes";

Result<Note> read_note(Statement& statement)
{
    Note note;
    note.id = statement.column_int64(0);
    note.body = statement.column_text(1);
    note.pinned = statement.column_int(2) != 0;

    auto created_at = statement.column_optional_timestamp(3);
    if (!created_at.ok()) {
        return created_at.error();
    }
    if (created_at.value().has_value()) {
        note.created_at = *created_at.value();
    }

    auto updated_at = statement.column_optional_timestamp(4);
    if (!updated_at.ok()) {
        return updated_at.error();
    }
    if (updated_at.value().has_value()) {
        note.updated_at = *updated_at.value();
    }
    return note;
}

}  // namespace

Result<std::vector<Note>> NoteRepository::list_all()
{
    auto guard = database_.lock();
    auto statement = database_.prepare(
        std::string{k_select} + " ORDER BY pinned DESC, updated_at DESC, id DESC");
    if (!statement.ok()) {
        return statement.error();
    }

    std::vector<Note> notes;
    for (;;) {
        auto row = statement.value().step();
        if (!row.ok()) {
            return row.error();
        }
        if (!row.value()) {
            break;
        }
        auto note = read_note(statement.value());
        if (!note.ok()) {
            return note.error();
        }
        notes.push_back(std::move(note.value()));
    }
    return notes;
}

Result<std::vector<Note>> NoteRepository::search(std::string_view query, std::size_t limit)
{
    auto guard = database_.lock();
    auto statement = database_.prepare(std::string{k_select}
        + " WHERE body LIKE ?1 ESCAPE '\\' ORDER BY pinned DESC, updated_at DESC LIMIT ?2");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, like_pattern(query))) {
        return error;
    }
    if (Error error = statement.value().bind(2, static_cast<int>(limit))) {
        return error;
    }

    std::vector<Note> notes;
    for (;;) {
        auto row = statement.value().step();
        if (!row.ok()) {
            return row.error();
        }
        if (!row.value()) {
            break;
        }
        auto note = read_note(statement.value());
        if (!note.ok()) {
            return note.error();
        }
        notes.push_back(std::move(note.value()));
    }
    return notes;
}

Result<std::optional<Note>> NoteRepository::find(std::int64_t id)
{
    auto guard = database_.lock();
    auto statement = database_.prepare(std::string{k_select} + " WHERE id = ?1");
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
        return std::optional<Note>{};
    }

    auto note = read_note(statement.value());
    if (!note.ok()) {
        return note.error();
    }
    return std::optional<Note>{std::move(note.value())};
}

Result<Note> NoteRepository::create(Note note)
{
    if (Error error = validate(note)) {
        return error;
    }

    const Timestamp now = now_utc();
    if (note.created_at == Timestamp{}) {
        note.created_at = now;
    }
    note.updated_at = now;

    auto guard = database_.lock();
    auto statement = database_.prepare(
        "INSERT INTO notes (body, pinned, created_at, updated_at) VALUES (?1, ?2, ?3, ?4)");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, note.body)) {
        return error;
    }
    if (Error error = statement.value().bind(2, note.pinned ? 1 : 0)) {
        return error;
    }
    if (Error error = statement.value().bind(3, to_iso8601(note.created_at))) {
        return error;
    }
    if (Error error = statement.value().bind(4, to_iso8601(note.updated_at))) {
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
    note.id = inserted_id.value();
    return note;
}

Error NoteRepository::update(Note note)
{
    if (note.id <= 0) {
        return make_error(ErrorCode::invalid_argument, "catatan tanpa id tidak bisa diupdate");
    }
    if (Error error = validate(note)) {
        return error;
    }
    note.updated_at = now_utc();

    auto guard = database_.lock();
    auto statement = database_.prepare(
        "UPDATE notes SET body = ?1, pinned = ?2, updated_at = ?3 WHERE id = ?4");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, note.body)) {
        return error;
    }
    if (Error error = statement.value().bind(2, note.pinned ? 1 : 0)) {
        return error;
    }
    if (Error error = statement.value().bind(3, to_iso8601(note.updated_at))) {
        return error;
    }
    if (Error error = statement.value().bind(4, note.id)) {
        return error;
    }

    auto completed = statement.value().step();
    if (!completed.ok()) {
        return completed.error();
    }
    if (database_.changes() == 0) {
        return make_error(ErrorCode::not_found, "catatan tidak ditemukan");
    }
    return {};
}

Error NoteRepository::remove(std::int64_t id)
{
    auto guard = database_.lock();
    auto statement = database_.prepare("DELETE FROM notes WHERE id = ?1");
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
        return make_error(ErrorCode::not_found, "catatan tidak ditemukan");
    }
    return {};
}

}  // namespace dayflow
