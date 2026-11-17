#include "ui/calendar_page.hpp"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <algorithm>

#include "core/recurrence.hpp"
#include "data/event_repository.hpp"
#include "ui/day_cell.hpp"
#include "ui/event_dialog.hpp"
#include "ui/select.hpp"
#include "ui/theme.hpp"
#include "ui/time_util.hpp"

namespace dayflow {
namespace {

constexpr int k_grid_days = 42;
constexpr int k_year_span = 5;

QString weekday_code(int iso_weekday)
{
    static const char* names[] = {"SEN", "SEL", "RAB", "KAM", "JUM", "SAB", "MIN"};
    if (iso_weekday < 1 || iso_weekday > 7) {
        return {};
    }
    return QString::fromLatin1(names[iso_weekday - 1]);
}

QDate qdate_of(std::chrono::sys_days day)
{
    return QDate{1970, 1, 1}.addDays(static_cast<int>(day.time_since_epoch().count()));
}

}  // namespace

CalendarPage::CalendarPage(EventRepository& events, EventCategoryRepository& categories, QWidget* parent)
    : QWidget(parent), events_(events), categories_(categories)
{
    const QDate today = QDate::currentDate();
    visible_month_ = QDate{today.year(), today.month(), 1};
    selected_date_ = today;
    year_base_ = today.year() - k_year_span;

    load_categories();

    auto* layout = new QHBoxLayout{this};
    layout->setContentsMargins(28, 22, 28, 22);
    layout->setSpacing(16);

    auto* left = new QVBoxLayout{};
    left->setSpacing(10);
    left->addWidget(build_toolbar());

    auto* weekdays = new QHBoxLayout{};
    weekdays->setSpacing(6);
    for (int day = 1; day <= 7; ++day) {
        auto* label = theme::make_micro_label(weekday_code(day));
        label->setAlignment(Qt::AlignCenter);
        weekdays->addWidget(label, 1);
    }
    left->addLayout(weekdays);

    grid_ = new QGridLayout{};
    grid_->setSpacing(6);
    for (int column = 0; column < 7; ++column) {
        grid_->setColumnStretch(column, 1);
    }
    for (int row = 0; row < 6; ++row) {
        grid_->setRowStretch(row, 1);
    }
    left->addLayout(grid_, 1);

    layout->addLayout(left, 1);
    layout->addWidget(build_agenda_panel());

    rebuild();
}

void CalendarPage::showEvent(QShowEvent*)
{
    if (grid_ != nullptr) {
        rebuild();
    }
}

QWidget* CalendarPage::build_toolbar()
{
    auto* toolbar = new QWidget{};
    auto* layout = new QHBoxLayout{toolbar};
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    const QLocale indonesian{QLocale::Indonesian, QLocale::Indonesia};

    month_select_ = new Select{};
    month_select_->setMinimumWidth(148);
    for (int month = 1; month <= 12; ++month) {
        month_select_->add_item(indonesian.monthName(month, QLocale::LongFormat), month);
    }

    year_select_ = new Select{};
    year_select_->setMinimumWidth(96);
    for (int year = year_base_; year <= year_base_ + k_year_span * 2; ++year) {
        year_select_->add_item(QString::number(year), year);
    }

    auto* today_button = new QPushButton{QStringLiteral("HARI INI")};
    today_button->setObjectName(QStringLiteral("secondary"));
    auto* add_button = new QPushButton{QStringLiteral("+ EVENT")};
    add_button->setObjectName(QStringLiteral("primary"));

    layout->addWidget(month_select_);
    layout->addWidget(year_select_);
    layout->addStretch(1);
    layout->addWidget(today_button);
    layout->addWidget(add_button);

    connect(month_select_, &Select::changed, this, [this](int) {
        const int month = month_select_->current_data().toInt();
        visible_month_ = QDate{visible_month_.year(), month, 1};
        rebuild();
    });
    connect(year_select_, &Select::changed, this, [this](int) {
        const int year = year_select_->current_data().toInt();
        visible_month_ = QDate{year, visible_month_.month(), 1};
        rebuild();
    });
    connect(today_button, &QPushButton::clicked, this, &CalendarPage::go_today);
    connect(add_button, &QPushButton::clicked, this, &CalendarPage::add_event);

    return toolbar;
}

QWidget* CalendarPage::build_agenda_panel()
{
    auto* panel = new QFrame{};
    panel->setObjectName(QStringLiteral("card"));
    panel->setFixedWidth(268);

    auto* layout = new QVBoxLayout{panel};
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    layout->addWidget(theme::make_micro_label(QStringLiteral("AGENDA")));

    agenda_title_ = new QLabel{};
    agenda_title_->setObjectName(QStringLiteral("agendaTitle"));
    layout->addWidget(agenda_title_);

    agenda_layout_ = new QVBoxLayout{};
    agenda_layout_->setContentsMargins(0, 6, 0, 0);
    agenda_layout_->setSpacing(6);
    layout->addLayout(agenda_layout_, 1);

    return panel;
}

void CalendarPage::sync_toolbar()
{
    if (month_select_ != nullptr) {
        const QSignalBlocker blocker{month_select_};
        month_select_->set_current_index(visible_month_.month() - 1);
    }
    if (year_select_ != nullptr) {
        const QSignalBlocker blocker{year_select_};
        const int index = year_select_->index_of_data(visible_month_.year());
        if (index >= 0) {
            year_select_->set_current_index(index);
        }
    }
}

void CalendarPage::go_today()
{
    const QDate today = QDate::currentDate();
    visible_month_ = QDate{today.year(), today.month(), 1};
    selected_date_ = today;
    rebuild();
}

void CalendarPage::load_categories()
{
    category_colors_.clear();
    auto listed = categories_.list_all();
    if (!listed.ok()) {
        report(listed.error());
        return;
    }
    for (const EventCategory& category : listed.value()) {
        category_colors_.insert(category.id, QColor{QString::fromStdString(category.color)});
    }
}

QColor CalendarPage::color_for(std::optional<std::int64_t> category_id) const
{
    if (category_id.has_value()) {
        const auto found = category_colors_.constFind(*category_id);
        if (found != category_colors_.constEnd()) {
            return found.value();
        }
    }
    return QColor{QStringLiteral("#8A8A8A")};
}

void CalendarPage::load_occurrences(const QDate& from, const QDate& to)
{
    occurrences_.clear();
    holidays_.clear();

    const QDateTime range_begin_local{from, QTime{0, 0}};
    const QDateTime range_finish_local{to.addDays(1), QTime{0, 0}};
    const Timestamp range_start = utc_from_local(range_begin_local);
    const Timestamp range_end = utc_from_local(range_finish_local);

    const auto holiday_from = std::chrono::floor<std::chrono::days>(range_start);
    const auto holiday_to = std::chrono::floor<std::chrono::days>(range_end - std::chrono::seconds{1});
    holidays_ = holidays_in_range(holiday_from, holiday_to);

    auto events = events_.list_for_range(range_start, range_end);
    if (!events.ok()) {
        report(events.error());
        return;
    }

    for (const Event& event : events.value()) {
        auto expanded = expand_occurrences(event, range_start, range_end);
        if (!expanded.ok()) {
            continue;
        }
        for (const Occurrence& occurrence : expanded.value()) {
            OccurrenceView view;
            view.event_id = occurrence.event_id;
            view.title = QString::fromStdString(event.title);
            view.starts_at = local_from_utc(occurrence.starts_at);
            view.ends_at = local_from_utc(occurrence.ends_at);
            view.all_day = event.all_day;
            view.color = color_for(event.category_id);
            occurrences_.push_back(view);
        }
    }

    std::sort(occurrences_.begin(), occurrences_.end(),
        [](const OccurrenceView& left, const OccurrenceView& right) { return left.starts_at < right.starts_at; });
}

void CalendarPage::rebuild()
{
    sync_toolbar();

    while (QLayoutItem* item = grid_->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    const QDate first{visible_month_.year(), visible_month_.month(), 1};
    const int leading = first.dayOfWeek() - 1;
    const QDate grid_start = first.addDays(-leading);

    load_occurrences(grid_start, grid_start.addDays(k_grid_days - 1));

    for (int index = 0; index < k_grid_days; ++index) {
        const QDate date = grid_start.addDays(index);

        auto* cell = new DayCell{};
        cell->configure(date, date.month() == visible_month_.month(), date == QDate::currentDate(),
            date == selected_date_);

        QVector<DayEntry> entries;
        for (const Holiday& holiday : holidays_) {
            if (qdate_of(holiday.day) != date) {
                continue;
            }
            entries.push_back(
                DayEntry{QStringLiteral("LIBUR"), QString::fromStdString(holiday.name), QColor{0xE5, 0xC1, 0x58}});
        }
        for (const OccurrenceView& item : occurrences_) {
            if (item.starts_at.date() != date) {
                continue;
            }
            entries.push_back(DayEntry{item.all_day ? QString{} : item.starts_at.toString(QStringLiteral("HH:mm")),
                item.title, item.color});
        }
        cell->set_entries(entries);

        connect(cell, &DayCell::clicked, this, &CalendarPage::select_date);
        grid_->addWidget(cell, index / 7, index % 7);
    }

    refresh_agenda();
}

void CalendarPage::select_date(const QDate& date)
{
    selected_date_ = date;
    if (date.year() != visible_month_.year() || date.month() != visible_month_.month()) {
        visible_month_ = QDate{date.year(), date.month(), 1};
    }
    rebuild();
}

void CalendarPage::refresh_agenda()
{
    while (QLayoutItem* item = agenda_layout_->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    agenda_title_->setText(
        QLocale{QLocale::Indonesian, QLocale::Indonesia}.toString(selected_date_, QStringLiteral("dddd, d MMMM")));

    int shown = 0;

    for (const Holiday& holiday : holidays_) {
        if (qdate_of(holiday.day) != selected_date_) {
            continue;
        }
        auto* label = new QLabel{QStringLiteral("Libur nasional  ·  %1").arg(QString::fromStdString(holiday.name))};
        label->setObjectName(QStringLiteral("agendaHoliday"));
        label->setWordWrap(true);
        agenda_layout_->addWidget(label);
        ++shown;
    }

    for (const OccurrenceView& view : occurrences_) {
        if (view.starts_at.date() != selected_date_) {
            continue;
        }

        const QString label = view.all_day
            ? QStringLiteral("Sepanjang hari  ·  %1").arg(view.title)
            : QStringLiteral("%1  ·  %2").arg(view.starts_at.toString(QStringLiteral("HH:mm")), view.title);

        auto* row = new QPushButton{label};
        row->setObjectName(QStringLiteral("agendaItem"));
        row->setCursor(Qt::PointingHandCursor);
        row->setStyleSheet(QStringLiteral("border-left: 2px solid %1;").arg(view.color.name()));
        row->setToolTip(QStringLiteral("Klik untuk edit"));

        const std::int64_t event_id = view.event_id;
        connect(row, &QPushButton::clicked, this, [this, event_id] { open_event(event_id); });

        agenda_layout_->addWidget(row);
        ++shown;
    }

    if (shown == 0) {
        auto* empty = new QLabel{QStringLiteral("Tidak ada event")};
        empty->setObjectName(QStringLiteral("agendaEmpty"));
        agenda_layout_->addWidget(empty);
    }

    agenda_layout_->addStretch(1);
}

void CalendarPage::add_event()
{
    EventDialog dialog{categories_, std::nullopt, selected_date_, this};
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    auto created = events_.create(dialog.result());
    if (!created.ok()) {
        report(created.error());
        return;
    }
    rebuild();
}

void CalendarPage::open_event(std::int64_t id)
{
    auto existing = events_.find(id);
    if (!existing.ok()) {
        report(existing.error());
        return;
    }
    if (!existing.value().has_value()) {
        rebuild();
        return;
    }

    EventDialog dialog{categories_, existing.value(), selected_date_, this};
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    if (dialog.wants_delete()) {
        if (Error error = events_.remove(id)) {
            report(error);
            return;
        }
        rebuild();
        return;
    }

    Event updated = dialog.result();
    updated.id = id;
    updated.created_at = existing.value()->created_at;
    if (Error error = events_.update(updated)) {
        report(error);
        return;
    }
    rebuild();
}

void CalendarPage::report(const Error& error)
{
    QMessageBox::warning(this, QStringLiteral("DayFlow"), QString::fromStdString(error.message));
}

}  // namespace dayflow
