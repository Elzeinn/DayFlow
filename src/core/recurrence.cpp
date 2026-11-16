#include "core/recurrence.hpp"

#include <algorithm>
#include <chrono>

namespace dayflow {
namespace {

using namespace std::chrono;

constexpr int k_max_iterations = 200000;

bool contains(const std::vector<int>& values, int value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

sys_days day_of(Timestamp value)
{
    return floor<days>(value);
}

bool matches_rule(const RecurrenceRule& rule, sys_days first_day, sys_days candidate)
{
    switch (rule.freq) {
    case RecurrenceFreq::none:
        return false;
    case RecurrenceFreq::daily: {
        const auto difference = (candidate - first_day).count();
        return difference >= 0 && difference % rule.interval == 0;
    }
    case RecurrenceFreq::weekly: {
        const int candidate_weekday = static_cast<int>(weekday{candidate}.iso_encoding());
        const int first_weekday = static_cast<int>(weekday{first_day}.iso_encoding());
        const bool weekday_ok = rule.by_weekday.empty() ? candidate_weekday == first_weekday
                                                        : contains(rule.by_weekday, candidate_weekday);
        if (!weekday_ok) {
            return false;
        }
        const sys_days first_monday = first_day - days{first_weekday - 1};
        const sys_days candidate_monday = candidate - days{candidate_weekday - 1};
        const auto weeks = (candidate_monday - first_monday).count() / 7;
        return weeks >= 0 && weeks % rule.interval == 0;
    }
    case RecurrenceFreq::monthly: {
        const year_month_day first{first_day};
        const year_month_day current{candidate};
        const int months = (static_cast<int>(current.year()) - static_cast<int>(first.year())) * 12
            + static_cast<int>(static_cast<unsigned>(current.month()))
            - static_cast<int>(static_cast<unsigned>(first.month()));
        if (months < 0 || months % rule.interval != 0) {
            return false;
        }
        const int day_number = static_cast<int>(static_cast<unsigned>(current.day()));
        const year_month_day_last last_day{current.year(), month_day_last{current.month()}};
        const int last_number = static_cast<int>(static_cast<unsigned>(last_day.day()));

        if (rule.by_monthday.empty()) {
            const int anchor = static_cast<int>(static_cast<unsigned>(first.day()));
            return day_number == anchor;
        }
        for (int monthday : rule.by_monthday) {
            const int resolved = monthday == -1 ? last_number : monthday;
            if (resolved == day_number) {
                return true;
            }
        }
        return false;
    }
    case RecurrenceFreq::yearly: {
        const year_month_day first{first_day};
        const year_month_day current{candidate};
        const int years = static_cast<int>(current.year()) - static_cast<int>(first.year());
        if (years < 0 || years % rule.interval != 0) {
            return false;
        }
        return current.month() == first.month() && current.day() == first.day();
    }
    }
    return false;
}

}  // namespace

bool overlaps(const Occurrence& occurrence, Timestamp range_start, Timestamp range_end)
{
    return occurrence.starts_at < range_end && occurrence.ends_at > range_start;
}

Result<std::vector<Occurrence>> expand_occurrences(
    const Event& event, Timestamp range_start, Timestamp range_end, std::size_t limit)
{
    std::vector<Occurrence> occurrences;
    if (range_end <= range_start) {
        return occurrences;
    }
    if (Error error = validate(event)) {
        return error;
    }

    const seconds time_of_day = event.starts_at - Timestamp{day_of(event.starts_at)};
    const seconds length = event.ends_at - event.starts_at;

    if (!is_recurring(event)) {
        Occurrence single{event.id, event.starts_at, event.ends_at};
        if (overlaps(single, range_start, range_end)) {
            occurrences.push_back(single);
        }
        return occurrences;
    }

    const RecurrenceRule& rule = *event.recurrence;
    const sys_days first_day = day_of(event.starts_at);
    const sys_days last_day = day_of(range_end);

    int generated = 0;
    int iterations = 0;
    for (sys_days candidate = first_day; candidate <= last_day; candidate += days{1}) {
        if (++iterations > k_max_iterations) {
            return make_error(ErrorCode::invalid_argument, "pengulangan terlalu panjang untuk dievaluasi");
        }
        if (rule.until.has_value() && Timestamp{candidate} > *rule.until) {
            break;
        }
        if (!matches_rule(rule, first_day, candidate)) {
            continue;
        }

        ++generated;
        if (rule.count.has_value() && generated > *rule.count) {
            break;
        }

        const Timestamp start{candidate + time_of_day};
        if (start >= range_end) {
            break;
        }
        const Timestamp end = start + length;
        if (end > range_start) {
            occurrences.push_back(Occurrence{event.id, start, end});
            if (occurrences.size() >= limit) {
                break;
            }
        }
    }

    return occurrences;
}

}  // namespace dayflow
