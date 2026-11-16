#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "core/error.hpp"

namespace dayflow {

class Database;

class SettingsRepository {
public:
    explicit SettingsRepository(Database& database) : database_(database) {}

    Result<std::optional<std::string>> get(std::string_view key);
    Error set(std::string_view key, std::string_view value);
    Error remove(std::string_view key);

private:
    Database& database_;
};

}  // namespace dayflow
