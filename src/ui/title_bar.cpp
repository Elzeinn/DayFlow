#include "ui/title_bar.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWindow>

#include "ui/theme.hpp"

namespace dayflow {
namespace {

constexpr int k_button_width = 42;
constexpr int k_button_height = 28;

}  // namespace

TitleButton::TitleButton(Kind kind, QWidget* parent) : QWidget(parent), kind_(kind)
{
    setFixedSize(k_button_width, k_button_height);
    setCursor(Qt::ArrowCursor);
    setToolTip(kind == Kind::close ? QStringLiteral("Tutup")
                                   : (kind == Kind::minimize ? QStringLiteral("Minimize") : QStringLiteral("Maximize")));
}

void TitleButton::set_kind(Kind kind)
{
    kind_ = kind;
    setToolTip(kind == Kind::restore ? QStringLiteral("Restore") : QStringLiteral("Maximize"));
    update();
}

void TitleButton::enterEvent(QEnterEvent*)
{
    hovered_ = true;
    update();
}

void TitleButton::leaveEvent(QEvent*)
{
    hovered_ = false;
    pressed_ = false;
    update();
}

void TitleButton::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        pressed_ = true;
        update();
    }
}

void TitleButton::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && pressed_) {
        pressed_ = false;
        update();
        if (rect().contains(event->position().toPoint())) {
            emit clicked();
        }
    }
}

void TitleButton::paintEvent(QPaintEvent*)
{
    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing);

    const bool danger = kind_ == Kind::close;
    if (hovered_) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(danger ? QColor{232, 17, 35} : QColor{255, 255, 255, 26});
        painter.drawRoundedRect(rect(), 6, 6);
    }

    QColor glyph = QColor{227, 227, 227};
    if (danger && hovered_) {
        glyph = QColor{255, 255, 255};
    }
    if (pressed_) {
        glyph = glyph.darker(115);
    }

    QPen pen{glyph};
    pen.setWidthF(1.1);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    const QPointF center = QRectF{rect()}.center();
    const qreal half = 4.0;

    switch (kind_) {
    case Kind::minimize:
        painter.drawLine(QPointF{center.x() - half, center.y() + 3},
            QPointF{center.x() + half, center.y() + 3});
        break;
    case Kind::maximize:
        painter.drawRect(QRectF{center.x() - half, center.y() - half, half * 2, half * 2});
        break;
    case Kind::restore:
        painter.drawRect(QRectF{center.x() - half, center.y() - half + 2, half * 2 - 2, half * 2 - 2});
        painter.drawRect(QRectF{center.x() - half + 2, center.y() - half, half * 2 - 2, half * 2 - 2});
        break;
    case Kind::close:
        painter.drawLine(QPointF{center.x() - half, center.y() - half},
            QPointF{center.x() + half, center.y() + half});
        painter.drawLine(QPointF{center.x() + half, center.y() - half},
            QPointF{center.x() - half, center.y() + half});
        break;
    }
}

TitleBar::TitleBar(QWidget* window, QWidget* parent) : QWidget(parent), window_(window)
{
    setObjectName(QStringLiteral("titleBar"));
    setFixedHeight(42);

    auto* layout = new QHBoxLayout{this};
    layout->setContentsMargins(12, 0, 8, 0);
    layout->setSpacing(9);

    auto* mark = new QLabel{};
    mark->setObjectName(QStringLiteral("logoMark"));
    mark->setFixedSize(15, 15);
    layout->addWidget(mark);

    auto* title = new QLabel{QStringLiteral("DayFlow")};
    title->setObjectName(QStringLiteral("appTitle"));
    layout->addWidget(title);
    layout->addStretch(1);

    auto* minimize = new TitleButton{TitleButton::Kind::minimize};
    maximize_button_ = new TitleButton{TitleButton::Kind::maximize};
    auto* close = new TitleButton{TitleButton::Kind::close};

    layout->addWidget(minimize);
    layout->addWidget(maximize_button_);
    layout->addSpacing(2);
    layout->addWidget(close);

    connect(minimize, &TitleButton::clicked, this, [this] { window_->showMinimized(); });
    connect(maximize_button_, &TitleButton::clicked, this, [this] {
        if (window_->isMaximized()) {
            window_->showNormal();
        } else {
            window_->showMaximized();
        }
    });
    connect(close, &TitleButton::clicked, this, [this] { window_->close(); });
}

void TitleBar::set_maximized(bool maximized)
{
    maximize_button_->set_kind(maximized ? TitleButton::Kind::restore : TitleButton::Kind::maximize);
}

void TitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (QWindow* handle = window_->windowHandle(); handle != nullptr) {
            handle->startSystemMove();
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (window_->isMaximized()) {
            window_->showNormal();
        } else {
            window_->showMaximized();
        }
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

}  // namespace dayflow
