#include "core/event.hpp"

#include <cctype>
#include <string_view>

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

Error validate(const Event& event)
{
    const std::string_view title = trim(event.title);
    if (title.empty()) {
        return make_error(ErrorCode::invalid_argument, "judul event tidak boleh kosong");
    }
    if (title.size() > k_max_event_title_length) {
        return make_error(ErrorCode::invalid_argument, "judul event melebihi 200 karakter");
    }
    if (event.ends_at < event.starts_at) {
        return make_error(ErrorCode::invalid_argument, "waktu selesai tidak boleh sebelum waktu mulai");
    }
    if (event.recurrence.has_value()) {
        const RecurrenceRule& rule = *event.recurrence;
        if (rule.freq == RecurrenceFreq::none) {
            return make_error(ErrorCode::invalid_argument, "aturan pengulangan tanpa frekuensi");
        }
        if (rule.interval < 1) {
            return make_error(ErrorCode::invalid_argument, "interval pengulangan minimal 1");
        }
        if (rule.count.has_value() && *rule.count < 1) {
            return make_error(ErrorCode::invalid_argument, "jumlah pengulangan minimal 1");
        }
        for (int weekday : rule.by_weekday) {
            if (weekday < 1 || weekday > 7) {
                return make_error(ErrorCode::invalid_argument, "hari dalam minggu harus 1..7 (Senin..Minggu)");
            }
        }
        for (int monthday : rule.by_monthday) {
            if (monthday == -1) {
                continue;
            }
            if (monthday < 1 || monthday > 31) {
                return make_error(ErrorCode::invalid_argument, "tanggal dalam bulan harus 1..31 atau -1");
            }
        }
    }
    return {};
}

bool is_recurring(const Event& event)
{
    return event.recurrence.has_value() && event.recurrence->freq != RecurrenceFreq::none;
}

}  // namespace dayflow
