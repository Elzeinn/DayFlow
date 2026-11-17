#include "ui/search_dialog.hpp"

#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

#include "core/note.hpp"
#include "data/event_repository.hpp"
#include "data/note_repository.hpp"
#include "data/task_repository.hpp"
#include "ui/app_context.hpp"
#include "ui/theme.hpp"

namespace dayflow {
namespace {

constexpr int k_kind_role = Qt::UserRole;
constexpr int k_id_role = Qt::UserRole + 1;
constexpr std::size_t k_limit_per_kind = 6;

void add_item(QListWidget* list, const QString& tag, const QString& text, SearchKind kind, std::int64_t id)
{
    auto* item = new QListWidgetItem{QStringLiteral("%1  ·  %2").arg(tag, text)};
    item->setData(k_kind_role, static_cast<int>(kind));
    item->setData(k_id_role, static_cast<qlonglong>(id));
    list->addItem(item);
}

}  // namespace

SearchDialog::SearchDialog(const AppContext& context, QWidget* parent) : QDialog(parent), context_(context)
{
    setWindowFlag(Qt::FramelessWindowHint, true);
    setWindowTitle(QStringLiteral("Cari"));
    resize(640, 420);

    auto* layout = new QVBoxLayout{this};
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    field_ = new QLineEdit{this};
    field_->setObjectName(QStringLiteral("searchField"));
    field_->setPlaceholderText(QStringLiteral("Cari tugas, catatan, atau event..."));
    layout->addWidget(field_);

    results_ = new QListWidget{this};
    results_->setObjectName(QStringLiteral("searchResults"));
    results_->setSpacing(2);
    layout->addWidget(results_, 1);

    auto* hint = new QLabel{QStringLiteral("↑↓ pilih · Enter buka · Esc tutup")};
    hint->setObjectName(QStringLiteral("searchHint"));
    layout->addWidget(hint);

    connect(field_, &QLineEdit::textChanged, this, [this](const QString& text) { refresh(text); });
    connect(results_, &QListWidget::itemActivated, this, [this](QListWidgetItem*) { activate(); });
    connect(results_, &QListWidget::itemClicked, this, [this](QListWidgetItem*) { activate(); });

    refresh(QString{});
    field_->setFocus();
}

void SearchDialog::refresh(const QString& query)
{
    results_->clear();

    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    const std::string text = trimmed.toStdString();

    auto tasks = context_.tasks.search(text, k_limit_per_kind);
    if (tasks.ok()) {
        for (const Task& task : tasks.value()) {
            add_item(results_, QStringLiteral("TUGAS"), QString::fromStdString(task.title), SearchKind::task, task.id);
        }
    }

    auto notes = context_.notes.search(text, k_limit_per_kind);
    if (notes.ok()) {
        for (const Note& note : notes.value()) {
            add_item(results_, QStringLiteral("CATATAN"), QString::fromStdString(note_title(note.body)),
                SearchKind::note, note.id);
        }
    }

    auto events = context_.events.search(text, k_limit_per_kind);
    if (events.ok()) {
        for (const Event& event : events.value()) {
            add_item(results_, QStringLiteral("EVENT"), QString::fromStdString(event.title), SearchKind::event, event.id);
        }
    }

    if (results_->count() > 0) {
        results_->setCurrentRow(0);
    }
}

void SearchDialog::activate()
{
    QListWidgetItem* item = results_->currentItem();
    if (item == nullptr) {
        return;
    }

    choice_.kind = static_cast<SearchKind>(item->data(k_kind_role).toInt());
    choice_.id = static_cast<std::int64_t>(item->data(k_id_role).toLongLong());
    accept();
}

void SearchDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        activate();
        return;
    }
    if (event->key() == Qt::Key_Down) {
        const int row = results_->currentRow();
        if (row + 1 < results_->count()) {
            results_->setCurrentRow(row + 1);
        }
        return;
    }
    if (event->key() == Qt::Key_Up) {
        const int row = results_->currentRow();
        if (row > 0) {
            results_->setCurrentRow(row - 1);
        }
        return;
    }
    QDialog::keyPressEvent(event);
}

}  // namespace dayflow
