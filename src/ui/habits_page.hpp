#pragma once

#include <QWidget>

#include <cstdint>

#include "core/error.hpp"

class QVBoxLayout;

namespace dayflow {

class HabitRepository;

class HabitsPage : public QWidget {
    Q_OBJECT

public:
    explicit HabitsPage(HabitRepository& habits, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;

private:
    QWidget* make_card(const QString& title, QVBoxLayout** body);
    void reload();
    void add_habit();
    void remove_habit(std::int64_t id);
    void toggle_today(std::int64_t id, bool done);
    void report(const Error& error);

    HabitRepository& habits_;
    QVBoxLayout* body_ = nullptr;
};

}  // namespace dayflow
