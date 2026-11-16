#pragma once

#include <QWidget>

#include <cstdint>
#include <vector>

#include "core/error.hpp"
#include "core/task.hpp"

class QButtonGroup;
class QLabel;
class QLineEdit;
class QListWidget;
class QProgressBar;

namespace dayflow {

class TaskRepository;

class TasksPage : public QWidget {
    Q_OBJECT

public:
    explicit TasksPage(TaskRepository& tasks, QWidget* parent = nullptr);

private:
    QWidget* make_stat_card(const QString& label, QLabel** value_label, bool with_progress);

    void reload();
    void schedule_reload();
    void add_task();
    void edit_task(std::int64_t id);
    void toggle_task(std::int64_t id);
    void remove_task(std::int64_t id);
    void update_stats(const std::vector<Task>& tasks);
    void report(const Error& error);

    TaskRepository& tasks_;
    QLineEdit* search_ = nullptr;
    QButtonGroup* filter_group_ = nullptr;
    QListWidget* task_list_ = nullptr;
    QLabel* empty_state_ = nullptr;
    QLabel* stat_total_ = nullptr;
    QLabel* stat_done_ = nullptr;
    QLabel* stat_progress_ = nullptr;
    QProgressBar* progress_bar_ = nullptr;
    int filter_ = 0;
};

}  // namespace dayflow
