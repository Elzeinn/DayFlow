#include "ui/notes_page.hpp"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

#include "data/note_repository.hpp"
#include "ui/theme.hpp"

namespace dayflow {
namespace {

constexpr int k_id_role = Qt::UserRole;

}  // namespace

NotesPage::NotesPage(NoteRepository& notes, QWidget* parent) : QWidget(parent), notes_(notes)
{
    auto* layout = new QVBoxLayout{this};
    layout->setContentsMargins(28, 22, 28, 22);
    layout->setSpacing(18);

    auto* header = new QHBoxLayout{};
    auto* header_text = new QVBoxLayout{};
    header_text->setSpacing(6);
    header_text->addWidget(theme::make_micro_label(QStringLiteral("CATATAN CEPAT")));
    auto* title = new QLabel{QStringLiteral("Catatan")};
    title->setObjectName(QStringLiteral("displayTitle"));
    title->setFont(theme::display_font());
    title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    title->setFixedHeight(40);
    header_text->addWidget(title);
    header->addLayout(header_text);
    header->addStretch(1);

    auto* new_button = new QPushButton{QStringLiteral("+ CATATAN BARU")};
    new_button->setObjectName(QStringLiteral("primary"));
    new_button->setCursor(Qt::PointingHandCursor);
    header->addWidget(new_button);
    layout->addLayout(header);

    auto* content = new QHBoxLayout{};
    content->setSpacing(16);

    QVBoxLayout* list_body = nullptr;
    auto* list_card = make_card(QStringLiteral("DAFTAR"), &list_body);
    list_card->setFixedWidth(280);
    list_ = new QListWidget{};
    list_->setObjectName(QStringLiteral("noteList"));
    list_->setSpacing(2);
    list_body->addWidget(list_, 1);
    content->addWidget(list_card);

    QVBoxLayout* editor_body = nullptr;
    auto* editor_card = make_card(QStringLiteral("ISI CATATAN"), &editor_body);

    auto* actions = new QHBoxLayout{};
    actions->setSpacing(8);
    pin_button_ = new QPushButton{QStringLiteral("SEMATKAN")};
    pin_button_->setObjectName(QStringLiteral("miniButton"));
    pin_button_->setCursor(Qt::PointingHandCursor);
    auto* delete_button = new QPushButton{QStringLiteral("HAPUS")};
    delete_button->setObjectName(QStringLiteral("danger"));
    actions->addWidget(pin_button_);
    actions->addWidget(delete_button);
    actions->addStretch(1);
    editor_body->addLayout(actions);

    editor_ = new QTextEdit{};
    editor_->setObjectName(QStringLiteral("noteEditor"));
    editor_->setPlaceholderText(QStringLiteral("Tulis apa saja... tersimpan otomatis."));
    editor_body->addWidget(editor_, 1);
    content->addWidget(editor_card, 1);

    layout->addLayout(content, 1);

    save_timer_ = new QTimer{this};
    save_timer_->setSingleShot(true);
    save_timer_->setInterval(700);

    connect(new_button, &QPushButton::clicked, this, &NotesPage::new_note);
    connect(pin_button_, &QPushButton::clicked, this, &NotesPage::toggle_pin);
    connect(delete_button, &QPushButton::clicked, this, &NotesPage::remove_current);
    connect(save_timer_, &QTimer::timeout, this, &NotesPage::save_current);
    connect(editor_, &QTextEdit::textChanged, this, [this] {
        if (loading_) {
            return;
        }
        save_timer_->start();
    });
    connect(list_, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current, QListWidgetItem*) {
        if (current == nullptr) {
            return;
        }
        open_note(static_cast<std::int64_t>(current->data(k_id_role).toLongLong()));
    });

    reload();
}

QWidget* NotesPage::make_card(const QString& title, QVBoxLayout** body)
{
    auto* card = new QFrame{};
    card->setObjectName(QStringLiteral("card"));

    auto* layout = new QVBoxLayout{card};
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);
    layout->addWidget(theme::make_micro_label(title));

    auto* inner = new QVBoxLayout{};
    inner->setContentsMargins(0, 0, 0, 0);
    inner->setSpacing(6);
    layout->addLayout(inner, 1);

    *body = inner;
    return card;
}

void NotesPage::showEvent(QShowEvent*)
{
    reload();
}

void NotesPage::reload()
{
    auto listed = notes_.list_all();
    if (!listed.ok()) {
        report(listed.error());
        return;
    }

    const QSignalBlocker blocker{list_};
    list_->clear();

    int selected_row = -1;
    for (const Note& note : listed.value()) {
        QString text = note_title(note.body).empty() ? QStringLiteral("Catatan kosong")
                                                     : QString::fromStdString(note_title(note.body));
        if (note.pinned) {
            text.prepend(QStringLiteral("[!] "));
        }

        auto* item = new QListWidgetItem{text};
        item->setData(k_id_role, static_cast<qlonglong>(note.id));
        list_->addItem(item);

        if (note.id == current_id_) {
            selected_row = list_->count() - 1;
        }
    }

    if (selected_row >= 0) {
        list_->setCurrentRow(selected_row);
    }
}

void NotesPage::open_note(std::int64_t id)
{
    auto found = notes_.find(id);
    if (!found.ok()) {
        report(found.error());
        return;
    }
    if (!found.value().has_value()) {
        reload();
        return;
    }

    current_id_ = id;
    loading_ = true;
    editor_->setPlainText(QString::fromStdString(found.value()->body));
    loading_ = false;

    pin_button_->setText(found.value()->pinned ? QStringLiteral("LEPAS SEMATAN") : QStringLiteral("SEMATKAN"));
}

void NotesPage::open_note_id(std::int64_t id)
{
    open_note(id);

    for (int row = 0; row < list_->count(); ++row) {
        auto* item = list_->item(row);
        if (item != nullptr && static_cast<std::int64_t>(item->data(k_id_role).toLongLong()) == id) {
            const QSignalBlocker blocker{list_};
            list_->setCurrentRow(row);
            break;
        }
    }
}

void NotesPage::new_note()
{
    save_current();

    current_id_ = 0;
    loading_ = true;
    editor_->clear();
    loading_ = false;
    pin_button_->setText(QStringLiteral("SEMATKAN"));
    editor_->setFocus();
}

void NotesPage::save_current()
{
    const QString body = editor_->toPlainText();
    if (body.trimmed().isEmpty()) {
        return;
    }

    if (current_id_ == 0) {
        Note note;
        note.body = body.toStdString();
        auto created = notes_.create(note);
        if (!created.ok()) {
            report(created.error());
            return;
        }
        current_id_ = created.value().id;
    } else {
        auto found = notes_.find(current_id_);
        if (!found.ok() || !found.value().has_value()) {
            return;
        }
        Note note = *found.value();
        note.body = body.toStdString();
        if (Error error = notes_.update(note)) {
            report(error);
            return;
        }
    }

    reload();
}

void NotesPage::toggle_pin()
{
    if (current_id_ == 0) {
        return;
    }

    auto found = notes_.find(current_id_);
    if (!found.ok() || !found.value().has_value()) {
        return;
    }

    Note note = *found.value();
    note.pinned = !note.pinned;
    note.body = editor_->toPlainText().toStdString();
    if (Error error = notes_.update(note)) {
        report(error);
        return;
    }

    pin_button_->setText(note.pinned ? QStringLiteral("LEPAS SEMATAN") : QStringLiteral("SEMATKAN"));
    reload();
}

void NotesPage::remove_current()
{
    if (current_id_ == 0) {
        return;
    }
    if (Error error = notes_.remove(current_id_)) {
        report(error);
        return;
    }

    current_id_ = 0;
    loading_ = true;
    editor_->clear();
    loading_ = false;
    reload();
}

void NotesPage::report(const Error& error)
{
    QMessageBox::warning(this, QStringLiteral("DayFlow"), QString::fromStdString(error.message));
}

}  // namespace dayflow
