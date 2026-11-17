#include <QApplication>
#include <QFont>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStyleFactory>

#include <filesystem>

#include "data/database.hpp"
#include "data/event_repository.hpp"
#include "data/note_repository.hpp"
#include "data/routine_repository.hpp"
#include "data/task_repository.hpp"
#include "ui/app_context.hpp"
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
    dayflow::EventRepository events{*database.value()};
    dayflow::EventCategoryRepository categories{*database.value()};
    dayflow::RoutineRepository routines{*database.value()};
    dayflow::HabitRepository habits{*database.value()};
    dayflow::NoteRepository notes{*database.value()};

    if (dayflow::Error error = categories.ensure_defaults()) {
        QMessageBox::warning(nullptr, QStringLiteral("DayFlow"), QString::fromStdString(error.message));
    }

    const dayflow::AppContext context{
        tasks, events, categories, routines, habits, notes, *database.value(), database_path};

    dayflow::MainWindow window{context};
    window.show();

    return app.exec();
}
