#include "ui/main_window.hpp"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>

#include "data/event_repository.hpp"
#include "data/note_repository.hpp"
#include "data/routine_repository.hpp"
#include "data/task_repository.hpp"
#include "ui/calendar_page.hpp"
#include "ui/event_dialog.hpp"
#include "ui/habits_page.hpp"
#include "ui/notes_page.hpp"
#include "ui/routines_page.hpp"
#include "ui/search_dialog.hpp"
#include "ui/settings_page.hpp"
#include "ui/task_dialog.hpp"
#include "ui/tasks_page.hpp"
#include "ui/theme.hpp"
#include "ui/title_bar.hpp"
#include "ui/today_page.hpp"

namespace dayflow {
namespace {

constexpr int k_resize_margin = 6;

QPushButton* make_nav_button(const QString& text, bool enabled, bool active)
{
    auto* button = new QPushButton{text};
    button->setObjectName(QStringLiteral("nav"));
    button->setProperty("active", active);
    button->setEnabled(enabled);
    button->setCursor(enabled ? Qt::PointingHandCursor : Qt::ArrowCursor);
    if (!enabled) {
        button->setToolTip(QStringLiteral("Belum tersedia — dikerjakan di milestone berikutnya"));
    }
    return button;
}

}  // namespace

MainWindow::MainWindow(const AppContext& context, QWidget* parent) : QMainWindow(parent), context_(context)
{
    setWindowFlag(Qt::FramelessWindowHint, true);
    setWindowTitle(QStringLiteral("DayFlow"));
    resize(1180, 760);
    setMinimumSize(900, 600);

    root_ = new QWidget{this};
    root_->setObjectName(QStringLiteral("root"));
    root_->setMouseTracking(true);

    outer_layout_ = new QVBoxLayout{root_};
    outer_layout_->setSpacing(0);
    apply_window_margins();

    title_bar_ = new TitleBar{this, root_};
    outer_layout_->addWidget(title_bar_);

    stack_ = new QStackedWidget{};
    stack_->addWidget(new TodayPage{context_.tasks, context_.events, context_.categories});
    stack_->addWidget(new TasksPage{context_.tasks});
    notes_page_ = new NotesPage{context_.notes};
    stack_->addWidget(notes_page_);
    stack_->addWidget(new CalendarPage{context_.events, context_.categories});
    stack_->addWidget(new RoutinesPage{context_.routines});
    stack_->addWidget(new HabitsPage{context_.habits});
    stack_->addWidget(new SettingsPage{context_.database, context_.database_path});

    auto* body = new QHBoxLayout{};
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);
    body->addWidget(build_sidebar());
    body->addWidget(stack_, 1);
    outer_layout_->addLayout(body, 1);

    setCentralWidget(root_);
    root_->installEventFilter(this);

    auto* search_shortcut = new QShortcut{QKeySequence{QStringLiteral("Ctrl+K")}, this};
    connect(search_shortcut, &QShortcut::activated, this, &MainWindow::open_search);

    for (std::size_t index = 0; index < nav_buttons_.size(); ++index) {
        auto* shortcut = new QShortcut{
            QKeySequence{QStringLiteral("Ctrl+%1").arg(static_cast<int>(index) + 1)}, this};
        const int page = static_cast<int>(index);
        connect(shortcut, &QShortcut::activated, this, [this, page] { set_page(page); });
    }

    set_page(0);
}

QWidget* MainWindow::build_sidebar()
{
    auto* sidebar = new QWidget{};
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(208);

    auto* layout = new QVBoxLayout{sidebar};
    layout->setContentsMargins(16, 20, 16, 18);
    layout->setSpacing(4);

    auto* wordmark = new QLabel{QStringLiteral("DAYFLOW")};
    wordmark->setObjectName(QStringLiteral("wordmark"));
    layout->addWidget(wordmark);
    layout->addSpacing(22);

    layout->addWidget(theme::make_micro_label(QStringLiteral("MENU")));
    layout->addSpacing(8);

    auto* today_button = make_nav_button(QStringLiteral("Hari Ini"), true, false);
    auto* tasks_button = make_nav_button(QStringLiteral("Tugas"), true, false);
    auto* notes_button = make_nav_button(QStringLiteral("Catatan"), true, false);
    auto* calendar_button = make_nav_button(QStringLiteral("Kalender"), true, false);
    auto* routines_button = make_nav_button(QStringLiteral("Rutinitas"), true, false);
    auto* habits_button = make_nav_button(QStringLiteral("Kebiasaan"), true, false);
    auto* settings_button = make_nav_button(QStringLiteral("Pengaturan"), true, false);

    layout->addWidget(today_button);
    layout->addWidget(tasks_button);
    layout->addWidget(notes_button);
    layout->addWidget(calendar_button);
    layout->addWidget(routines_button);
    layout->addWidget(habits_button);
    layout->addWidget(settings_button);

    nav_buttons_.push_back(today_button);
    nav_buttons_.push_back(tasks_button);
    nav_buttons_.push_back(notes_button);
    nav_buttons_.push_back(calendar_button);
    nav_buttons_.push_back(routines_button);
    nav_buttons_.push_back(habits_button);
    nav_buttons_.push_back(settings_button);

    for (std::size_t index = 0; index < nav_buttons_.size(); ++index) {
        QPushButton* button = nav_buttons_.at(index);
        const int page = static_cast<int>(index);
        connect(button, &QPushButton::clicked, this, [this, page] { set_page(page); });
    }

    layout->addStretch(1);

    auto* note = theme::make_micro_label(QStringLiteral("DATA LOKAL · v0.1"));
    note->setWordWrap(true);
    layout->addWidget(note);

    return sidebar;
}

void MainWindow::set_page(int index)
{
    if (stack_ == nullptr) {
        return;
    }
    stack_->setCurrentIndex(index);

    for (std::size_t position = 0; position < nav_buttons_.size(); ++position) {
        QPushButton* button = nav_buttons_.at(position);
        const bool active = static_cast<int>(position) == index;
        if (button->property("active").toBool() != active) {
            button->setProperty("active", active);
            button->style()->unpolish(button);
            button->style()->polish(button);
        }
    }
}

void MainWindow::open_search()
{
    SearchDialog dialog{context_, this};
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    handle_search_result(dialog.choice());
}

void MainWindow::handle_search_result(const SearchChoice& choice)
{
    switch (choice.kind) {
    case SearchKind::task: {
        auto found = context_.tasks.find(choice.id);
        if (!found.ok() || !found.value().has_value()) {
            return;
        }
        TaskDialog dialog{found.value(), this};
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        if (dialog.wants_delete()) {
            context_.tasks.remove(choice.id);
        } else {
            Task updated = dialog.result();
            updated.id = choice.id;
            updated.created_at = found.value()->created_at;
            updated.status = found.value()->status;
            updated.completed_at = found.value()->completed_at;
            context_.tasks.update(updated);
        }
        set_page(1);
        return;
    }
    case SearchKind::note:
        set_page(2);
        if (notes_page_ != nullptr) {
            notes_page_->open_note_id(choice.id);
        }
        return;
    case SearchKind::event: {
        auto found = context_.events.find(choice.id);
        if (!found.ok() || !found.value().has_value()) {
            return;
        }
        EventDialog dialog{context_.categories, found.value(), QDate::currentDate(), this};
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        if (dialog.wants_delete()) {
            context_.events.remove(choice.id);
        } else {
            Event updated = dialog.result();
            updated.id = choice.id;
            updated.created_at = found.value()->created_at;
            context_.events.update(updated);
        }
        set_page(3);
        return;
    }
    }
}

void MainWindow::apply_window_margins()
{
    const bool maximized = isMaximized() || isFullScreen();
    const int margin = maximized ? 0 : k_resize_margin;
    outer_layout_->setContentsMargins(margin, margin, margin, margin);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == root_ && !isMaximized() && !isFullScreen()) {
        if (event->type() == QEvent::MouseMove) {
            auto* mouse_event = static_cast<QMouseEvent*>(event);
            update_resize_cursor(edges_at(mouse_event->position().toPoint()));
        } else if (event->type() == QEvent::MouseButtonPress) {
            auto* mouse_event = static_cast<QMouseEvent*>(event);
            if (mouse_event->button() == Qt::LeftButton) {
                const Qt::Edges edges = edges_at(mouse_event->position().toPoint());
                if (edges != Qt::Edges{}) {
                    if (QWindow* handle = windowHandle(); handle != nullptr) {
                        handle->startSystemResize(edges);
                        return true;
                    }
                }
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange) {
        apply_window_margins();
        if (title_bar_ != nullptr) {
            title_bar_->set_maximized(isMaximized());
        }
    }
    QMainWindow::changeEvent(event);
}

Qt::Edges MainWindow::edges_at(const QPoint& position) const
{
    Qt::Edges edges;
    const QRect area = root_->rect();
    if (position.x() <= k_resize_margin) {
        edges |= Qt::LeftEdge;
    }
    if (position.x() >= area.width() - k_resize_margin) {
        edges |= Qt::RightEdge;
    }
    if (position.y() <= k_resize_margin) {
        edges |= Qt::TopEdge;
    }
    if (position.y() >= area.height() - k_resize_margin) {
        edges |= Qt::BottomEdge;
    }
    return edges;
}

void MainWindow::update_resize_cursor(Qt::Edges edges)
{
    Qt::CursorShape shape = Qt::ArrowCursor;

    const bool left = edges.testFlag(Qt::LeftEdge);
    const bool right = edges.testFlag(Qt::RightEdge);
    const bool top = edges.testFlag(Qt::TopEdge);
    const bool bottom = edges.testFlag(Qt::BottomEdge);

    if ((left && top) || (right && bottom)) {
        shape = Qt::SizeFDiagCursor;
    } else if ((right && top) || (left && bottom)) {
        shape = Qt::SizeBDiagCursor;
    } else if (left || right) {
        shape = Qt::SizeHorCursor;
    } else if (top || bottom) {
        shape = Qt::SizeVerCursor;
    }

    root_->setCursor(shape);
}

}  // namespace dayflow
