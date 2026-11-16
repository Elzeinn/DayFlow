#include "ui/day_cell.hpp"

#include <QLabel>
#include <QMouseEvent>
#include <QStyle>
#include <QVBoxLayout>

namespace dayflow {
namespace {

constexpr int k_max_visible_entries = 3;

}  // namespace

DayCell::DayCell(QWidget* parent) : QFrame(parent)
{
    setObjectName(QStringLiteral("dayCell"));
    setAttribute(Qt::WA_StyledBackground, true);
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(64);

    auto* layout = new QVBoxLayout{this};
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(3);

    number_ = new QLabel{this};
    number_->setObjectName(QStringLiteral("dayNumber"));
    layout->addWidget(number_);

    entries_layout_ = new QVBoxLayout{};
    entries_layout_->setContentsMargins(0, 0, 0, 0);
    entries_layout_->setSpacing(2);
    layout->addLayout(entries_layout_);
    layout->addStretch(1);
}

void DayCell::configure(const QDate& date, bool in_month, bool is_today, bool selected)
{
    date_ = date;
    number_->setText(QString::number(date.day()));

    setProperty("inMonth", in_month);
    setProperty("today", is_today);
    setProperty("selected", selected);

    style()->unpolish(this);
    style()->polish(this);
}

void DayCell::set_entries(const QVector<DayEntry>& entries)
{
    while (QLayoutItem* item = entries_layout_->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    const int visible = qMin(entries.size(), k_max_visible_entries);
    for (int index = 0; index < visible; ++index) {
        const DayEntry& entry = entries.at(index);
        const QString text = entry.time.isEmpty()
            ? entry.title
            : QStringLiteral("%1 %2").arg(entry.time, entry.title);
        auto* label = new QLabel{text, this};
        label->setObjectName(QStringLiteral("dayEntry"));
        label->setStyleSheet(
            QStringLiteral("border-left: 2px solid %1; padding-left: 4px;").arg(entry.color.name()));
        label->setToolTip(entry.title);
        entries_layout_->addWidget(label);
    }

    if (entries.size() > visible) {
        auto* more = new QLabel{QStringLiteral("+%1 lagi").arg(entries.size() - visible), this};
        more->setObjectName(QStringLiteral("dayEntryMore"));
        entries_layout_->addWidget(more);
    }
}

void DayCell::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(date_);
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

}  // namespace dayflow
