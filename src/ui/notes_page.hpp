#pragma once

#include <QWidget>

#include <cstdint>

#include "core/error.hpp"

class QListWidget;
class QPushButton;
class QTextEdit;
class QTimer;
class QVBoxLayout;

namespace dayflow {

class NoteRepository;

class NotesPage : public QWidget {
    Q_OBJECT

public:
    explicit NotesPage(NoteRepository& notes, QWidget* parent = nullptr);

    void open_note_id(std::int64_t id);

protected:
    void showEvent(QShowEvent* event) override;

private:
    QWidget* make_card(const QString& title, QVBoxLayout** body);
    void reload();
    void open_note(std::int64_t id);
    void new_note();
    void save_current();
    void toggle_pin();
    void remove_current();
    void report(const Error& error);

    NoteRepository& notes_;
    QListWidget* list_ = nullptr;
    QTextEdit* editor_ = nullptr;
    QPushButton* pin_button_ = nullptr;
    QTimer* save_timer_ = nullptr;
    std::int64_t current_id_ = 0;
    bool loading_ = false;
};

}  // namespace dayflow
