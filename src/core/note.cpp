#include "core/note.hpp"

#include <cctype>

namespace dayflow {
namespace {

std::string_view trim(std::string_view text)
{
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return text.substr(begin, end - begin);
}

}  // namespace

Error validate(const Note& note)
{
    if (trim(note.body).empty()) {
        return make_error(ErrorCode::invalid_argument, "catatan tidak boleh kosong");
    }
    if (note.body.size() > k_max_note_body) {
        return make_error(ErrorCode::invalid_argument, "catatan melebihi 20000 karakter");
    }
    return {};
}

std::string note_title(std::string_view body, std::size_t limit)
{
    const std::string_view trimmed = trim(body);
    if (trimmed.empty()) {
        return "Catatan kosong";
    }

    const std::size_t line_end = trimmed.find('\n');
    std::string_view first_line = line_end == std::string_view::npos ? trimmed : trimmed.substr(0, line_end);
    first_line = trim(first_line);

    if (first_line.size() <= limit) {
        return std::string{first_line};
    }
    std::string title{first_line.substr(0, limit)};
    title += "...";
    return title;
}

}  // namespace dayflow
