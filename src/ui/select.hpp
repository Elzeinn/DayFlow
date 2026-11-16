#pragma once

#include <QColor>
#include <QFrame>
#include <QVariant>
#include <QVector>

class QLabel;

namespace dayflow {

class Select : public QFrame {
    Q_OBJECT

public:
    explicit Select(QWidget* parent = nullptr);

    void add_item(const QString& text, const QVariant& payload = {}, const QColor& dot = QColor{});
    void clear_items();
    void set_current_index(int index);
    int current_index() const { return current_; }
    int index_of_data(const QVariant& payload) const;
    QVariant current_data() const;
    QString current_text() const;

signals:
    void changed(int index);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    struct Item {
        QString text;
        QVariant data;
        QColor dot;
    };

    void refresh_label();
    void open_popup();

    QVector<Item> items_;
    int current_ = -1;
    QLabel* value_ = nullptr;
};

}  // namespace dayflow
