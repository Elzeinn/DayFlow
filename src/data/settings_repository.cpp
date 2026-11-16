#include "data/settings_repository.hpp"

#include <string>

#include "data/database.hpp"

namespace dayflow {

Result<std::optional<std::string>> SettingsRepository::get(std::string_view key)
{
    auto guard = database_.lock();
    auto statement = database_.prepare("SELECT value FROM settings WHERE key = ?1");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, key)) {
        return error;
    }

    auto row = statement.value().step();
    if (!row.ok()) {
        return row.error();
    }
    if (!row.value()) {
        return std::optional<std::string>{};
    }
    return std::optional<std::string>{statement.value().column_text(0)};
}

Error SettingsRepository::set(std::string_view key, std::string_view value)
{
    auto guard = database_.lock();
    auto statement = database_.prepare(
        "INSERT INTO settings (key, value) VALUES (?1, ?2) "
        "ON CONFLICT(key) DO UPDATE SET value = excluded.value");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, key)) {
        return error;
    }
    if (Error error = statement.value().bind(2, value)) {
        return error;
    }

    auto completed = statement.value().step();
    if (!completed.ok()) {
        return completed.error();
    }
    return {};
}

Error SettingsRepository::remove(std::string_view key)
{
    auto guard = database_.lock();
    auto statement = database_.prepare("DELETE FROM settings WHERE key = ?1");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, key)) {
        return error;
    }

    auto completed = statement.value().step();
    if (!completed.ok()) {
        return completed.error();
    }
    return {};
}

}  // namespace dayflow
