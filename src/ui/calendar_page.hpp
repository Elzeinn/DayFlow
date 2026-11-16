#pragma once

#include <QColor>
#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QWidget>

#include <cstdint>
#include <vector>

#include "core/error.hpp"
#include "core/event.hpp"
#include "core/holidays.hpp"

class QGridLayout;
class QLabel;
class QVBoxLayout;

namespace dayflow {

class EventCategoryRepository;
class EventRepository;
class Select;

struct OccurrenceView {
    std::int64_t event_id = 0;
    QString title;
    QDateTime starts_at;
    QDateTime ends_at;
    QColor color;
    bool all_day = false;
};

class CalendarPage : public QWidget {
    Q_OBJECT

public:
    CalendarPage(EventRepository& events, EventCategoryRepository& categories, QWidget* parent = nullptr);

private:
    QWidget* build_toolbar();
    QWidget* build_agenda_panel();

    void rebuild();
    void load_occurrences(const QDate& from, const QDate& to);
    void load_categories();
    void select_date(const QDate& date);
    void refresh_agenda();
    void sync_toolbar();
    void go_today();
    void add_event();
    void open_event(std::int64_t id);
    void report(const Error& error);
    QColor color_for(std::optional<std::int64_t> category_id) const;

    EventRepository& events_;
    EventCategoryRepository& categories_;

    Select* month_select_ = nullptr;
    Select* year_select_ = nullptr;
    QGridLayout* grid_ = nullptr;
    QLabel* agenda_title_ = nullptr;
    QVBoxLayout* agenda_layout_ = nullptr;

    QDate visible_month_;
    QDate selected_date_;
    int year_base_ = 0;
    std::vector<OccurrenceView> occurrences_;
    std::vector<Holiday> holidays_;
    QHash<qint64, QColor> category_colors_;
};

}  // namespace dayflow
