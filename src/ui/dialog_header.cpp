#include "ui/dialog_header.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QWindow>

#include "ui/theme.hpp"
#include "ui/title_bar.hpp"

namespace dayflow {

DialogHeader::DialogHeader(const QString& title, QWidget* dialog, QWidget* parent)
    : QWidget(parent), dialog_(dialog)
{
    setObjectName(QStringLiteral("dialogHeader"));
    setFixedHeight(46);

    auto* layout = new QHBoxLayout{this};
    layout->setContentsMargins(20, 0, 10, 0);
    layout->setSpacing(8);

    auto* heading = new QLabel{title, this};
    heading->setObjectName(QStringLiteral("dialogHeading"));
    heading->setFont(theme::dialog_title_font());
    layout->addWidget(heading);
    layout->addStretch(1);

    auto* close = new TitleButton{TitleButton::Kind::close, this};
    layout->addWidget(close);

    connect(close, &TitleButton::clicked, dialog_, [this] { dialog_->close(); });
}

void DialogHeader::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (dialog_ != nullptr) {
            if (QWindow* handle = dialog_->windowHandle(); handle != nullptr) {
                handle->startSystemMove();
                event->accept();
                return;
            }
        }
    }
    QWidget::mousePressEvent(event);
}

}  // namespace dayflow
