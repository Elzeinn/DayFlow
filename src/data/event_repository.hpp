#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "core/error.hpp"
#include "core/event.hpp"
#include "core/timestamp.hpp"

namespace dayflow {

class Database;

class EventRepository {
public:
    explicit EventRepository(Database& database) : database_(database) {}

    Result<std::vector<Event>> list_for_range(Timestamp range_start, Timestamp range_end);
    Result<std::vector<Event>> search(std::string_view query, std::size_t limit);
    Result<std::optional<Event>> find(std::int64_t id);
    Result<Event> create(Event event);
    Error update(Event event);
    Error remove(std::int64_t id);

private:
    Database& database_;
};

class EventCategoryRepository {
public:
    explicit EventCategoryRepository(Database& database) : database_(database) {}

    Result<std::vector<EventCategory>> list_all();
    Result<EventCategory> create(EventCategory category);
    Error remove(std::int64_t id);
    Error ensure_defaults();

private:
    Database& database_;
};

}  // namespace dayflow
