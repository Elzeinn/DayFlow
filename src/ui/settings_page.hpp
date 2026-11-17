#pragma once

#include <QWidget>

#include <filesystem>

class QLabel;
class QVBoxLayout;

namespace dayflow {

class Database;

class SettingsPage : public QWidget {
    Q_OBJECT

public:
    SettingsPage(Database& database, std::filesystem::path database_path, QWidget* parent = nullptr);

private:
    QWidget* make_card(const QString& title, QVBoxLayout** body);
    void backup();
    void refresh_size();
    void report(const std::string& message);

    Database& database_;
    std::filesystem::path path_;
    QLabel* size_label_ = nullptr;
};

}  // namespace dayflow
