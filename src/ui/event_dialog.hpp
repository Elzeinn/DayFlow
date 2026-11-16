#pragma once

#include <QDate>
#include <QDialog>

#include <optional>

#include "core/event.hpp"

class QLineEdit;

namespace dayflow {

class DateField;
class EventCategoryRepository;
class Select;

class EventDialog : public QDialog {
    Q_OBJECT

public:
    EventDialog(EventCategoryRepository& categories, std::optional<Event> existing, const QDate& default_date,
        QWidget* parent = nullptr);

    Event result() const { return event_; }
    bool wants_delete() const { return delete_requested_; }

protected:
    void accept() override;

private:
    EventCategoryRepository& categories_;
    Event event_;
    bool delete_requested_ = false;

    QLineEdit* title_ = nullptr;
    Select* category_ = nullptr;
    DateField* start_ = nullptr;
    Select* duration_ = nullptr;
    Select* repeat_ = nullptr;
    QLineEdit* notes_ = nullptr;
};

}  // namespace dayflow
