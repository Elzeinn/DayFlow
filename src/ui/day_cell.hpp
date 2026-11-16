#pragma once

#include <QColor>
#include <QDate>
#include <QFrame>
#include <QString>
#include <QVector>

class QLabel;
class QVBoxLayout;

namespace dayflow {

struct DayEntry {
    QString time;
    QString title;
    QColor color;
};

class DayCell : public QFrame {
    Q_OBJECT

public:
    explicit DayCell(QWidget* parent = nullptr);

    void configure(const QDate& date, bool in_month, bool is_today, bool selected);
    void set_entries(const QVector<DayEntry>& entries);

signals:
    void clicked(const QDate& date);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    QDate date_;
    QLabel* number_ = nullptr;
    QVBoxLayout* entries_layout_ = nullptr;
};

}  // namespace dayflow
