#pragma once

#include <QMainWindow>
#include <QPoint>
#include <Qt>

#include <vector>

class QPushButton;
class QStackedWidget;
class QVBoxLayout;

namespace dayflow {

class EventCategoryRepository;
class EventRepository;
class TaskRepository;
class TitleBar;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(TaskRepository& tasks, EventRepository& events, EventCategoryRepository& categories,
        QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    QWidget* build_sidebar();
    void set_page(int index);
    void apply_window_margins();
    Qt::Edges edges_at(const QPoint& position) const;
    void update_resize_cursor(Qt::Edges edges);

    QWidget* root_ = nullptr;
    QVBoxLayout* outer_layout_ = nullptr;
    TitleBar* title_bar_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    std::vector<QPushButton*> nav_buttons_;
};

}  // namespace dayflow
