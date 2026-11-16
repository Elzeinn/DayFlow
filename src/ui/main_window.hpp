#pragma once

#include <QMainWindow>

#include <cstdint>
#include <vector>

#include "core/error.hpp"
#include "core/task.hpp"

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QProgressBar;

namespace dayflow {

class TaskRepository;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(TaskRepository& tasks, QWidget* parent = nullptr);

private:
    QWidget* build_sidebar();
    QWidget* build_content();
    QWidget* make_stat_card(const QString& label, QLabel** value_label, bool with_progress);

    void reload();
    void schedule_reload();
    void add_task();
    void toggle_task(std::int64_t id);
    void remove_task(std::int64_t id);
    void update_stats(const std::vector<Task>& tasks);
    void report(const Error& error);

    TaskRepository& tasks_;
    QLineEdit* title_input_ = nullptr;
    QComboBox* priority_input_ = nullptr;
    QListWidget* task_list_ = nullptr;
    QLabel* empty_state_ = nullptr;
    QLabel* stat_total_ = nullptr;
    QLabel* stat_done_ = nullptr;
    QLabel* stat_progress_ = nullptr;
    QProgressBar* progress_bar_ = nullptr;
    int filter_ = 0;
};

}  // namespace dayflow
