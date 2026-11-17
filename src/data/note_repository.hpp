#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "core/error.hpp"
#include "core/note.hpp"

namespace dayflow {

class Database;

class NoteRepository {
public:
    explicit NoteRepository(Database& database) : database_(database) {}

    Result<std::vector<Note>> list_all();
    Result<std::vector<Note>> search(std::string_view query, std::size_t limit);
    Result<std::optional<Note>> find(std::int64_t id);
    Result<Note> create(Note note);
    Error update(Note note);
    Error remove(std::int64_t id);

private:
    Database& database_;
};

}  // namespace dayflow
