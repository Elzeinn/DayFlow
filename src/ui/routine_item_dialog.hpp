#pragma once

#include <QDialog>

#include <array>
#include <cstdint>
#include <optional>

#include "core/routine.hpp"

class QLineEdit;
class QPushButton;
class QTimeEdit;

namespace dayflow {

class Select;

class RoutineItemDialog : public QDialog {
    Q_OBJECT

public:
    RoutineItemDialog(std::int64_t routine_id, std::optional<RoutineItem> existing, QWidget* parent = nullptr);

    RoutineItem result() const { return item_; }
    bool wants_delete() const { return delete_requested_; }

protected:
    void accept() override;

private:
    RoutineItem item_;
    bool delete_requested_ = false;

    QLineEdit* title_ = nullptr;
    std::array<QPushButton*, 7> day_buttons_{};
    QTimeEdit* start_ = nullptr;
    Select* duration_ = nullptr;
};

}  // namespace dayflow
