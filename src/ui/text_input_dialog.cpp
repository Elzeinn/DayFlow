#include "ui/text_input_dialog.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "ui/dialog_header.hpp"

namespace dayflow {

TextInputDialog::TextInputDialog(const QString& title, const QString& label, const QString& value, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    setWindowFlag(Qt::FramelessWindowHint, true);
    setMinimumWidth(420);

    auto* layout = new QVBoxLayout{this};
    layout->setContentsMargins(0, 0, 0, 20);
    layout->setSpacing(0);
    layout->addWidget(new DialogHeader{title, this, this});
    layout->addSpacing(10);

    auto* body = new QVBoxLayout{};
    body->setContentsMargins(22, 0, 22, 0);
    body->setSpacing(12);
    layout->addLayout(body);

    auto* caption = new QLabel{label, this};
    body->addWidget(caption);

    field_ = new QLineEdit{value, this};
    field_->setObjectName(QStringLiteral("field"));
    body->addWidget(field_);

    auto* buttons = new QHBoxLayout{};
    buttons->setSpacing(8);
    auto* cancel = new QPushButton{QStringLiteral("Batal"), this};
    cancel->setObjectName(QStringLiteral("secondary"));
    auto* save = new QPushButton{QStringLiteral("Simpan"), this};
    save->setObjectName(QStringLiteral("primary"));
    save->setDefault(true);
    buttons->addStretch(1);
    buttons->addWidget(cancel);
    buttons->addWidget(save);
    body->addSpacing(4);
    body->addLayout(buttons);

    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(save, &QPushButton::clicked, this, &TextInputDialog::accept);
    connect(field_, &QLineEdit::returnPressed, this, &TextInputDialog::accept);

    field_->setFocus();
    field_->selectAll();
}

QString TextInputDialog::text() const
{
    return field_->text().trimmed();
}

void TextInputDialog::accept()
{
    if (text().isEmpty()) {
        field_->setFocus();
        return;
    }
    QDialog::accept();
}

}  // namespace dayflow
