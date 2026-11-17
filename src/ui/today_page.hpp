#pragma once

#include <QColor>
#include <QHash>
#include <QWidget>

#include <cstdint>

#include "core/error.hpp"

class QLabel;
class QVBoxLayout;

namespace dayflow {

class EventCategoryRepository;
class EventRepository;
class TaskRepository;

class TodayPage : public QWidget {
    Q_OBJECT

public:
    TodayPage(TaskRepository& tasks, EventRepository& events, EventCategoryRepository& categories,
        QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;

private:
    QWidget* make_stat_card(const QString& label, QLabel** value);
    QWidget* make_section(const QString& title, QVBoxLayout** body);
    void reload();
    void report(const Error& error);
    QColor color_for(std::optional<std::int64_t> category_id) const;

    TaskRepository& tasks_;
    EventRepository& events_;
    EventCategoryRepository& categories_;

    QLabel* stat_open_ = nullptr;
    QLabel* stat_events_ = nullptr;
    QLabel* stat_done_ = nullptr;
    QLabel* agenda_title_ = nullptr;
    QVBoxLayout* agenda_body_ = nullptr;
    QVBoxLayout* task_body_ = nullptr;
    QHash<qint64, QColor> category_colors_;
};

}  // namespace dayflow
