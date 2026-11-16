#include "ui/date_field.hpp"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMouseEvent>
#include <QPushButton>
#include <QTimeEdit>
#include <QVBoxLayout>

#include <functional>

namespace dayflow {
namespace {

constexpr int k_weekdays = 7;
constexpr int k_cells = 42;

class DatePopup : public QFrame {
public:
    DatePopup(const QDateTime& value, bool allow_empty)
        : QFrame{nullptr, Qt::Popup | Qt::FramelessWindowHint}, value_(value), allow_empty_(allow_empty)
    {
        setObjectName(QStringLiteral("datePopup"));
        view_month_ = QDate{value.date().year(), value.date().month(), 1};
        build();
    }

    std::function<void(const QDateTime&)> on_pick;
    std::function<void()> on_clear;

private:
    QPushButton* small_button(const QString& text)
    {
        auto* button = new QPushButton{text, this};
        button->setObjectName(QStringLiteral("iconButton"));
        button->setFixedSize(26, 26);
        button->setCursor(Qt::PointingHandCursor);
        return button;
    }

    void build()
    {
        auto* outer = new QVBoxLayout{this};
        outer->setContentsMargins(12, 12, 12, 12);
        outer->setSpacing(8);

        auto* header = new QHBoxLayout{};
        header->setSpacing(6);
        auto* previous = small_button(QStringLiteral("\u2039"));
        month_label_ = new QLabel{this};
        month_label_->setObjectName(QStringLiteral("datePopupMonth"));
        month_label_->setAlignment(Qt::AlignCenter);
        auto* next = small_button(QStringLiteral("\u203A"));
        header->addWidget(previous);
        header->addWidget(month_label_, 1);
        header->addWidget(next);
        outer->addLayout(header);

        auto* weekdays = new QHBoxLayout{};
        weekdays->setSpacing(2);
        const char* names[] = {"S", "S", "R", "K", "J", "S", "M"};
        for (int index = 0; index < k_weekdays; ++index) {
            auto* label = new QLabel{QString::fromLatin1(names[index]), this};
            label->setObjectName(QStringLiteral("datePopupWeekday"));
            label->setAlignment(Qt::AlignCenter);
            weekdays->addWidget(label, 1);
        }
        outer->addLayout(weekdays);

        grid_ = new QGridLayout{};
        grid_->setSpacing(2);
        outer->addLayout(grid_);

        time_ = new QTimeEdit{value_.time(), this};
        time_->setObjectName(QStringLiteral("field"));
        time_->setDisplayFormat(QStringLiteral("HH:mm"));
        outer->addWidget(time_);

        if (allow_empty_) {
            auto* clear = new QPushButton{QStringLiteral("Kosongkan"), this};
            clear->setObjectName(QStringLiteral("secondary"));
            connect(clear, &QPushButton::clicked, this, [this] {
                if (on_clear) {
                    on_clear();
                }
                close();
            });
            outer->addWidget(clear);
        }

        connect(previous, &QPushButton::clicked, this, [this] { shift_month(-1); });
        connect(next, &QPushButton::clicked, this, [this] { shift_month(1); });
        connect(time_, &QTimeEdit::timeChanged, this, [this](const QTime& time) {
            value_.setTime(time);
            if (on_pick) {
                on_pick(value_);
            }
        });

        rebuild();
    }

    void shift_month(int delta)
    {
        view_month_ = view_month_.addMonths(delta);
        rebuild();
    }

    void rebuild()
    {
        month_label_->setText(
            QLocale{QLocale::Indonesian, QLocale::Indonesia}.toString(view_month_, QStringLiteral("MMMM yyyy")));

        while (QLayoutItem* item = grid_->takeAt(0)) {
            delete item->widget();
            delete item;
        }

        const QDate first{view_month_.year(), view_month_.month(), 1};
        const QDate start = first.addDays(-(first.dayOfWeek() - 1));

        for (int index = 0; index < k_cells; ++index) {
            const QDate date = start.addDays(index);
            auto* button = new QPushButton{QString::number(date.day()), this};
            button->setObjectName(QStringLiteral("dateCell"));
            button->setCheckable(true);
            button->setChecked(date == value_.date());
            button->setProperty("outside", date.month() != view_month_.month());
            button->setCursor(Qt::PointingHandCursor);
            button->setFixedSize(30, 28);
            connect(button, &QPushButton::clicked, this, [this, date] {
                value_.setDate(date);
                if (on_pick) {
                    on_pick(value_);
                }
                close();
            });
            grid_->addWidget(button, index / k_weekdays, index % k_weekdays);
        }
    }

    QDate view_month_;
    QDateTime value_;
    bool allow_empty_ = true;
    QLabel* month_label_ = nullptr;
    QGridLayout* grid_ = nullptr;
    QTimeEdit* time_ = nullptr;
};

}  // namespace

DateField::DateField(QWidget* parent) : QFrame(parent)
{
    setObjectName(QStringLiteral("select"));
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(36);

    auto* layout = new QHBoxLayout{this};
    layout->setContentsMargins(12, 0, 10, 0);
    layout->setSpacing(8);

    value_label_ = new QLabel{this};
    value_label_->setObjectName(QStringLiteral("selectValue"));
    layout->addWidget(value_label_, 1);

    auto* chevron = new QLabel{QStringLiteral("\u25BE"), this};
    chevron->setObjectName(QStringLiteral("selectChevron"));
    layout->addWidget(chevron);

    refresh_label();
}

void DateField::set_allow_empty(bool allow)
{
    allow_empty_ = allow;
    if (!allow && empty_) {
        value_ = QDateTime{QDate::currentDate(), QTime{9, 0}};
        empty_ = false;
        refresh_label();
    }
}

void DateField::set_value(const QDateTime& value)
{
    value_ = value;
    empty_ = false;
    refresh_label();
}

void DateField::set_empty()
{
    if (!allow_empty_) {
        return;
    }
    empty_ = true;
    refresh_label();
}

void DateField::refresh_label()
{
    if (empty_) {
        value_label_->setText(empty_text_);
        return;
    }
    value_label_->setText(value_.toString(QStringLiteral("dd/MM/yyyy HH:mm")));
}

void DateField::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        open_popup();
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void DateField::open_popup()
{
    const QDateTime initial = empty_ ? QDateTime{QDate::currentDate(), QTime{9, 0}} : value_;
    auto* popup = new DatePopup{initial, allow_empty_};
    popup->setAttribute(Qt::WA_DeleteOnClose);

    popup->on_pick = [this](const QDateTime& picked) {
        value_ = picked;
        empty_ = false;
        refresh_label();
        emit changed();
    };
    popup->on_clear = [this] {
        empty_ = true;
        refresh_label();
        emit changed();
    };

    popup->adjustSize();
    popup->move(mapToGlobal(QPoint{0, height() + 4}));
    popup->show();
}

}  // namespace dayflow
