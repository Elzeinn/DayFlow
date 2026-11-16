#include "ui/task_row.hpp"

#include <QCheckBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

#include "ui/theme.hpp"

namespace dayflow {
namespace {

QString format_due(const std::optional<Timestamp>& due)
{
    if (!due.has_value()) {
        return {};
    }
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(due->time_since_epoch()).count();
    return QDateTime::fromSecsSinceEpoch(seconds).toLocalTime().toString("dd/MM HH:mm");
}

QLabel* make_chip(const QString& text, bool urgent)
{
    auto* chip = new QLabel{text};
    chip->setObjectName(QStringLiteral("chip"));
    chip->setProperty("urgent", urgent);
    return chip;
}

}  // namespace

TaskRow::TaskRow(const Task& task, QWidget* parent) : QWidget(parent), id_(task.id)
{
    setObjectName(QStringLiteral("taskRow"));
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_Hover);
    setFixedHeight(62);

    auto* layout = new QHBoxLayout{this};
    layout->setContentsMargins(14, 10, 10, 10);
    layout->setSpacing(12);

    const bool finished = is_finished(task.status);

    auto* done_box = new QCheckBox{this};
    done_box->setObjectName(QStringLiteral("doneBox"));
    done_box->setChecked(finished);
    done_box->setToolTip(finished ? QStringLiteral("Tandai belum selesai") : QStringLiteral("Tandai selesai"));
    layout->addWidget(done_box);

    auto* text_column = new QVBoxLayout{};
    text_column->setSpacing(5);

    auto* title = new QLabel{QString::fromStdString(task.title), this};
    title->setObjectName(QStringLiteral("taskTitle"));
    title->setProperty("done", finished);
    if (finished) {
        QFont font = title->font();
        font.setStrikeOut(true);
        title->setFont(font);
    }
    text_column->addWidget(title);

    auto* meta_row = new QHBoxLayout{};
    meta_row->setSpacing(6);
    meta_row->addWidget(make_chip(QString::fromStdString(to_string(task.priority)).toUpper(),
        task.priority == Priority::urgent));

    const QString due = format_due(task.due_at);
    if (!due.isEmpty()) {
        meta_row->addWidget(make_chip(due, false));
    }
    meta_row->addStretch(1);
    text_column->addLayout(meta_row);

    layout->addLayout(text_column, 1);

    auto* remove_button = new QPushButton{QStringLiteral("\u2715"), this};
    remove_button->setObjectName(QStringLiteral("rowAction"));
    remove_button->setFixedSize(26, 26);
    remove_button->setToolTip(QStringLiteral("Hapus tugas"));
    layout->addWidget(remove_button);

    connect(done_box, &QCheckBox::toggled, this, [this](bool) { emit toggled(id_); });
    connect(remove_button, &QPushButton::clicked, this, [this] { emit remove_requested(id_); });
}

void TaskRow::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit edit_requested(id_);
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

}  // namespace dayflow
