#pragma once

#include <filesystem>

namespace dayflow {

class Database;
class EventCategoryRepository;
class EventRepository;
class HabitRepository;
class NoteRepository;
class RoutineRepository;
class TaskRepository;

struct AppContext {
    TaskRepository& tasks;
    EventRepository& events;
    EventCategoryRepository& categories;
    RoutineRepository& routines;
    HabitRepository& habits;
    NoteRepository& notes;
    Database& database;
    std::filesystem::path database_path;
};

}  // namespace dayflow
