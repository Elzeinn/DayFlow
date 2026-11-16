#pragma once

#include <QMainWindow>
#include <QPoint>
#include <Qt>

#include <cstdint>
#include <vector>

#include "core/error.hpp"
#include "core/task.hpp"

class QComboBox;
class QButtonGroup;
class QLabel;
class QLineEdit;
class QListWidget;
class QProgressBar;
class QVBoxLayout;

namespace dayflow {

class TaskRepository;
class TitleBar;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(TaskRepository& tasks, QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;

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
    void apply_window_margins();
    Qt::Edges edges_at(const QPoint& position) const;
    void update_resize_cursor(Qt::Edges edges);

    TaskRepository& tasks_;
    QWidget* root_ = nullptr;
    QVBoxLayout* outer_layout_ = nullptr;
    TitleBar* title_bar_ = nullptr;
    QLineEdit* title_input_ = nullptr;
    QComboBox* priority_input_ = nullptr;
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
