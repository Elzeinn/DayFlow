#include "ui/today_page.hpp"

#include <QDate>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

#include "core/holidays.hpp"
#include "core/recurrence.hpp"
#include "data/event_repository.hpp"
#include "data/task_repository.hpp"
#include "ui/event_dialog.hpp"
#include "ui/task_dialog.hpp"
#include "ui/task_row.hpp"
#include "ui/theme.hpp"
#include "ui/time_util.hpp"

namespace dayflow {
namespace {

constexpr int k_max_agenda_rows = 8;

void clear_layout(QVBoxLayout* layout)
{
    while (QLayoutItem* item = layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
}

}  // namespace

TodayPage::TodayPage(TaskRepository& tasks, EventRepository& events, EventCategoryRepository& categories,
    QWidget* parent)
    : QWidget(parent), tasks_(tasks), events_(events), categories_(categories)
{
    auto* layout = new QVBoxLayout{this};
    layout->setContentsMargins(28, 22, 28, 22);
    layout->setSpacing(18);

    auto* header = new QVBoxLayout{};
    header->setSpacing(6);
    const QLocale indonesian{QLocale::Indonesian, QLocale::Indonesia};
    header->addWidget(theme::make_micro_label(
        indonesian.toString(QDate::currentDate(), QStringLiteral("dddd, d MMMM yyyy")).toUpper()));

    auto* title = new QLabel{QStringLiteral("Hari Ini")};
    title->setObjectName(QStringLiteral("displayTitle"));
    title->setFont(theme::display_font());
    title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    title->setFixedHeight(40);
    header->addWidget(title);
    layout->addLayout(header);

    auto* stats = new QHBoxLayout{};
    stats->setSpacing(12);
    stats->addWidget(make_stat_card(QStringLiteral("TUGAS AKTIF"), &stat_open_), 1);
    stats->addWidget(make_stat_card(QStringLiteral("EVENT HARI INI"), &stat_events_), 1);
    stats->addWidget(make_stat_card(QStringLiteral("SELESAI HARI INI"), &stat_done_), 1);
    layout->addLayout(stats);

    auto* content = new QHBoxLayout{};
    content->setSpacing(16);

    QVBoxLayout* tasks_body = nullptr;
    content->addWidget(make_section(QStringLiteral("TUGAS HARI INI"), &tasks_body), 1);

    auto* agenda_card = new QFrame{};
    agenda_card->setObjectName(QStringLiteral("card"));
    agenda_card->setFixedWidth(300);
    auto* agenda_layout = new QVBoxLayout{agenda_card};
    agenda_layout->setContentsMargins(16, 16, 16, 16);
    agenda_layout->setSpacing(8);
    agenda_layout->addWidget(theme::make_micro_label(QStringLiteral("AGENDA")));

    agenda_title_ = new QLabel{};
    agenda_title_->setObjectName(QStringLiteral("agendaTitle"));
    agenda_title_->setWordWrap(true);
    agenda_layout->addWidget(agenda_title_);

    agenda_body_ = new QVBoxLayout{};
    agenda_body_->setContentsMargins(0, 6, 0, 0);
    agenda_body_->setSpacing(6);
    agenda_layout->addLayout(agenda_body_, 1);

    content->addWidget(agenda_card);
    layout->addLayout(content, 1);

    task_body_ = tasks_body;
    reload();
}

QWidget* TodayPage::make_stat_card(const QString& label, QLabel** value)
{
    auto* card = new QFrame{};
    card->setObjectName(QStringLiteral("card"));

    auto* layout = new QVBoxLayout{card};
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(6);
    layout->addWidget(theme::make_micro_label(label));

    auto* number = new QLabel{QStringLiteral("0")};
    number->setObjectName(QStringLiteral("statValue"));
    layout->addWidget(number);
    layout->addStretch(1);

    *value = number;
    return card;
}

QWidget* TodayPage::make_section(const QString& title, QVBoxLayout** body)
{
    auto* card = new QFrame{};
    card->setObjectName(QStringLiteral("card"));

    auto* layout = new QVBoxLayout{card};
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    layout->addWidget(theme::make_micro_label(title));

    auto* scroll = new QScrollArea{};
    scroll->setObjectName(QStringLiteral("taskScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* container = new QWidget{};
    container->setObjectName(QStringLiteral("taskScrollBody"));
    auto* body_layout = new QVBoxLayout{container};
    body_layout->setContentsMargins(0, 0, 0, 0);
    body_layout->setSpacing(6);

    scroll->setWidget(container);
    layout->addWidget(scroll, 1);

    *body = body_layout;
    return card;
}

QColor TodayPage::color_for(std::optional<std::int64_t> category_id) const
{
    if (category_id.has_value()) {
        const auto found = category_colors_.constFind(*category_id);
        if (found != category_colors_.constEnd()) {
            return found.value();
        }
    }
    return QColor{QStringLiteral("#8A8A8A")};
}

void TodayPage::showEvent(QShowEvent*)
{
    reload();
}

void TodayPage::reload()
{
    category_colors_.clear();
    auto categories = categories_.list_all();
    if (categories.ok()) {
        for (const EventCategory& category : categories.value()) {
            category_colors_.insert(category.id, QColor{QString::fromStdString(category.color)});
        }
    }

    const QDate today = QDate::currentDate();
    const QLocale indonesian{QLocale::Indonesian, QLocale::Indonesia};
    agenda_title_->setText(indonesian.toString(today, QStringLiteral("dddd, d MMMM yyyy")));

    clear_layout(agenda_body_);
    clear_layout(task_body_);

    const QDateTime start_of_day{today, QTime{0, 0}};
    const Timestamp range_start = utc_from_local(start_of_day);
    const Timestamp range_end = utc_from_local(QDateTime{today.addDays(1), QTime{0, 0}});

    int agenda_rows = 0;

    for (const Holiday& holiday : holidays_in_range(std::chrono::floor<std::chrono::days>(range_start),
             std::chrono::floor<std::chrono::days>(range_start))) {
        auto* label = new QLabel{QStringLiteral("Libur nasional  ·  %1").arg(QString::fromStdString(holiday.name))};
        label->setObjectName(QStringLiteral("agendaHoliday"));
        label->setWordWrap(true);
        agenda_body_->addWidget(label);
    }

    int event_count = 0;
    auto events = events_.list_for_range(range_start, range_end);
    if (!events.ok()) {
        report(events.error());
    } else {
        std::vector<Occurrence> occurrences;
        for (const Event& event : events.value()) {
            auto expanded = expand_occurrences(event, range_start, range_end);
            if (!expanded.ok()) {
                continue;
            }
            for (const Occurrence& occurrence : expanded.value()) {
                occurrences.push_back(occurrence);
            }
        }
        std::sort(occurrences.begin(), occurrences.end(),
            [](const Occurrence& left, const Occurrence& right) { return left.starts_at < right.starts_at; });

        for (const Occurrence& occurrence : occurrences) {
            ++event_count;
            if (agenda_rows >= k_max_agenda_rows) {
                continue;
            }
            const Event* source = nullptr;
            for (const Event& event : events.value()) {
                if (event.id == occurrence.event_id) {
                    source = &event;
                    break;
                }
            }
            if (source == nullptr) {
                continue;
            }

            const QDateTime local = local_from_utc(occurrence.starts_at);
            const QString text = source->all_day
                ? QStringLiteral("Sepanjang hari  ·  %1").arg(QString::fromStdString(source->title))
                : QStringLiteral("%1  ·  %2").arg(local.toString(QStringLiteral("HH:mm")),
                      QString::fromStdString(source->title));

            auto* row = new QPushButton{text};
            row->setObjectName(QStringLiteral("agendaItem"));
            row->setCursor(Qt::PointingHandCursor);
            row->setStyleSheet(QStringLiteral("border-left: 2px solid %1;").arg(color_for(source->category_id).name()));

            const std::int64_t event_id = occurrence.event_id;
            connect(row, &QPushButton::clicked, this, [this, event_id] {
                auto existing = events_.find(event_id);
                if (!existing.ok() || !existing.value().has_value()) {
                    reload();
                    return;
                }
                EventDialog dialog{categories_, existing.value(), QDate::currentDate(), this};
                if (dialog.exec() != QDialog::Accepted) {
                    return;
                }
                if (dialog.wants_delete()) {
                    events_.remove(event_id);
                    reload();
                    return;
                }
                Event updated = dialog.result();
                updated.id = event_id;
                updated.created_at = existing.value()->created_at;
                events_.update(updated);
                reload();
            });

            agenda_body_->addWidget(row);
            ++agenda_rows;
        }
    }

    if (agenda_rows == 0) {
        auto* empty = new QLabel{QStringLiteral("Tidak ada event hari ini")};
        empty->setObjectName(QStringLiteral("agendaEmpty"));
        empty->setWordWrap(true);
        agenda_body_->addWidget(empty);
    }
    agenda_body_->addStretch(1);

    auto listed = tasks_.list_all();
    if (!listed.ok()) {
        report(listed.error());
        return;
    }

    std::vector<Task> open_tasks;
    int done_today = 0;
    for (const Task& task : listed.value()) {
        if (is_finished(task.status)) {
            if (task.completed_at.has_value() && local_from_utc(*task.completed_at).date() == today) {
                ++done_today;
            }
            continue;
        }
        if (task.due_at.has_value() && local_from_utc(*task.due_at).date() > today) {
            continue;
        }
        open_tasks.push_back(task);
    }

    std::sort(open_tasks.begin(), open_tasks.end(), [](const Task& left, const Task& right) {
        if (left.due_at.has_value() != right.due_at.has_value()) {
            return left.due_at.has_value();
        }
        if (left.due_at.has_value() && right.due_at.has_value() && *left.due_at != *right.due_at) {
            return *left.due_at < *right.due_at;
        }
        if (left.priority != right.priority) {
            return static_cast<int>(left.priority) > static_cast<int>(right.priority);
        }
        return left.id < right.id;
    });

    for (const Task& task : open_tasks) {
        auto* row = new TaskRow{task};
        connect(row, &TaskRow::toggled, this, [this](std::int64_t id) {
            tasks_.set_status(id, TaskStatus::done);
            QTimer::singleShot(0, this, [this] { reload(); });
        });
        connect(row, &TaskRow::remove_requested, this, [this](std::int64_t id) {
            tasks_.remove(id);
            QTimer::singleShot(0, this, [this] { reload(); });
        });
        connect(row, &TaskRow::edit_requested, this, [this](std::int64_t id) {
            auto existing = tasks_.find(id);
            if (!existing.ok() || !existing.value().has_value()) {
                reload();
                return;
            }
            TaskDialog dialog{existing.value(), this};
            if (dialog.exec() != QDialog::Accepted) {
                return;
            }
            if (dialog.wants_delete()) {
                tasks_.remove(id);
                reload();
                return;
            }
            Task updated = dialog.result();
            updated.id = id;
            updated.created_at = existing.value()->created_at;
            updated.status = existing.value()->status;
            updated.completed_at = existing.value()->completed_at;
            tasks_.update(updated);
            reload();
        });
        task_body_->addWidget(row);
    }

    if (open_tasks.empty()) {
        auto* empty = new QLabel{QStringLiteral("Tidak ada tugas untuk hari ini")};
        empty->setObjectName(QStringLiteral("agendaEmpty"));
        task_body_->addWidget(empty);
    }
    task_body_->addStretch(1);

    stat_open_->setText(QString::number(open_tasks.size()));
    stat_events_->setText(QString::number(event_count));
    stat_done_->setText(QString::number(done_today));
}

void TodayPage::report(const Error& error)
{
    QMessageBox::warning(this, QStringLiteral("DayFlow"), QString::fromStdString(error.message));
}

}  // namespace dayflow
