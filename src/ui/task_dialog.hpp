#pragma once

#include <QDialog>

#include <optional>

#include "core/task.hpp"

class QLineEdit;

namespace dayflow {

class DateField;
class Select;

class TaskDialog : public QDialog {
    Q_OBJECT

public:
    explicit TaskDialog(const std::optional<Task>& existing = std::nullopt, QWidget* parent = nullptr);

    Task result() const { return task_; }
    bool wants_delete() const { return delete_requested_; }

protected:
    void accept() override;

private:
    Task task_;
    bool delete_requested_ = false;

    QLineEdit* title_ = nullptr;
    Select* priority_ = nullptr;
    DateField* due_ = nullptr;
    QLineEdit* notes_ = nullptr;
};

}  // namespace dayflow
