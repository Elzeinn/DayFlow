#include <QApplication>
#include <QFont>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStyleFactory>

#include <filesystem>

#include "data/database.hpp"
#include "data/task_repository.hpp"
#include "ui/main_window.hpp"
#include "ui/theme.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("DayFlow"));

    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    app.setFont(QFont{QStringLiteral("Segoe UI"), 10});
    app.setStyleSheet(dayflow::theme::style_sheet());

    const QString data_directory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const std::filesystem::path database_path =
        std::filesystem::path{data_directory.toStdWString()} / "dayflow.db";

    auto database = dayflow::Database::open(database_path);
    if (!database.ok()) {
        QMessageBox::critical(nullptr, QStringLiteral("DayFlow"),
            QString::fromStdString(database.error().message));
        return 1;
    }

    dayflow::TaskRepository tasks{*database.value()};
    dayflow::MainWindow window{tasks};
    window.show();

    return app.exec();
}
