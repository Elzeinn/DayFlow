#pragma once

#include <QWidget>

#include <cstdint>

#include "core/task.hpp"

class QMouseEvent;

namespace dayflow {

class TaskRow : public QWidget {
    Q_OBJECT

public:
    explicit TaskRow(const Task& task, QWidget* parent = nullptr);

    std::int64_t id() const { return id_; }

signals:
    void toggled(std::int64_t id);
    void remove_requested(std::int64_t id);
    void edit_requested(std::int64_t id);

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    std::int64_t id_ = 0;
};

}  // namespace dayflow
