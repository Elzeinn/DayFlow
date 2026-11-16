#include "ui/main_window.hpp"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>

#include "data/event_repository.hpp"
#include "data/task_repository.hpp"
#include "ui/calendar_page.hpp"
#include "ui/tasks_page.hpp"
#include "ui/theme.hpp"
#include "ui/title_bar.hpp"

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

MainWindow::MainWindow(TaskRepository& tasks, EventRepository& events, EventCategoryRepository& categories,
    QWidget* parent)
    : QMainWindow(parent)
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
    stack_->addWidget(new TasksPage{tasks});
    stack_->addWidget(new CalendarPage{events, categories});

    auto* body = new QHBoxLayout{};
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);
    body->addWidget(build_sidebar());
    body->addWidget(stack_, 1);
    outer_layout_->addLayout(body, 1);

    setCentralWidget(root_);
    root_->installEventFilter(this);

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

    layout->addWidget(make_nav_button(QStringLiteral("Hari Ini"), false, false));

    auto* tasks_button = make_nav_button(QStringLiteral("Tugas"), true, false);
    auto* calendar_button = make_nav_button(QStringLiteral("Kalender"), true, false);
    layout->addWidget(tasks_button);
    layout->addWidget(calendar_button);

    layout->addWidget(make_nav_button(QStringLiteral("Rutinitas"), false, false));
    layout->addWidget(make_nav_button(QStringLiteral("Kebiasaan"), false, false));
    layout->addWidget(make_nav_button(QStringLiteral("Pengaturan"), false, false));

    nav_buttons_.push_back(tasks_button);
    nav_buttons_.push_back(calendar_button);

    connect(tasks_button, &QPushButton::clicked, this, [this] { set_page(0); });
    connect(calendar_button, &QPushButton::clicked, this, [this] { set_page(1); });

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
