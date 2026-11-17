#pragma once

#include <QDialog>

class QLineEdit;

namespace dayflow {

class TextInputDialog : public QDialog {
    Q_OBJECT

public:
    TextInputDialog(const QString& title, const QString& label, const QString& value = QString{},
        QWidget* parent = nullptr);

    QString text() const;

protected:
    void accept() override;

private:
    QLineEdit* field_ = nullptr;
};

}  // namespace dayflow
