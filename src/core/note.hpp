#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "core/error.hpp"
#include "core/timestamp.hpp"

namespace dayflow {

struct Note {
    std::int64_t id = 0;
    std::string body;
    bool pinned = false;
    Timestamp created_at{};
    Timestamp updated_at{};
};

inline constexpr std::size_t k_max_note_body = 20000;

Error validate(const Note& note);
std::string note_title(std::string_view body, std::size_t limit = 40);

}  // namespace dayflow
