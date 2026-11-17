#include "ui/settings_page.hpp"

#include <QDate>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

#include "data/database.hpp"
#include "ui/theme.hpp"

namespace dayflow {
namespace {

QString human_size(std::uintmax_t bytes)
{
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }
    if (bytes < 1024 * 1024) {
        return QStringLiteral("%1 KB").arg(bytes / 1024);
    }
    return QStringLiteral("%1 MB").arg(QString::number(static_cast<double>(bytes) / (1024.0 * 1024.0), 'f', 1));
}

QLabel* row_label(const QString& text, const QString& objectName = QStringLiteral("settingValue"))
{
    auto* label = new QLabel{text};
    label->setObjectName(objectName);
    label->setWordWrap(true);
    return label;
}

}  // namespace

SettingsPage::SettingsPage(Database& database, std::filesystem::path database_path, QWidget* parent)
    : QWidget(parent), database_(database), path_(std::move(database_path))
{
    auto* layout = new QVBoxLayout{this};
    layout->setContentsMargins(28, 22, 28, 22);
    layout->setSpacing(18);

    auto* header = new QVBoxLayout{};
    header->setSpacing(6);
    header->addWidget(theme::make_micro_label(QStringLiteral("PREFERENSI")));
    auto* title = new QLabel{QStringLiteral("Pengaturan")};
    title->setObjectName(QStringLiteral("displayTitle"));
    title->setFont(theme::display_font());
    title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    title->setFixedHeight(40);
    header->addWidget(title);
    layout->addLayout(header);

    QVBoxLayout* data_body = nullptr;
    auto* data_card = make_card(QStringLiteral("DATA"), &data_body);
    data_body->addWidget(theme::make_micro_label(QStringLiteral("LOKASI DATABASE")));
    data_body->addWidget(row_label(QString::fromStdWString(path_.wstring())));

    auto* size_row = new QHBoxLayout{};
    size_row->addWidget(theme::make_micro_label(QStringLiteral("UKURAN")));
    size_label_ = row_label(QString{});
    size_row->addWidget(size_label_, 1);
    data_body->addLayout(size_row);

    auto* backup_button = new QPushButton{QStringLiteral("BACKUP SEKARANG")};
    backup_button->setObjectName(QStringLiteral("primary"));
    backup_button->setCursor(Qt::PointingHandCursor);
    auto* backup_row = new QHBoxLayout{};
    backup_row->addWidget(backup_button);
    backup_row->addStretch(1);
    data_body->addSpacing(6);
    data_body->addLayout(backup_row);
    data_body->addStretch(1);
    layout->addWidget(data_card, 1);

    QVBoxLayout* about_body = nullptr;
    auto* about_card = make_card(QStringLiteral("TENTANG"), &about_body);
    about_body->addWidget(row_label(QStringLiteral("DayFlow 0.1.0")));
    about_body->addWidget(row_label(QStringLiteral("Skema database v1 · data lokal, tanpa server")));
    about_body->addStretch(1);
    layout->addWidget(about_card);

    QVBoxLayout* pending_body = nullptr;
    auto* pending_card = make_card(QStringLiteral("BELUM TERSEDIA"), &pending_body);
    pending_body->addWidget(row_label(QStringLiteral("Tema terang · autostart · notifikasi toast · global hotkey")));
    pending_body->addWidget(
        row_label(QStringLiteral("Bagian platform Windows dikerjakan terpisah; pengaturan lain menyusul."),
            QStringLiteral("settingNote")));
    pending_body->addStretch(1);
    layout->addWidget(pending_card);

    connect(backup_button, &QPushButton::clicked, this, &SettingsPage::backup);
    refresh_size();
}

QWidget* SettingsPage::make_card(const QString& title, QVBoxLayout** body)
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

void SettingsPage::refresh_size()
{
    std::error_code code;
    const auto bytes = std::filesystem::file_size(path_, code);
    size_label_->setText(code ? QStringLiteral("tidak terbaca") : human_size(bytes));
}

void SettingsPage::backup()
{
    const QString default_name = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        + QStringLiteral("/dayflow-backup-%1.db").arg(QDate::currentDate().toString(QStringLiteral("yyyyMMdd")));

    const QString target = QFileDialog::getSaveFileName(
        this, QStringLiteral("Simpan backup"), default_name, QStringLiteral("Database (*.db)"));
    if (target.isEmpty()) {
        return;
    }

    if (Error error = database_.backup_to(std::filesystem::path{target.toStdWString()})) {
        report(error.message);
        return;
    }

    QMessageBox::information(this, QStringLiteral("DayFlow"),
        QStringLiteral("Backup tersimpan di:\n%1").arg(target));
    refresh_size();
}

void SettingsPage::report(const std::string& message)
{
    QMessageBox::warning(this, QStringLiteral("DayFlow"), QString::fromStdString(message));
}

}  // namespace dayflow
