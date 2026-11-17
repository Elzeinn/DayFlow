#include "data/event_repository.hpp"

#include <charconv>
#include <string>
#include <string_view>

#include "data/database.hpp"

namespace dayflow {
namespace {

constexpr char k_event_select[] =
    "SELECT e.id, e.title, e.description, e.category_id, e.starts_at_utc, e.ends_at_utc, e.all_day, "
    "r.id, r.freq, r.interval, r.by_weekday, r.by_monthday, r.count, r.until_utc, "
    "e.created_at, e.updated_at FROM events e LEFT JOIN recurrence_rules r ON r.id = e.recurrence_id";

const char* freq_to_text(RecurrenceFreq freq)
{
    switch (freq) {
    case RecurrenceFreq::none:
        return "none";
    case RecurrenceFreq::daily:
        return "daily";
    case RecurrenceFreq::weekly:
        return "weekly";
    case RecurrenceFreq::monthly:
        return "monthly";
    case RecurrenceFreq::yearly:
        return "yearly";
    }
    return "none";
}

Result<RecurrenceFreq> freq_from_text(std::string_view text)
{
    if (text == "daily") {
        return RecurrenceFreq::daily;
    }
    if (text == "weekly") {
        return RecurrenceFreq::weekly;
    }
    if (text == "monthly") {
        return RecurrenceFreq::monthly;
    }
    if (text == "yearly") {
        return RecurrenceFreq::yearly;
    }
    if (text == "none") {
        return RecurrenceFreq::none;
    }
    return make_error(ErrorCode::database, "frekuensi pengulangan tidak dikenal: " + std::string{text});
}

std::string join_ints(const std::vector<int>& values)
{
    std::string joined;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            joined += ',';
        }
        joined += std::to_string(values[index]);
    }
    return joined;
}

Result<std::vector<int>> split_ints(std::string_view text)
{
    std::vector<int> values;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t comma = text.find(',', start);
        const std::string_view part =
            comma == std::string_view::npos ? text.substr(start) : text.substr(start, comma - start);
        if (!part.empty()) {
            int value = 0;
            const auto result = std::from_chars(part.data(), part.data() + part.size(), value);
            if (result.ec != std::errc{} || result.ptr != part.data() + part.size()) {
                return make_error(ErrorCode::database, "nilai daftar pengulangan tidak valid: " + std::string{part});
            }
            values.push_back(value);
        }
        if (comma == std::string_view::npos) {
            break;
        }
        start = comma + 1;
    }
    return values;
}

Result<Timestamp> required_timestamp(Statement& statement, int index, const char* name)
{
    auto value = statement.column_optional_timestamp(index);
    if (!value.ok()) {
        return value.error();
    }
    if (!value.value().has_value()) {
        return make_error(ErrorCode::database, std::string{"kolom "} + name + " kosong pada tabel events");
    }
    return *value.value();
}

Result<Event> read_event(Statement& statement)
{
    Event event;
    event.id = statement.column_int64(0);
    event.title = statement.column_text(1);
    event.description = statement.column_text(2);
    event.category_id = statement.column_optional_int64(3);

    auto starts_at = required_timestamp(statement, 4, "starts_at_utc");
    if (!starts_at.ok()) {
        return starts_at.error();
    }
    event.starts_at = starts_at.value();

    auto ends_at = required_timestamp(statement, 5, "ends_at_utc");
    if (!ends_at.ok()) {
        return ends_at.error();
    }
    event.ends_at = ends_at.value();

    event.all_day = statement.column_int(6) != 0;

    if (!statement.is_null(7)) {
        RecurrenceRule rule;

        const std::string freq_text = statement.column_text(8);
        auto freq = freq_from_text(freq_text);
        if (!freq.ok()) {
            return freq.error();
        }
        rule.freq = freq.value();
        rule.interval = statement.column_int(9);

        auto weekdays = split_ints(statement.column_text(10));
        if (!weekdays.ok()) {
            return weekdays.error();
        }
        rule.by_weekday = std::move(weekdays.value());

        auto monthdays = split_ints(statement.column_text(11));
        if (!monthdays.ok()) {
            return monthdays.error();
        }
        rule.by_monthday = std::move(monthdays.value());

        rule.count = statement.column_optional_int(12);

        auto until = statement.column_optional_timestamp(13);
        if (!until.ok()) {
            return until.error();
        }
        rule.until = until.value();

        event.recurrence = rule;
    }

    auto created_at = required_timestamp(statement, 14, "created_at");
    if (!created_at.ok()) {
        return created_at.error();
    }
    event.created_at = created_at.value();

    auto updated_at = required_timestamp(statement, 15, "updated_at");
    if (!updated_at.ok()) {
        return updated_at.error();
    }
    event.updated_at = updated_at.value();

    return event;
}

Result<std::int64_t> insert_rule(Database& database, const RecurrenceRule& rule)
{
    auto statement = database.prepare(
        "INSERT INTO recurrence_rules (freq, interval, by_weekday, by_monthday, count, until_utc) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6)");
    if (!statement.ok()) {
        return statement.error();
    }

    Statement& insert = statement.value();
    if (Error error = insert.bind(1, freq_to_text(rule.freq))) {
        return error;
    }
    if (Error error = insert.bind(2, rule.interval)) {
        return error;
    }
    if (Error error = insert.bind(3, join_ints(rule.by_weekday))) {
        return error;
    }
    if (Error error = insert.bind(4, join_ints(rule.by_monthday))) {
        return error;
    }
    if (rule.count.has_value()) {
        if (Error error = insert.bind(5, *rule.count)) {
            return error;
        }
    } else if (Error error = insert.bind_null(5)) {
        return error;
    }
    if (rule.until.has_value()) {
        if (Error error = insert.bind(6, to_iso8601(*rule.until))) {
            return error;
        }
    } else if (Error error = insert.bind_null(6)) {
        return error;
    }

    auto completed = insert.step();
    if (!completed.ok()) {
        return completed.error();
    }
    return database.last_insert_rowid();
}

Result<std::optional<std::int64_t>> rule_id_of(Database& database, std::int64_t event_id)
{
    auto statement = database.prepare("SELECT recurrence_id FROM events WHERE id = ?1");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, event_id)) {
        return error;
    }
    auto row = statement.value().step();
    if (!row.ok()) {
        return row.error();
    }
    if (!row.value()) {
        return std::optional<std::int64_t>{};
    }
    return statement.value().column_optional_int64(0);
}

}  // namespace

Result<std::vector<Event>> EventRepository::list_for_range(Timestamp range_start, Timestamp range_end)
{
    auto guard = database_.lock();
    auto statement = database_.prepare(
        std::string{k_event_select}
        + " WHERE (e.recurrence_id IS NOT NULL AND e.starts_at_utc < ?2) "
          "OR (e.starts_at_utc < ?2 AND e.ends_at_utc > ?1) ORDER BY e.starts_at_utc");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, to_iso8601(range_start))) {
        return error;
    }
    if (Error error = statement.value().bind(2, to_iso8601(range_end))) {
        return error;
    }

    std::vector<Event> events;
    for (;;) {
        auto row = statement.value().step();
        if (!row.ok()) {
            return row.error();
        }
        if (!row.value()) {
            break;
        }
        auto event = read_event(statement.value());
        if (!event.ok()) {
            return event.error();
        }
        events.push_back(std::move(event.value()));
    }
    return events;
}

Result<std::vector<Event>> EventRepository::search(std::string_view query, std::size_t limit)
{
    auto guard = database_.lock();
    auto statement = database_.prepare(std::string{k_event_select}
        + " WHERE e.title LIKE ?1 ESCAPE '\\' ORDER BY e.starts_at_utc DESC LIMIT ?2");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, like_pattern(query))) {
        return error;
    }
    if (Error error = statement.value().bind(2, static_cast<int>(limit))) {
        return error;
    }

    std::vector<Event> events;
    for (;;) {
        auto row = statement.value().step();
        if (!row.ok()) {
            return row.error();
        }
        if (!row.value()) {
            break;
        }
        auto event = read_event(statement.value());
        if (!event.ok()) {
            return event.error();
        }
        events.push_back(std::move(event.value()));
    }
    return events;
}

Result<std::optional<Event>> EventRepository::find(std::int64_t id)
{
    auto guard = database_.lock();
    auto statement = database_.prepare(std::string{k_event_select} + " WHERE e.id = ?1");
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
        return std::optional<Event>{};
    }

    auto event = read_event(statement.value());
    if (!event.ok()) {
        return event.error();
    }
    return std::optional<Event>{std::move(event.value())};
}

Result<Event> EventRepository::create(Event event)
{
    if (Error error = validate(event)) {
        return error;
    }

    const Timestamp now = now_utc();
    if (event.created_at == Timestamp{}) {
        event.created_at = now;
    }
    event.updated_at = now;

    auto guard = database_.lock();
    Transaction transaction{database_};

    std::optional<std::int64_t> rule_id;
    if (is_recurring(event)) {
        auto inserted_rule = insert_rule(database_, *event.recurrence);
        if (!inserted_rule.ok()) {
            return inserted_rule.error();
        }
        rule_id = inserted_rule.value();
    }

    auto statement = database_.prepare(
        "INSERT INTO events (title, description, category_id, starts_at_utc, ends_at_utc, all_day, "
        "recurrence_id, created_at, updated_at) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)");
    if (!statement.ok()) {
        return statement.error();
    }

    Statement& insert = statement.value();
    if (Error error = insert.bind(1, event.title)) {
        return error;
    }
    if (Error error = insert.bind(2, event.description)) {
        return error;
    }
    if (event.category_id.has_value()) {
        if (Error error = insert.bind(3, *event.category_id)) {
            return error;
        }
    } else if (Error error = insert.bind_null(3)) {
        return error;
    }
    if (Error error = insert.bind(4, to_iso8601(event.starts_at))) {
        return error;
    }
    if (Error error = insert.bind(5, to_iso8601(event.ends_at))) {
        return error;
    }
    if (Error error = insert.bind(6, event.all_day ? 1 : 0)) {
        return error;
    }
    if (rule_id.has_value()) {
        if (Error error = insert.bind(7, *rule_id)) {
            return error;
        }
    } else if (Error error = insert.bind_null(7)) {
        return error;
    }
    if (Error error = insert.bind(8, to_iso8601(event.created_at))) {
        return error;
    }
    if (Error error = insert.bind(9, to_iso8601(event.updated_at))) {
        return error;
    }

    auto completed = insert.step();
    if (!completed.ok()) {
        return completed.error();
    }

    auto inserted_id = database_.last_insert_rowid();
    if (!inserted_id.ok()) {
        return inserted_id.error();
    }
    event.id = inserted_id.value();

    if (Error error = transaction.commit()) {
        return error;
    }
    return event;
}

Error EventRepository::update(Event event)
{
    if (event.id <= 0) {
        return make_error(ErrorCode::invalid_argument, "event tanpa id tidak bisa diupdate");
    }
    if (Error error = validate(event)) {
        return error;
    }
    event.updated_at = now_utc();

    auto guard = database_.lock();
    Transaction transaction{database_};

    auto previous_rule = rule_id_of(database_, event.id);
    if (!previous_rule.ok()) {
        return previous_rule.error();
    }

    std::optional<std::int64_t> new_rule_id;
    if (is_recurring(event)) {
        auto inserted_rule = insert_rule(database_, *event.recurrence);
        if (!inserted_rule.ok()) {
            return inserted_rule.error();
        }
        new_rule_id = inserted_rule.value();
    }

    auto statement = database_.prepare(
        "UPDATE events SET title = ?1, description = ?2, category_id = ?3, starts_at_utc = ?4, "
        "ends_at_utc = ?5, all_day = ?6, recurrence_id = ?7, updated_at = ?8 WHERE id = ?9");
    if (!statement.ok()) {
        return statement.error();
    }

    Statement& update = statement.value();
    if (Error error = update.bind(1, event.title)) {
        return error;
    }
    if (Error error = update.bind(2, event.description)) {
        return error;
    }
    if (event.category_id.has_value()) {
        if (Error error = update.bind(3, *event.category_id)) {
            return error;
        }
    } else if (Error error = update.bind_null(3)) {
        return error;
    }
    if (Error error = update.bind(4, to_iso8601(event.starts_at))) {
        return error;
    }
    if (Error error = update.bind(5, to_iso8601(event.ends_at))) {
        return error;
    }
    if (Error error = update.bind(6, event.all_day ? 1 : 0)) {
        return error;
    }
    if (new_rule_id.has_value()) {
        if (Error error = update.bind(7, *new_rule_id)) {
            return error;
        }
    } else if (Error error = update.bind_null(7)) {
        return error;
    }
    if (Error error = update.bind(8, to_iso8601(event.updated_at))) {
        return error;
    }
    if (Error error = update.bind(9, event.id)) {
        return error;
    }

    auto completed = update.step();
    if (!completed.ok()) {
        return completed.error();
    }
    if (database_.changes() == 0) {
        return make_error(ErrorCode::not_found, "event dengan id " + std::to_string(event.id) + " tidak ditemukan");
    }

    if (previous_rule.value().has_value()) {
        if (Error error = database_.exec("DELETE FROM recurrence_rules WHERE id = "
                + std::to_string(*previous_rule.value()))) {
            return error;
        }
    }

    return transaction.commit();
}

Error EventRepository::remove(std::int64_t id)
{
    auto guard = database_.lock();
    Transaction transaction{database_};

    auto previous_rule = rule_id_of(database_, id);
    if (!previous_rule.ok()) {
        return previous_rule.error();
    }

    auto statement = database_.prepare("DELETE FROM events WHERE id = ?1");
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
        return make_error(ErrorCode::not_found, "event dengan id " + std::to_string(id) + " tidak ditemukan");
    }

    if (previous_rule.value().has_value()) {
        if (Error error = database_.exec("DELETE FROM recurrence_rules WHERE id = "
                + std::to_string(*previous_rule.value()))) {
            return error;
        }
    }

    return transaction.commit();
}

Result<std::vector<EventCategory>> EventCategoryRepository::list_all()
{
    auto guard = database_.lock();
    auto statement = database_.prepare("SELECT id, name, color, sort_order FROM event_categories ORDER BY sort_order, name");
    if (!statement.ok()) {
        return statement.error();
    }

    std::vector<EventCategory> categories;
    for (;;) {
        auto row = statement.value().step();
        if (!row.ok()) {
            return row.error();
        }
        if (!row.value()) {
            break;
        }
        EventCategory category;
        category.id = statement.value().column_int64(0);
        category.name = statement.value().column_text(1);
        category.color = statement.value().column_text(2);
        category.sort_order = statement.value().column_int(3);
        categories.push_back(std::move(category));
    }
    return categories;
}

Result<EventCategory> EventCategoryRepository::create(EventCategory category)
{
    if (category.name.empty()) {
        return make_error(ErrorCode::invalid_argument, "nama kategori tidak boleh kosong");
    }

    auto guard = database_.lock();
    auto statement = database_.prepare(
        "INSERT INTO event_categories (name, color, sort_order) VALUES (?1, ?2, ?3)");
    if (!statement.ok()) {
        return statement.error();
    }
    if (Error error = statement.value().bind(1, category.name)) {
        return error;
    }
    if (Error error = statement.value().bind(2, category.color)) {
        return error;
    }
    if (Error error = statement.value().bind(3, category.sort_order)) {
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
    category.id = inserted_id.value();
    return category;
}

Error EventCategoryRepository::remove(std::int64_t id)
{
    auto guard = database_.lock();
    auto statement = database_.prepare("DELETE FROM event_categories WHERE id = ?1");
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
        return make_error(ErrorCode::not_found, "kategori dengan id " + std::to_string(id) + " tidak ditemukan");
    }
    return {};
}

Error EventCategoryRepository::ensure_defaults()
{
    const EventCategory defaults[] = {
        {0, "Pribadi", "#BEEE11", 0},
        {0, "Kerja", "#5AA9FF", 1},
        {0, "Kesehatan", "#6FE3A1", 2},
        {0, "Sosial", "#FF9F45", 3},
    };

    auto guard = database_.lock();
    for (const EventCategory& category : defaults) {
        auto statement = database_.prepare(
            "INSERT INTO event_categories (name, color, sort_order) VALUES (?1, ?2, ?3) "
            "ON CONFLICT(name) DO NOTHING");
        if (!statement.ok()) {
            return statement.error();
        }
        if (Error error = statement.value().bind(1, category.name)) {
            return error;
        }
        if (Error error = statement.value().bind(2, category.color)) {
            return error;
        }
        if (Error error = statement.value().bind(3, category.sort_order)) {
            return error;
        }
        auto completed = statement.value().step();
        if (!completed.ok()) {
            return completed.error();
        }
    }
    return {};
}

}  // namespace dayflow
