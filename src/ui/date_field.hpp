#pragma once

#include <QDateTime>
#include <QFrame>

class QLabel;

namespace dayflow {

class DateField : public QFrame {
    Q_OBJECT

public:
    explicit DateField(QWidget* parent = nullptr);

    void set_allow_empty(bool allow);
    bool is_empty() const { return empty_; }
    QDateTime value() const { return value_; }
    void set_value(const QDateTime& value);
    void set_empty();

signals:
    void changed();

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    void refresh_label();
    void open_popup();

    QString empty_text_ = QStringLiteral("Tanpa tanggal");
    bool allow_empty_ = true;
    bool empty_ = true;
    QDateTime value_;
    QLabel* value_label_ = nullptr;
};

}  // namespace dayflow
