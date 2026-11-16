#include "ui/select.hpp"

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

namespace dayflow {
namespace {

QIcon dot_icon(const QColor& color)
{
    QPixmap pixmap{10, 10};
    pixmap.fill(Qt::transparent);
    QPainter painter{&pixmap};
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(0, 0, 10, 10);
    return QIcon{pixmap};
}

}  // namespace

Select::Select(QWidget* parent) : QFrame(parent)
{
    setObjectName(QStringLiteral("select"));
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(36);

    auto* layout = new QHBoxLayout{this};
    layout->setContentsMargins(12, 0, 10, 0);
    layout->setSpacing(8);

    value_ = new QLabel{this};
    value_->setObjectName(QStringLiteral("selectValue"));
    layout->addWidget(value_, 1);

    auto* chevron = new QLabel{QStringLiteral("\u25BE"), this};
    chevron->setObjectName(QStringLiteral("selectChevron"));
    layout->addWidget(chevron);
}

void Select::add_item(const QString& text, const QVariant& payload, const QColor& dot)
{
    items_.push_back(Item{text, payload, dot});
    if (current_ < 0) {
        current_ = 0;
    }
    refresh_label();
}

void Select::clear_items()
{
    items_.clear();
    current_ = -1;
    refresh_label();
}

void Select::set_current_index(int index)
{
    if (index < 0 || index >= items_.size()) {
        return;
    }
    current_ = index;
    refresh_label();
}

int Select::index_of_data(const QVariant& payload) const
{
    for (int index = 0; index < items_.size(); ++index) {
        if (items_.at(index).data == payload) {
            return index;
        }
    }
    return -1;
}

QVariant Select::current_data() const
{
    if (current_ < 0 || current_ >= items_.size()) {
        return {};
    }
    return items_.at(current_).data;
}

QString Select::current_text() const
{
    if (current_ < 0 || current_ >= items_.size()) {
        return {};
    }
    return items_.at(current_).text;
}

void Select::refresh_label()
{
    value_->setText(current_text());
}

void Select::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        open_popup();
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void Select::open_popup()
{
    if (items_.isEmpty()) {
        return;
    }

    auto* popup = new QFrame{nullptr, Qt::Popup | Qt::FramelessWindowHint};
    popup->setObjectName(QStringLiteral("selectPopup"));
    popup->setAttribute(Qt::WA_DeleteOnClose);

    auto* layout = new QVBoxLayout{popup};
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(2);

    for (int index = 0; index < items_.size(); ++index) {
        auto* button = new QPushButton{items_.at(index).text, popup};
        button->setObjectName(QStringLiteral("selectItem"));
        button->setProperty("current", index == current_);
        button->setCursor(Qt::PointingHandCursor);
        if (items_.at(index).dot.isValid()) {
            button->setIcon(dot_icon(items_.at(index).dot));
        }
        connect(button, &QPushButton::clicked, this, [this, popup, index] {
            popup->close();
            set_current_index(index);
            emit changed(index);
        });
        layout->addWidget(button);
    }

    popup->setMinimumWidth(qMax(width(), 160));
    popup->adjustSize();
    popup->move(mapToGlobal(QPoint{0, height() + 4}));
    popup->show();
}

}  // namespace dayflow
