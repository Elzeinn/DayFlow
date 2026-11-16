#include "core/task.hpp"

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


std::string to_string(TaskStatus taskstatus)
{
    switch (taskstatus) {
        case TaskStatus::todo:
            return "To Do";
        case TaskStatus::doing:
            return "Doing";
        case TaskStatus::done:
            return "Done";
        case TaskStatus::cancelled:
            return "Cancelled";
    }
    return "Unknown";
}

std::string to_string(Priority priority)
{
    switch (priority) {
        case Priority::low:
            return "Low";
        case Priority::normal:
            return "Normal";
        case Priority::high:
            return "High";
        case Priority::urgent:
            return "Urgent";
    }
    return "Unknown";
}

Error validate(const Task& task)
{
    const std::string_view title = trim(task.title);
    if (title.empty()) {
        return make_error(ErrorCode::invalid_argument, "judul task tidak boleh kosong");
    }
    if (title.size() > k_max_title_length) {
        return make_error(ErrorCode::invalid_argument, "judul task melebihi 200 karakter");
    }
    if (task.status == TaskStatus::done && !task.completed_at.has_value()) {
        return make_error(ErrorCode::invalid_argument, "task berstatus selesai harus punya completed_at");
    }
    if (task.completed_at.has_value() && task.status != TaskStatus::done) {
        return make_error(ErrorCode::invalid_argument, "completed_at hanya boleh diisi untuk task selesai");
    }
    return {};
}

bool is_finished(TaskStatus status)
{
    return status == TaskStatus::done || status == TaskStatus::cancelled;
}

}  // namespace dayflow
