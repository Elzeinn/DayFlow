#include "ui/habits_page.hpp"

#include <QCheckBox>
#include <QDate>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <set>

#include "core/habit.hpp"
#include "data/routine_repository.hpp"
#include "ui/text_input_dialog.hpp"
#include "ui/theme.hpp"

namespace dayflow {
namespace {

void clear_layout(QVBoxLayout* layout)
{
    while (QLayoutItem* item = layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
}

QFrame* make_dot(bool on)
{
    auto* dot = new QFrame{};
    dot->setObjectName(QStringLiteral("habitDot"));
    dot->setProperty("on", on);
    dot->setFixedSize(10, 10);
    dot->setToolTip(on ? QStringLiteral("selesai") : QStringLiteral("belum"));
    return dot;
}

}  // namespace

HabitsPage::HabitsPage(HabitRepository& habits, QWidget* parent) : QWidget(parent), habits_(habits)
{
    auto* layout = new QVBoxLayout{this};
    layout->setContentsMargins(28, 22, 28, 22);
    layout->setSpacing(18);

    auto* header = new QVBoxLayout{};
    header->setSpacing(6);
    header->addWidget(theme::make_micro_label(QStringLiteral("KONSISTENSI")));
    auto* title = new QLabel{QStringLiteral("Kebiasaan")};
    title->setObjectName(QStringLiteral("displayTitle"));
    title->setFont(theme::display_font());
    title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    title->setFixedHeight(40);
    header->addWidget(title);
    layout->addLayout(header);

    auto* toolbar = new QHBoxLayout{};
    toolbar->addStretch(1);
    auto* add = new QPushButton{QStringLiteral("+ KEBIASAAN")};
    add->setObjectName(QStringLiteral("primary"));
    add->setCursor(Qt::PointingHandCursor);
    toolbar->addWidget(add);
    layout->addLayout(toolbar);

    layout->addWidget(make_card(QStringLiteral("DAFTAR KEBIASAAN"), &body_), 1);

    connect(add, &QPushButton::clicked, this, &HabitsPage::add_habit);

    reload();
}

QWidget* HabitsPage::make_card(const QString& title, QVBoxLayout** body)
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

void HabitsPage::showEvent(QShowEvent*)
{
    reload();
}

void HabitsPage::reload()
{
    clear_layout(body_);

    auto listed = habits_.list_all();
    if (!listed.ok()) {
        report(listed.error());
        return;
    }

    const QDate today = QDate::currentDate();
    const std::string today_text = today.toString(Qt::ISODate).toStdString();
    const QDate history_start = today.addDays(-59);

    if (listed.value().empty()) {
        auto* empty = new QLabel{QStringLiteral("Belum ada kebiasaan. Tambahkan yang pertama.")};
        empty->setObjectName(QStringLiteral("agendaEmpty"));
        empty->setWordWrap(true);
        body_->addWidget(empty);
        body_->addStretch(1);
        return;
    }

    for (const Habit& habit : listed.value()) {
        auto dates = habits_.dates_for(habit.id, history_start.toString(Qt::ISODate).toStdString(), today_text);
        if (!dates.ok()) {
            report(dates.error());
            continue;
        }

        std::set<LocalDate> done_days;
        std::set<QString> done_text;
        for (const std::string& date : dates.value()) {
            auto parsed = parse_local_date(date);
            if (parsed.ok()) {
                done_days.insert(parsed.value());
                done_text.insert(QString::fromStdString(date));
            }
        }

        auto parsed_today = parse_local_date(today_text);
        const int streak = parsed_today.ok() ? current_streak(done_days, parsed_today.value()) : 0;
        const bool today_done = done_text.count(QString::fromStdString(today_text)) > 0;

        auto* row = new QWidget{};
        row->setObjectName(QStringLiteral("routineRow"));
        row->setAttribute(Qt::WA_StyledBackground, true);
        auto* layout = new QHBoxLayout{row};
        layout->setContentsMargins(10, 8, 10, 8);
        layout->setSpacing(12);

        auto* check = new QCheckBox{row};
        check->setObjectName(QStringLiteral("doneBox"));
        check->setChecked(today_done);
        layout->addWidget(check);

        auto* text_column = new QVBoxLayout{};
        text_column->setSpacing(2);
        auto* name = new QLabel{QString::fromStdString(habit.name), row};
        name->setObjectName(QStringLiteral("taskTitle"));
        text_column->addWidget(name);

        auto* streak_label = new QLabel{streak > 0 ? QStringLiteral("%1 hari berturut-turut").arg(streak)
                                                  : QStringLiteral("Belum ada streak"),
            row};
        streak_label->setObjectName(QStringLiteral("habitStreak"));
        text_column->addWidget(streak_label);
        layout->addLayout(text_column, 1);

        auto* strip = new QHBoxLayout{};
        strip->setSpacing(4);
        for (int offset = 6; offset >= 0; --offset) {
            const QString day = today.addDays(-offset).toString(Qt::ISODate);
            strip->addWidget(make_dot(done_text.count(day) > 0));
        }
        layout->addLayout(strip);

        auto* remove = new QPushButton{QStringLiteral("\u2715"), row};
        remove->setObjectName(QStringLiteral("rowAction"));
        remove->setFixedSize(26, 26);
        remove->setToolTip(QStringLiteral("Hapus kebiasaan"));
        layout->addWidget(remove);

        const std::int64_t habit_id = habit.id;
        connect(check, &QCheckBox::toggled, this, [this, habit_id](bool checked) { toggle_today(habit_id, checked); });
        connect(remove, &QPushButton::clicked, this, [this, habit_id] { remove_habit(habit_id); });

        body_->addWidget(row);
    }

    body_->addStretch(1);
}

void HabitsPage::toggle_today(std::int64_t id, bool done)
{
    const std::string today = QDate::currentDate().toString(Qt::ISODate).toStdString();
    if (Error error = habits_.set_done(id, today, done)) {
        report(error);
        return;
    }
    QTimer::singleShot(0, this, [this] { reload(); });
}

void HabitsPage::remove_habit(std::int64_t id)
{
    if (Error error = habits_.remove(id)) {
        report(error);
        return;
    }
    QTimer::singleShot(0, this, [this] { reload(); });
}

void HabitsPage::add_habit()
{
    TextInputDialog dialog{QStringLiteral("Kebiasaan Baru"), QStringLiteral("Nama kebiasaan"), {}, this};
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    Habit habit;
    habit.name = dialog.text().toStdString();

    auto created = habits_.create(habit);
    if (!created.ok()) {
        report(created.error());
        return;
    }
    reload();
}

void HabitsPage::report(const Error& error)
{
    QMessageBox::warning(this, QStringLiteral("DayFlow"), QString::fromStdString(error.message));
}

}  // namespace dayflow
