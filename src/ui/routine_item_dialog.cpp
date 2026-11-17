#include "ui/routine_item_dialog.hpp"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTimeEdit>
#include <QVBoxLayout>

#include "ui/dialog_header.hpp"
#include "ui/select.hpp"

namespace dayflow {
namespace {

const char* k_day_names[] = {"SEN", "SEL", "RAB", "KAM", "JUM", "SAB", "MIN"};
constexpr int k_duration_choices[] = {5, 10, 15, 20, 30, 45, 60, 90, 120};

}  // namespace

RoutineItemDialog::RoutineItemDialog(std::int64_t routine_id, std::optional<RoutineItem> existing, QWidget* parent)
    : QDialog(parent), item_(existing.value_or(RoutineItem{}))
{
    const bool editing = existing.has_value();
    item_.routine_id = routine_id;

    setWindowTitle(editing ? QStringLiteral("Edit Kegiatan") : QStringLiteral("Kegiatan Baru"));
    setWindowFlag(Qt::FramelessWindowHint, true);
    setMinimumWidth(470);

    auto* layout = new QVBoxLayout{this};
    layout->setContentsMargins(0, 0, 0, 20);
    layout->setSpacing(0);
    layout->addWidget(new DialogHeader{editing ? QStringLiteral("Edit Kegiatan") : QStringLiteral("Kegiatan Baru"),
        this, this});
    layout->addSpacing(10);

    auto* body = new QVBoxLayout{};
    body->setContentsMargins(22, 0, 22, 0);
    body->setSpacing(14);
    layout->addLayout(body);

    auto* form = new QFormLayout{};
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    title_ = new QLineEdit{this};
    title_->setObjectName(QStringLiteral("field"));
    title_->setPlaceholderText(QStringLiteral("Contoh: minum air putih"));
    form->addRow(QStringLiteral("Kegiatan"), title_);

    auto* days_row = new QWidget{this};
    auto* days_layout = new QHBoxLayout{days_row};
    days_layout->setContentsMargins(0, 0, 0, 0);
    days_layout->setSpacing(4);
    for (int index = 0; index < 7; ++index) {
        auto* button = new QPushButton{QString::fromLatin1(k_day_names[index]), days_row};
        button->setObjectName(QStringLiteral("dayToggle"));
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        day_buttons_[static_cast<std::size_t>(index)] = button;
        days_layout->addWidget(button);
    }
    days_layout->addStretch(1);
    form->addRow(QStringLiteral("Hari"), days_row);

    start_ = new QTimeEdit{this};
    start_->setObjectName(QStringLiteral("field"));
    start_->setDisplayFormat(QStringLiteral("HH:mm"));
    start_->setTime(QTime{7, 0});
    form->addRow(QStringLiteral("Mulai"), start_);

    duration_ = new Select{this};
    for (int choice : k_duration_choices) {
        duration_->add_item(QStringLiteral("%1 menit").arg(choice), choice);
    }
    duration_->set_current_index(duration_->index_of_data(30));
    form->addRow(QStringLiteral("Durasi"), duration_);

    body->addLayout(form);

    auto* buttons = new QHBoxLayout{};
    buttons->setSpacing(8);
    auto* delete_button = new QPushButton{QStringLiteral("Hapus"), this};
    delete_button->setObjectName(QStringLiteral("danger"));
    delete_button->setVisible(editing);
    auto* cancel = new QPushButton{QStringLiteral("Batal"), this};
    cancel->setObjectName(QStringLiteral("secondary"));
    auto* save = new QPushButton{QStringLiteral("Simpan"), this};
    save->setObjectName(QStringLiteral("primary"));
    save->setDefault(true);

    buttons->addWidget(delete_button);
    buttons->addStretch(1);
    buttons->addWidget(cancel);
    buttons->addWidget(save);
    body->addSpacing(4);
    body->addLayout(buttons);

    if (editing) {
        title_->setText(QString::fromStdString(item_.title));
        for (int index = 0; index < 7; ++index) {
            day_buttons_[static_cast<std::size_t>(index)]->setChecked((item_.weekday_mask & (1 << index)) != 0);
        }
        start_->setTime(QTime{item_.start_minute / 60, item_.start_minute % 60});
        const int index = duration_->index_of_data(item_.duration_minutes);
        if (index >= 0) {
            duration_->set_current_index(index);
        }
    } else {
        for (auto* button : day_buttons_) {
            button->setChecked(true);
        }
    }

    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(save, &QPushButton::clicked, this, &RoutineItemDialog::accept);
    connect(delete_button, &QPushButton::clicked, this, [this] {
        delete_requested_ = true;
        done(QDialog::Accepted);
    });
}

void RoutineItemDialog::accept()
{
    const QString title = title_->text().trimmed();
    if (title.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("DayFlow"), QStringLiteral("Nama kegiatan tidak boleh kosong."));
        return;
    }

    int mask = 0;
    for (int index = 0; index < 7; ++index) {
        if (day_buttons_[static_cast<std::size_t>(index)]->isChecked()) {
            mask |= 1 << index;
        }
    }
    if (mask == 0) {
        QMessageBox::warning(this, QStringLiteral("DayFlow"), QStringLiteral("Pilih minimal satu hari."));
        return;
    }

    item_.title = title.toStdString();
    item_.weekday_mask = mask;
    item_.start_minute = start_->time().hour() * 60 + start_->time().minute();
    item_.duration_minutes = duration_->current_data().toInt();

    if (Error error = validate(item_)) {
        QMessageBox::warning(this, QStringLiteral("DayFlow"), QString::fromStdString(error.message));
        return;
    }

    QDialog::accept();
}

}  // namespace dayflow
