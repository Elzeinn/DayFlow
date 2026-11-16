#pragma once

#include <QWidget>

class QPushButton;

namespace dayflow {

class TitleButton : public QWidget {
    Q_OBJECT

public:
    enum class Kind {
        minimize,
        maximize,
        restore,
        close,
    };

    explicit TitleButton(Kind kind, QWidget* parent = nullptr);

    void set_kind(Kind kind);

signals:
    void clicked();

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    Kind kind_;
    bool hovered_ = false;
    bool pressed_ = false;
};

class TitleBar : public QWidget {
    Q_OBJECT

public:
    explicit TitleBar(QWidget* window, QWidget* parent = nullptr);

    void set_maximized(bool maximized);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    QWidget* window_;
    TitleButton* maximize_button_ = nullptr;
};

}  // namespace dayflow
