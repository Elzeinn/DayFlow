#include "ui/routines_page.hpp"

#include <QCheckBox>
#include <QDate>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>

#include <map>

#include "data/routine_repository.hpp"
#include "ui/routine_item_dialog.hpp"
#include "ui/select.hpp"
#include "ui/text_input_dialog.hpp"
#include "ui/theme.hpp"

namespace dayflow {
namespace {

const char* k_day_names[] = {"SEN", "SEL", "RAB", "KAM", "JUM", "SAB", "MIN"};

void clear_layout(QVBoxLayout* layout)
{
    while (QLayoutItem* item = layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
}

QString describe_days(int mask)
{
    QStringList names;
    for (int index = 0; index < 7; ++index) {
        if ((mask & (1 << index)) != 0) {
            names << QString::fromLatin1(k_day_names[index]);
        }
    }
    if (names.size() == 7) {
        return QStringLiteral("Setiap hari");
    }
    return names.join(QStringLiteral(" · "));
}

}  // namespace

RoutinesPage::RoutinesPage(RoutineRepository& routines, QWidget* parent) : QWidget(parent), routines_(routines)
{
    auto* layout = new QVBoxLayout{this};
    layout->setContentsMargins(28, 22, 28, 22);
    layout->setSpacing(18);

    auto* header = new QVBoxLayout{};
    header->setSpacing(6);
    header->addWidget(theme::make_micro_label(QStringLiteral("RENCANA HARIAN")));
    auto* title = new QLabel{QStringLiteral("Rutinitas")};
    title->setObjectName(QStringLiteral("displayTitle"));
    title->setFont(theme::display_font());
    title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    title->setFixedHeight(40);
    header->addWidget(title);
    layout->addLayout(header);
    layout->addWidget(build_toolbar());

    auto* content = new QHBoxLayout{};
    content->setSpacing(16);
    content->addWidget(make_card(QStringLiteral("HARI INI"), &today_body_), 1);
    content->addWidget(make_card(QStringLiteral("JADWAL MINGGUAN"), &schedule_body_), 1);
    layout->addLayout(content, 1);

    reload();
}

QWidget* RoutinesPage::build_toolbar()
{
    auto* toolbar = new QWidget{};
    auto* layout = new QHBoxLayout{toolbar};
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    routine_select_ = new Select{};
    routine_select_->setMinimumWidth(220);

    auto* add_routine = new QPushButton{QStringLiteral("+ RUTINITAS")};
    add_routine->setObjectName(QStringLiteral("secondary"));
    auto* add_item = new QPushButton{QStringLiteral("+ KEGIATAN")};
    add_item->setObjectName(QStringLiteral("primary"));
    auto* remove = new QPushButton{QStringLiteral("HAPUS")};
    remove->setObjectName(QStringLiteral("danger"));

    layout->addWidget(routine_select_);
    layout->addWidget(add_routine);
    layout->addWidget(add_item);
    layout->addStretch(1);
    layout->addWidget(remove);

    connect(routine_select_, &Select::changed, this, [this](int) {
        current_routine_id_ = routine_select_->current_data().toLongLong();
        reload();
    });
    connect(add_routine, &QPushButton::clicked, this, &RoutinesPage::add_routine);
    connect(add_item, &QPushButton::clicked, this, &RoutinesPage::add_item);
    connect(remove, &QPushButton::clicked, this, &RoutinesPage::remove_routine);

    return toolbar;
}

QWidget* RoutinesPage::make_card(const QString& title, QVBoxLayout** body)
{
    auto* card = new QFrame{};
    card->setObjectName(QStringLiteral("card"));

    auto* layout = new QVBoxLayout{card};
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);
    layout->addWidget(theme::make_micro_label(title));

    auto* inner = new QVBoxLayout{};
    inner->setContentsMargins(0, 0, 0, 0);
    inner->setSpacing(6);
    layout->addLayout(inner, 1);

    *body = inner;
    return card;
}

void RoutinesPage::showEvent(QShowEvent*)
{
    reload();
}

void RoutinesPage::reload()
{
    auto listed = routines_.list_routines();
    if (!listed.ok()) {
        report(listed.error());
        return;
    }
    routine_list_ = std::move(listed.value());

    {
        const QSignalBlocker blocker{routine_select_};
        routine_select_->clear_items();
        for (const Routine& routine : routine_list_) {
            routine_select_->add_item(QString::fromStdString(routine.name), static_cast<qlonglong>(routine.id));
        }

        bool found = false;
        for (int index = 0; index < static_cast<int>(routine_list_.size()); ++index) {
            if (routine_list_[static_cast<std::size_t>(index)].id == current_routine_id_) {
                routine_select_->set_current_index(index);
                found = true;
            }
        }
        if (!found) {
            current_routine_id_ = routine_list_.empty() ? 0 : routine_list_.front().id;
            if (!routine_list_.empty()) {
                routine_select_->set_current_index(0);
            }
        }
    }

    items_.clear();
    if (current_routine_id_ > 0) {
        auto loaded = routines_.list_items(current_routine_id_);
        if (!loaded.ok()) {
            report(loaded.error());
            return;
        }
        items_ = std::move(loaded.value());
    }

    rebuild_schedule();
    rebuild_today();
}

void RoutinesPage::rebuild_schedule()
{
    clear_layout(schedule_body_);

    if (items_.empty()) {
        auto* empty = new QLabel{current_routine_id_ == 0 ? QStringLiteral("Belum ada rutinitas")
                                                          : QStringLiteral("Belum ada kegiatan")};
        empty->setObjectName(QStringLiteral("agendaEmpty"));
        empty->setWordWrap(true);
        schedule_body_->addWidget(empty);
    }

    for (const RoutineItem& item : items_) {
        auto* row = new QPushButton{QStringLiteral("%1  ·  %2\n%3")
                                        .arg(QString::fromStdString(format_hhmm(item.start_minute)),
                                            QString::fromStdString(item.title), describe_days(item.weekday_mask))};
        row->setObjectName(QStringLiteral("scheduleItem"));
        row->setCursor(Qt::PointingHandCursor);
        const std::int64_t item_id = item.id;
        connect(row, &QPushButton::clicked, this, [this, item_id] { edit_item(item_id); });
        schedule_body_->addWidget(row);
    }

    schedule_body_->addStretch(1);
}

void RoutinesPage::rebuild_today()
{
    clear_layout(today_body_);

    const QDate today = QDate::currentDate();
    const std::string today_text = today.toString(Qt::ISODate).toStdString();

    auto logs = routines_.logs_for_date(today_text);
    if (!logs.ok()) {
        report(logs.error());
        return;
    }

    int shown = 0;
    for (const RoutineItem& item : items_) {
        if (!runs_on(item, today.dayOfWeek())) {
            continue;
        }
        ++shown;

        auto* row = new QWidget{};
        row->setObjectName(QStringLiteral("routineRow"));
        row->setAttribute(Qt::WA_StyledBackground, true);
        auto* layout = new QHBoxLayout{row};
        layout->setContentsMargins(10, 6, 10, 6);
        layout->setSpacing(10);

        const auto found = logs.value().find(item.id);
        const bool done = found != logs.value().end() && found->second == RoutineLogStatus::done;
        const bool skipped = found != logs.value().end() && found->second == RoutineLogStatus::skipped;

        auto* done_box = new QCheckBox{row};
        done_box->setObjectName(QStringLiteral("doneBox"));
        done_box->setChecked(done);
        layout->addWidget(done_box);

        auto* label = new QLabel{QStringLiteral("%1  %2")
                                     .arg(QString::fromStdString(format_hhmm(item.start_minute)),
                                         QString::fromStdString(item.title)),
            row};
        label->setObjectName(QStringLiteral("routineTitle"));
        layout->addWidget(label, 1);

        auto* skip = new QPushButton{skipped ? QStringLiteral("BATAL") : QStringLiteral("LEWATI"), row};
        skip->setObjectName(QStringLiteral("miniButton"));
        skip->setCursor(Qt::PointingHandCursor);
        layout->addWidget(skip);

        const std::int64_t item_id = item.id;
        connect(done_box, &QCheckBox::toggled, this, [this, item_id](bool checked) {
            mark(item_id, RoutineLogStatus::done, checked);
        });
        connect(skip, &QPushButton::clicked, this, [this, item_id, skipped] {
            mark(item_id, RoutineLogStatus::skipped, !skipped);
        });

        today_body_->addWidget(row);
    }

    if (shown == 0) {
        auto* empty = new QLabel{QStringLiteral("Tidak ada kegiatan untuk hari ini")};
        empty->setObjectName(QStringLiteral("agendaEmpty"));
        empty->setWordWrap(true);
        today_body_->addWidget(empty);
    }

    today_body_->addStretch(1);
}

void RoutinesPage::mark(std::int64_t item_id, RoutineLogStatus status, bool toggle)
{
    const std::string today = QDate::currentDate().toString(Qt::ISODate).toStdString();

    Error error;
    if (toggle) {
        error = routines_.set_log(item_id, today, status);
    } else {
        error = routines_.clear_log(item_id, today);
    }
    if (error) {
        report(error);
        return;
    }

    QTimer::singleShot(0, this, [this] { rebuild_today(); });
}

void RoutinesPage::add_routine()
{
    TextInputDialog dialog{QStringLiteral("Rutinitas Baru"), QStringLiteral("Nama rutinitas"), {}, this};
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    Routine routine;
    routine.name = dialog.text().toStdString();

    auto created = routines_.create_routine(routine);
    if (!created.ok()) {
        report(created.error());
        return;
    }
    current_routine_id_ = created.value().id;
    reload();
}

void RoutinesPage::remove_routine()
{
    if (current_routine_id_ <= 0) {
        return;
    }
    if (Error error = routines_.remove_routine(current_routine_id_)) {
        report(error);
        return;
    }
    current_routine_id_ = 0;
    reload();
}

void RoutinesPage::add_item()
{
    if (current_routine_id_ <= 0) {
        QMessageBox::information(this, QStringLiteral("DayFlow"), QStringLiteral("Buat rutinitas dulu."));
        return;
    }

    RoutineItemDialog dialog{current_routine_id_, std::nullopt, this};
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    auto created = routines_.create_item(dialog.result());
    if (!created.ok()) {
        report(created.error());
        return;
    }
    reload();
}

void RoutinesPage::edit_item(std::int64_t id)
{
    RoutineItem existing;
    bool found = false;
    for (const RoutineItem& item : items_) {
        if (item.id == id) {
            existing = item;
            found = true;
            break;
        }
    }
    if (!found) {
        return;
    }

    RoutineItemDialog dialog{current_routine_id_, existing, this};
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    if (dialog.wants_delete()) {
        if (Error error = routines_.remove_item(id)) {
            report(error);
            return;
        }
        reload();
        return;
    }

    RoutineItem changed = dialog.result();
    changed.id = id;
    if (Error error = routines_.update_item(changed)) {
        report(error);
        return;
    }
    reload();
}

void RoutinesPage::report(const Error& error)
{
    QMessageBox::warning(this, QStringLiteral("DayFlow"), QString::fromStdString(error.message));
}

}  // namespace dayflow
