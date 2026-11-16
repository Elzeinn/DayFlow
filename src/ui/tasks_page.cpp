#include "ui/tasks_page.hpp"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDate>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include "data/task_repository.hpp"
#include "ui/task_dialog.hpp"
#include "ui/task_row.hpp"
#include "ui/theme.hpp"

namespace dayflow {

TasksPage::TasksPage(TaskRepository& tasks, QWidget* parent) : QWidget(parent), tasks_(tasks)
{
    auto* layout = new QVBoxLayout{this};
    layout->setContentsMargins(28, 22, 28, 22);
    layout->setSpacing(18);

    auto* header = new QVBoxLayout{};
    header->setSpacing(6);

    const QLocale indonesian{QLocale::Indonesian, QLocale::Indonesia};
    header->addWidget(theme::make_micro_label(
        indonesian.toString(QDate::currentDate(), QStringLiteral("dddd, d MMMM yyyy")).toUpper()));

    auto* title = new QLabel{QStringLiteral("Tugas")};
    title->setObjectName(QStringLiteral("displayTitle"));
    title->setFont(theme::display_font());
    title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    title->setFixedHeight(40);
    header->addWidget(title);

    layout->addLayout(header);

    auto* stats = new QHBoxLayout{};
    stats->setSpacing(12);
    stats->addWidget(make_stat_card(QStringLiteral("TOTAL TUGAS"), &stat_total_, false), 1);
    stats->addWidget(make_stat_card(QStringLiteral("SELESAI"), &stat_done_, false), 1);
    stats->addWidget(make_stat_card(QStringLiteral("PROGRES"), &stat_progress_, true), 1);
    layout->addLayout(stats);
    layout->addSpacing(4);

    auto* search_row = new QHBoxLayout{};
    search_row->setSpacing(10);

    search_ = new QLineEdit{};
    search_->setObjectName(QStringLiteral("taskInput"));
    search_->setPlaceholderText(QStringLiteral("Cari tugas..."));
    search_->setClearButtonEnabled(true);

    auto* add_button = new QPushButton{QStringLiteral("+ TUGAS")};
    add_button->setObjectName(QStringLiteral("primary"));
    add_button->setCursor(Qt::PointingHandCursor);

    search_row->addWidget(search_, 1);
    search_row->addWidget(add_button);
    layout->addLayout(search_row);

    auto* tab_bar = new QFrame{};
    tab_bar->setObjectName(QStringLiteral("tabBar"));
    tab_bar->setFixedHeight(38);
    tab_bar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    auto* tab_layout = new QHBoxLayout{tab_bar};
    tab_layout->setContentsMargins(3, 3, 3, 3);
    tab_layout->setSpacing(4);

    auto* group = new QButtonGroup{this};
    group->setExclusive(true);
    filter_group_ = group;
    const QStringList tab_names{QStringLiteral("SEMUA"), QStringLiteral("AKTIF"), QStringLiteral("SELESAI")};
    for (int index = 0; index < tab_names.size(); ++index) {
        auto* tab = new QPushButton{tab_names.at(index)};
        tab->setObjectName(QStringLiteral("tab"));
        tab->setCheckable(true);
        tab->setCursor(Qt::PointingHandCursor);
        group->addButton(tab, index);
        tab_layout->addWidget(tab);
    }
    tab_layout->addStretch(1);

    auto* tab_wrapper = new QHBoxLayout{};
    tab_wrapper->addWidget(tab_bar);
    tab_wrapper->addStretch(1);
    layout->addLayout(tab_wrapper);

    task_list_ = new QListWidget{};
    task_list_->setObjectName(QStringLiteral("taskList"));
    task_list_->setSpacing(6);
    task_list_->setSelectionMode(QAbstractItemView::NoSelection);
    task_list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    layout->addWidget(task_list_, 1);

    empty_state_ = new QLabel{QStringLiteral("BELUM ADA TUGAS")};
    empty_state_->setObjectName(QStringLiteral("emptyState"));
    empty_state_->setAlignment(Qt::AlignCenter);
    empty_state_->setFont(theme::micro_font());
    empty_state_->setWordWrap(true);
    layout->addWidget(empty_state_, 1);

    connect(add_button, &QPushButton::clicked, this, &TasksPage::add_task);
    connect(search_, &QLineEdit::textChanged, this, [this](const QString&) { reload(); });
    connect(group, &QButtonGroup::idClicked, this, [this](int id) {
        filter_ = id;
        reload();
    });

    reload();
}

QWidget* TasksPage::make_stat_card(const QString& label, QLabel** value_label, bool with_progress)
{
    auto* card = new QFrame{};
    card->setObjectName(QStringLiteral("card"));

    auto* layout = new QVBoxLayout{card};
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(6);

    layout->addWidget(theme::make_micro_label(label));

    auto* value = new QLabel{QStringLiteral("0")};
    value->setObjectName(with_progress ? QStringLiteral("statValueAccent") : QStringLiteral("statValue"));
    layout->addWidget(value);
    *value_label = value;

    if (with_progress) {
        progress_bar_ = new QProgressBar{};
        progress_bar_->setObjectName(QStringLiteral("progress"));
        progress_bar_->setRange(0, 100);
        progress_bar_->setValue(0);
        progress_bar_->setTextVisible(false);
        progress_bar_->setFixedHeight(5);
        layout->addWidget(progress_bar_);
    } else {
        layout->addStretch(1);
    }

    return card;
}

void TasksPage::reload()
{
    auto listed = tasks_.list_all();
    if (!listed.ok()) {
        report(listed.error());
        return;
    }

    update_stats(listed.value());

    if (filter_group_ != nullptr) {
        for (QAbstractButton* button : filter_group_->buttons()) {
            button->setChecked(filter_group_->id(button) == filter_);
        }
    }

    const QString query = search_ != nullptr ? search_->text().trimmed() : QString{};

    task_list_->clear();

    int shown = 0;
    for (const Task& task : listed.value()) {
        const bool finished = is_finished(task.status);
        if (filter_ == 1 && finished) {
            continue;
        }
        if (filter_ == 2 && !finished) {
            continue;
        }
        if (!query.isEmpty() && !QString::fromStdString(task.title).contains(query, Qt::CaseInsensitive)) {
            continue;
        }

        auto* row = new TaskRow{task};
        auto* item = new QListWidgetItem{task_list_};
        item->setSizeHint(QSize{0, 68});
        task_list_->setItemWidget(item, row);

        connect(row, &TaskRow::toggled, this, &TasksPage::toggle_task);
        connect(row, &TaskRow::remove_requested, this, &TasksPage::remove_task);
        connect(row, &TaskRow::edit_requested, this, &TasksPage::edit_task);
        ++shown;
    }

    task_list_->setVisible(shown > 0);
    empty_state_->setVisible(shown == 0);
    empty_state_->setText(query.isEmpty() ? QStringLiteral("BELUM ADA TUGAS")
                                          : QStringLiteral("TIDAK ADA TUGAS YANG COCOK"));
}

void TasksPage::schedule_reload()
{
    QTimer::singleShot(0, this, [this] { reload(); });
}

void TasksPage::add_task()
{
    TaskDialog dialog{std::nullopt, this};
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    auto created = tasks_.create(dialog.result());
    if (!created.ok()) {
        report(created.error());
        return;
    }
    reload();
}

void TasksPage::edit_task(std::int64_t id)
{
    auto existing = tasks_.find(id);
    if (!existing.ok()) {
        report(existing.error());
        return;
    }
    if (!existing.value().has_value()) {
        reload();
        return;
    }

    TaskDialog dialog{existing.value(), this};
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    if (dialog.wants_delete()) {
        if (Error error = tasks_.remove(id)) {
            report(error);
            return;
        }
        reload();
        return;
    }

    Task updated = dialog.result();
    updated.id = id;
    updated.created_at = existing.value()->created_at;
    updated.status = existing.value()->status;
    updated.completed_at = existing.value()->completed_at;
    if (Error error = tasks_.update(updated)) {
        report(error);
        return;
    }
    reload();
}

void TasksPage::toggle_task(std::int64_t id)
{
    auto found = tasks_.find(id);
    if (!found.ok()) {
        report(found.error());
        return;
    }
    if (!found.value().has_value()) {
        schedule_reload();
        return;
    }

    const TaskStatus next = is_finished(found.value()->status) ? TaskStatus::todo : TaskStatus::done;
    if (Error error = tasks_.set_status(id, next)) {
        report(error);
        return;
    }
    schedule_reload();
}

void TasksPage::remove_task(std::int64_t id)
{
    if (Error error = tasks_.remove(id)) {
        report(error);
        return;
    }
    schedule_reload();
}

void TasksPage::update_stats(const std::vector<Task>& tasks)
{
    int finished = 0;
    for (const Task& task : tasks) {
        if (is_finished(task.status)) {
            ++finished;
        }
    }

    const int total = static_cast<int>(tasks.size());
    const int percent = total == 0 ? 0 : (finished * 100) / total;

    stat_total_->setText(QString::number(total));
    stat_done_->setText(QString::number(finished));
    stat_progress_->setText(QString::number(percent) + QStringLiteral("%"));
    if (progress_bar_ != nullptr) {
        progress_bar_->setValue(percent);
    }
}

void TasksPage::report(const Error& error)
{
    QMessageBox::warning(this, QStringLiteral("DayFlow"), QString::fromStdString(error.message));
}

}  // namespace dayflow
