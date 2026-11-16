#include "ui/event_dialog.hpp"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

#include "data/event_repository.hpp"
#include "ui/date_field.hpp"
#include "ui/select.hpp"
#include "ui/theme.hpp"
#include "ui/time_util.hpp"

namespace dayflow {
namespace {

constexpr int k_minimum_minutes = 15;

constexpr int k_duration_choices[] = {15, 30, 45, 60, 90, 120, 180, 240, 480};

}  // namespace

EventDialog::EventDialog(EventCategoryRepository& categories, std::optional<Event> existing,
    const QDate& default_date, QWidget* parent)
    : QDialog(parent), categories_(categories), event_(existing.value_or(Event{}))
{
    const bool editing = existing.has_value();

    setWindowTitle(editing ? QStringLiteral("Edit Event") : QStringLiteral("Event Baru"));
    setMinimumWidth(460);

    auto* layout = new QVBoxLayout{this};
    layout->setContentsMargins(22, 20, 22, 20);
    layout->setSpacing(14);

    auto* heading = new QLabel{editing ? QStringLiteral("Edit Event") : QStringLiteral("Event Baru")};
    heading->setObjectName(QStringLiteral("dialogTitle"));
    heading->setFont(theme::dialog_title_font());
    layout->addWidget(heading);

    auto* form = new QFormLayout{};
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    title_ = new QLineEdit{this};
    title_->setObjectName(QStringLiteral("field"));
    title_->setPlaceholderText(QStringLiteral("Contoh: rapat mingguan"));
    form->addRow(QStringLiteral("Judul"), title_);

    category_ = new Select{this};
    category_->add_item(QStringLiteral("Tanpa kategori"), QVariant{});
    auto categories_list = categories_.list_all();
    if (categories_list.ok()) {
        for (const EventCategory& category : categories_list.value()) {
            category_->add_item(QString::fromStdString(category.name), static_cast<qlonglong>(category.id),
                QColor{QString::fromStdString(category.color)});
        }
    }
    form->addRow(QStringLiteral("Kategori"), category_);

    start_ = new DateField{this};
    start_->set_allow_empty(false);
    start_->set_value(editing ? local_from_utc(event_.starts_at) : QDateTime{default_date, QTime{9, 0}});
    form->addRow(QStringLiteral("Mulai"), start_);

    const int existing_minutes = editing
        ? static_cast<int>(std::max<qint64>(k_minimum_minutes,
              std::chrono::duration_cast<std::chrono::minutes>(event_.ends_at - event_.starts_at).count()))
        : 60;

    duration_ = new Select{this};
    bool duration_listed = false;
    for (int choice : k_duration_choices) {
        duration_->add_item(QStringLiteral("%1 menit").arg(choice), choice);
        if (choice == existing_minutes) {
            duration_listed = true;
        }
    }
    if (!duration_listed) {
        duration_->add_item(QStringLiteral("%1 menit").arg(existing_minutes), existing_minutes);
    }
    duration_->set_current_index(qMax(0, duration_->index_of_data(existing_minutes)));
    form->addRow(QStringLiteral("Durasi"), duration_);

    repeat_ = new Select{this};
    repeat_->add_item(QStringLiteral("Tidak berulang"), static_cast<int>(RecurrenceFreq::none));
    repeat_->add_item(QStringLiteral("Harian"), static_cast<int>(RecurrenceFreq::daily));
    repeat_->add_item(QStringLiteral("Mingguan"), static_cast<int>(RecurrenceFreq::weekly));
    repeat_->add_item(QStringLiteral("Bulanan"), static_cast<int>(RecurrenceFreq::monthly));
    if (editing && event_.recurrence.has_value()) {
        const int index = repeat_->index_of_data(static_cast<int>(event_.recurrence->freq));
        if (index >= 0) {
            repeat_->set_current_index(index);
        }
    }
    form->addRow(QStringLiteral("Ulangi"), repeat_);

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
        title_->setText(QString::fromStdString(event_.title));
        notes_->setText(QString::fromStdString(event_.description));
        if (event_.category_id.has_value()) {
            const int index = category_->index_of_data(static_cast<qlonglong>(*event_.category_id));
            if (index >= 0) {
                category_->set_current_index(index);
            }
        }
    }

    connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);
    connect(save_button, &QPushButton::clicked, this, &EventDialog::accept);
    connect(delete_button, &QPushButton::clicked, this, [this] {
        delete_requested_ = true;
        done(QDialog::Accepted);
    });
}

void EventDialog::accept()
{
    const QString title = title_->text().trimmed();
    if (title.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("DayFlow"), QStringLiteral("Judul event tidak boleh kosong."));
        return;
    }

    event_.title = title.toStdString();
    event_.description = notes_->text().trimmed().toStdString();

    const QVariant category_data = category_->current_data();
    if (category_data.isValid() && !category_data.isNull()) {
        event_.category_id = static_cast<std::int64_t>(category_data.toLongLong());
    } else {
        event_.category_id.reset();
    }

    const QDateTime local_start = start_->value();
    event_.starts_at = utc_from_local(local_start);

    const int minutes = qMax(k_minimum_minutes, duration_->current_data().toInt());
    event_.ends_at = event_.starts_at + std::chrono::seconds{minutes * 60};

    const int frequency = repeat_->current_data().toInt();
    if (frequency <= static_cast<int>(RecurrenceFreq::none)) {
        event_.recurrence.reset();
    } else {
        RecurrenceRule rule;
        rule.freq = static_cast<RecurrenceFreq>(frequency);
        rule.interval = 1;
        if (rule.freq == RecurrenceFreq::weekly) {
            rule.by_weekday = {local_start.date().dayOfWeek()};
        }
        event_.recurrence = rule;
    }

    if (Error error = validate(event_)) {
        QMessageBox::warning(this, QStringLiteral("DayFlow"), QString::fromStdString(error.message));
        return;
    }

    QDialog::accept();
}

}  // namespace dayflow
