#pragma once

#include <QWidget>

#include <cstdint>
#include <vector>

#include "core/error.hpp"
#include "core/routine.hpp"

class QVBoxLayout;

namespace dayflow {

class RoutineRepository;
class Select;

class RoutinesPage : public QWidget {
    Q_OBJECT

public:
    explicit RoutinesPage(RoutineRepository& routines, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;

private:
    QWidget* build_toolbar();
    QWidget* make_card(const QString& title, QVBoxLayout** body);

    void reload();
    void rebuild_schedule();
    void rebuild_today();
    void add_routine();
    void remove_routine();
    void add_item();
    void edit_item(std::int64_t id);
    void mark(std::int64_t item_id, RoutineLogStatus status, bool toggle);
    void report(const Error& error);

    RoutineRepository& routines_;
    Select* routine_select_ = nullptr;
    QVBoxLayout* today_body_ = nullptr;
    QVBoxLayout* schedule_body_ = nullptr;

    std::vector<Routine> routine_list_;
    std::vector<RoutineItem> items_;
    std::int64_t current_routine_id_ = 0;
};

}  // namespace dayflow
