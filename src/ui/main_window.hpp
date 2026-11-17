#pragma once

#include <QMainWindow>
#include <QPoint>
#include <Qt>

#include <vector>

#include "ui/app_context.hpp"

class QPushButton;
class QStackedWidget;
class QVBoxLayout;

namespace dayflow {

class NotesPage;
class TitleBar;
struct SearchChoice;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const AppContext& context, QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    QWidget* build_sidebar();
    void set_page(int index);
    void open_search();
    void handle_search_result(const SearchChoice& choice);
    void apply_window_margins();
    Qt::Edges edges_at(const QPoint& position) const;
    void update_resize_cursor(Qt::Edges edges);

    QWidget* root_ = nullptr;
    QVBoxLayout* outer_layout_ = nullptr;
    TitleBar* title_bar_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    NotesPage* notes_page_ = nullptr;
    std::vector<QPushButton*> nav_buttons_;
    AppContext context_;
};

}  // namespace dayflow
