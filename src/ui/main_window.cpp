#include "ui/main_window.hpp"

#include <QButtonGroup>
#include <QComboBox>
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
#include <QWidget>

#include "data/task_repository.hpp"
#include "ui/task_row.hpp"
#include "ui/theme.hpp"

namespace dayflow {
namespace {

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

QLabel* make_micro_label(const QString& text)
{
    auto* label = new QLabel{text};
    label->setObjectName(QStringLiteral("microLabel"));
    label->setFont(theme::micro_font());
    return label;
}

}  // namespace

MainWindow::MainWindow(TaskRepository& tasks, QWidget* parent) : QMainWindow(parent), tasks_(tasks)
{
    setWindowTitle(QStringLiteral("DayFlow"));
    resize(1180, 760);
    setMinimumSize(900, 600);

    auto* root = new QWidget{this};
    root->setObjectName(QStringLiteral("root"));

    auto* root_layout = new QHBoxLayout{root};
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->setSpacing(0);
    root_layout->addWidget(build_sidebar());
    root_layout->addWidget(build_content(), 1);

    setCentralWidget(root);

    reload();
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

    layout->addWidget(make_micro_label(QStringLiteral("MENU")));
    layout->addSpacing(8);

    layout->addWidget(make_nav_button(QStringLiteral("Hari Ini"), false, false));
    layout->addWidget(make_nav_button(QStringLiteral("Tugas"), true, true));
    layout->addWidget(make_nav_button(QStringLiteral("Kalender"), false, false));
    layout->addWidget(make_nav_button(QStringLiteral("Rutinitas"), false, false));
    layout->addWidget(make_nav_button(QStringLiteral("Kebiasaan"), false, false));
    layout->addWidget(make_nav_button(QStringLiteral("Pengaturan"), false, false));

    layout->addStretch(1);

    auto* note = make_micro_label(QStringLiteral("DATA LOKAL · v0.1"));
    note->setWordWrap(true);
    layout->addWidget(note);

    return sidebar;
}

QWidget* MainWindow::make_stat_card(const QString& label, QLabel** value_label, bool with_progress)
{
    auto* card = new QFrame{};
    card->setObjectName(QStringLiteral("card"));

    auto* layout = new QVBoxLayout{card};
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(6);

    layout->addWidget(make_micro_label(label));

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

QWidget* MainWindow::build_content()
{
    auto* content = new QWidget{};
    auto* layout = new QVBoxLayout{content};
    layout->setContentsMargins(28, 22, 28, 22);
    layout->setSpacing(18);

    auto* header = new QVBoxLayout{};
    header->setSpacing(6);

    const QLocale indonesian{QLocale::Indonesian, QLocale::Indonesia};
    header->addWidget(make_micro_label(
        indonesian.toString(QDate::currentDate(), QStringLiteral("dddd, d MMMM yyyy")).toUpper()));

    auto* title = new QLabel{QStringLiteral("Tugas")};
    title->setObjectName(QStringLiteral("displayTitle"));
    title->setFont(theme::display_font());
    header->addWidget(title);

    layout->addLayout(header);

    auto* stats = new QHBoxLayout{};
    stats->setSpacing(12);
    stats->addWidget(make_stat_card(QStringLiteral("TOTAL TUGAS"), &stat_total_, false), 1);
    stats->addWidget(make_stat_card(QStringLiteral("SELESAI"), &stat_done_, false), 1);
    stats->addWidget(make_stat_card(QStringLiteral("PROGRES"), &stat_progress_, true), 1);
    layout->addLayout(stats);
    layout->addSpacing(4);

    auto* input_row = new QHBoxLayout{};
    input_row->setSpacing(10);

    title_input_ = new QLineEdit{};
    title_input_->setObjectName(QStringLiteral("taskInput"));
    title_input_->setPlaceholderText(QStringLiteral("Apa yang mau dikerjakan? Lalu Enter"));

    priority_input_ = new QComboBox{};
    priority_input_->setObjectName(QStringLiteral("priorityInput"));
    priority_input_->addItem(QStringLiteral("Low"), static_cast<int>(Priority::low));
    priority_input_->addItem(QStringLiteral("Normal"), static_cast<int>(Priority::normal));
    priority_input_->addItem(QStringLiteral("High"), static_cast<int>(Priority::high));
    priority_input_->addItem(QStringLiteral("Urgent"), static_cast<int>(Priority::urgent));
    priority_input_->setCurrentIndex(1);

    auto* add_button = new QPushButton{QStringLiteral("TAMBAH")};
    add_button->setObjectName(QStringLiteral("primary"));
    add_button->setCursor(Qt::PointingHandCursor);

    input_row->addWidget(title_input_, 1);
    input_row->addWidget(priority_input_);
    input_row->addWidget(add_button);
    layout->addLayout(input_row);

    auto* tab_bar = new QFrame{};
    tab_bar->setObjectName(QStringLiteral("tabBar"));
    tab_bar->setFixedHeight(38);
    auto* tab_layout = new QHBoxLayout{tab_bar};
    tab_layout->setContentsMargins(3, 3, 3, 3);
    tab_layout->setSpacing(4);

    auto* group = new QButtonGroup{this};
    group->setExclusive(true);
    const QStringList tab_names{QStringLiteral("SEMUA"), QStringLiteral("AKTIF"), QStringLiteral("SELESAI")};
    for (int index = 0; index < tab_names.size(); ++index) {
        auto* tab = new QPushButton{tab_names.at(index)};
        tab->setObjectName(QStringLiteral("tab"));
        tab->setCheckable(true);
        tab->setChecked(index == 0);
        tab->setCursor(Qt::PointingHandCursor);
        group->addButton(tab, index);
        tab_layout->addWidget(tab);
    }
    tab_layout->addStretch(1);

    auto* tab_wrapper = new QHBoxLayout{};
    tab_wrapper->addWidget(tab_bar);
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
    layout->addWidget(empty_state_, 1);

    connect(add_button, &QPushButton::clicked, this, &MainWindow::add_task);
    connect(title_input_, &QLineEdit::returnPressed, this, &MainWindow::add_task);
    connect(group, &QButtonGroup::idClicked, this, [this](int id) {
        filter_ = id;
        reload();
    });

    return content;
}

void MainWindow::reload()
{
    auto listed = tasks_.list_all();
    if (!listed.ok()) {
        report(listed.error());
        return;
    }

    update_stats(listed.value());

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

        auto* row = new TaskRow{task};
        auto* item = new QListWidgetItem{task_list_};
        item->setSizeHint(QSize{0, 68});
        task_list_->setItemWidget(item, row);

        connect(row, &TaskRow::toggled, this, &MainWindow::toggle_task);
        connect(row, &TaskRow::remove_requested, this, &MainWindow::remove_task);
        ++shown;
    }

    task_list_->setVisible(shown > 0);
    empty_state_->setVisible(shown == 0);
}

void MainWindow::schedule_reload()
{
    QTimer::singleShot(0, this, [this] { reload(); });
}

void MainWindow::add_task()
{
    const QString title = title_input_->text().trimmed();
    if (title.isEmpty()) {
        return;
    }

    Task task;
    task.title = title.toStdString();
    task.priority = static_cast<Priority>(priority_input_->currentData().toInt());

    auto created = tasks_.create(task);
    if (!created.ok()) {
        report(created.error());
        return;
    }

    title_input_->clear();
    reload();
}

void MainWindow::toggle_task(std::int64_t id)
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

void MainWindow::remove_task(std::int64_t id)
{
    if (Error error = tasks_.remove(id)) {
        report(error);
        return;
    }
    schedule_reload();
}

void MainWindow::update_stats(const std::vector<Task>& tasks)
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

void MainWindow::report(const Error& error)
{
    QMessageBox::warning(this, QStringLiteral("DayFlow"), QString::fromStdString(error.message));
}

}  // namespace dayflow
