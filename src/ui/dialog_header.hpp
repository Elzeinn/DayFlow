#pragma once

#include <QWidget>

namespace dayflow {

class DialogHeader : public QWidget {
    Q_OBJECT

public:
    DialogHeader(const QString& title, QWidget* dialog, QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    QWidget* dialog_ = nullptr;
};

}  // namespace dayflow
