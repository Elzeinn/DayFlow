#include "ui/task_dialog.hpp"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "ui/date_field.hpp"
#include "ui/select.hpp"
#include "ui/theme.hpp"
#include "ui/time_util.hpp"

namespace dayflow {

TaskDialog::TaskDialog(const std::optional<Task>& existing, QWidget* parent)
    : QDialog(parent), task_(existing.value_or(Task{}))
{
    const bool editing = existing.has_value();

    setWindowTitle(editing ? QStringLiteral("Edit Tugas") : QStringLiteral("Tugas Baru"));
    setMinimumWidth(460);

    auto* layout = new QVBoxLayout{this};
    layout->setContentsMargins(22, 20, 22, 20);
    layout->setSpacing(14);

    auto* heading = new QLabel{editing ? QStringLiteral("Edit Tugas") : QStringLiteral("Tugas Baru")};
    heading->setObjectName(QStringLiteral("dialogTitle"));
    heading->setFont(theme::dialog_title_font());
    layout->addWidget(heading);

    auto* form = new QFormLayout{};
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    title_ = new QLineEdit{this};
    title_->setObjectName(QStringLiteral("field"));
    title_->setPlaceholderText(QStringLiteral("Contoh: baca 20 halaman"));
    form->addRow(QStringLiteral("Judul"), title_);

    priority_ = new Select{this};
    priority_->add_item(QStringLiteral("Low"), static_cast<int>(Priority::low));
    priority_->add_item(QStringLiteral("Normal"), static_cast<int>(Priority::normal));
    priority_->add_item(QStringLiteral("High"), static_cast<int>(Priority::high));
    priority_->add_item(QStringLiteral("Urgent"), static_cast<int>(Priority::urgent));
    priority_->set_current_index(1);
    form->addRow(QStringLiteral("Prioritas"), priority_);

    due_ = new DateField{this};
    due_->set_allow_empty(true);
    due_->set_empty();
    form->addRow(QStringLiteral("Tenggat"), due_);

    notes_ = new QLineEdit{this};
    notes_->setObjectName(QStringLiteral("field"));
    notes_->setPlaceholderText(QStringLiteral("Opsional"));
    form->addRow(QStringLiteral("Catatan"), notes_);

    layout->addLayout(form);

    auto* buttons = new QHBoxLayout{};
    buttons->setSpacing(8);
    auto* delete_button = new QPushButton{QStringLiteral("Hapus"), this};
    delete_button->setObjectName(QStringLiteral("danger"));
    delete_button->setVisible(editing);
    auto* cancel_button = new QPushButton{QStringLiteral("Batal"), this};
    cancel_button->setObjectName(QStringLiteral("secondary"));
    auto* save_button = new QPushButton{QStringLiteral("Simpan"), this};
    save_button->setObjectName(QStringLiteral("primary"));
    save_button->setDefault(true);

    buttons->addWidget(delete_button);
    buttons->addStretch(1);
    buttons->addWidget(cancel_button);
    buttons->addWidget(save_button);
    layout->addSpacing(4);
    layout->addLayout(buttons);

    if (editing) {
        title_->setText(QString::fromStdString(task_.title));
        notes_->setText(QString::fromStdString(task_.notes));
        priority_->set_current_index(static_cast<int>(task_.priority));
        if (task_.due_at.has_value()) {
            due_->set_value(local_from_utc(*task_.due_at));
        }
    }

    connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);
    connect(save_button, &QPushButton::clicked, this, &TaskDialog::accept);
    connect(delete_button, &QPushButton::clicked, this, [this] {
        delete_requested_ = true;
        done(QDialog::Accepted);
    });
}

void TaskDialog::accept()
{
    const QString title = title_->text().trimmed();
    if (title.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("DayFlow"), QStringLiteral("Judul tugas tidak boleh kosong."));
        return;
    }

    task_.title = title.toStdString();
    task_.notes = notes_->text().trimmed().toStdString();
    task_.priority = static_cast<Priority>(priority_->current_data().toInt());

    if (due_->is_empty()) {
        task_.due_at.reset();
    } else {
        task_.due_at = utc_from_local(due_->value());
    }

    if (Error error = validate(task_)) {
        QMessageBox::warning(this, QStringLiteral("DayFlow"), QString::fromStdString(error.message));
        return;
    }

    QDialog::accept();
}

}  // namespace dayflow
