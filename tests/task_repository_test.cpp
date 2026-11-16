#include <catch2/catch_test_macros.hpp>

#include <memory>

#include "core/task.hpp"
#include "data/database.hpp"
#include "data/task_repository.hpp"

using namespace dayflow;

namespace {

struct Repo {
    std::unique_ptr<Database> database;
    std::unique_ptr<TaskRepository> tasks;
};

Repo make_repo()
{
    Repo repo;
    auto opened = Database::open_in_memory();
    REQUIRE(opened.ok());
    repo.database = std::move(opened.value());
    repo.tasks = std::make_unique<TaskRepository>(*repo.database);
    return repo;
}

}

TEST_CASE("task repository: create mengisi id dan timestamp")
{
    Repo repo = make_repo();

    Task task;
    task.title = "belajar sqlite";

    auto created = repo.tasks->create(task);
    REQUIRE(created.ok());
    CHECK(created.value().id > 0);
    CHECK(created.value().created_at != Timestamp{});
    CHECK(created.value().updated_at != Timestamp{});

    auto found = repo.tasks->find(created.value().id);
    REQUIRE(found.ok());
    REQUIRE(found.value().has_value());
    CHECK(found.value()->title == "belajar sqlite");
    CHECK(found.value()->priority == Priority::normal);
    CHECK(found.value()->status == TaskStatus::todo);
    CHECK_FALSE(found.value()->due_at.has_value());
    CHECK_FALSE(found.value()->completed_at.has_value());
}

TEST_CASE("task repository: create menolak data tidak valid")
{
    Repo repo = make_repo();

    Task task;
    task.title = "   ";

    auto created = repo.tasks->create(task);
    CHECK_FALSE(created.ok());
    CHECK(created.error().code == ErrorCode::invalid_argument);
}

TEST_CASE("task repository: find yang tidak ada mengembalikan nullopt")
{
    Repo repo = make_repo();

    auto found = repo.tasks->find(4242);
    REQUIRE(found.ok());
    CHECK_FALSE(found.value().has_value());
}

TEST_CASE("task repository: list_all mengikuti sort_order lalu id")
{
    Repo repo = make_repo();

    Task pertama;
    pertama.title = "pertama";
    pertama.sort_order = 2;

    Task kedua;
    kedua.title = "kedua";
    kedua.sort_order = 1;

    Task ketiga;
    ketiga.title = "ketiga";
    ketiga.sort_order = 1;

    REQUIRE(repo.tasks->create(pertama).ok());
    REQUIRE(repo.tasks->create(kedua).ok());
    REQUIRE(repo.tasks->create(ketiga).ok());

    auto listed = repo.tasks->list_all();
    REQUIRE(listed.ok());
    REQUIRE(listed.value().size() == 3);
    CHECK(listed.value()[0].title == "kedua");
    CHECK(listed.value()[1].title == "ketiga");
    CHECK(listed.value()[2].title == "pertama");
}

TEST_CASE("task repository: update menyimpan perubahan")
{
    Repo repo = make_repo();

    Task task;
    task.title = "judul lama";

    auto created = repo.tasks->create(task);
    REQUIRE(created.ok());

    Task changed = created.value();
    changed.title = "judul baru";
    changed.priority = Priority::urgent;
    changed.notes = "catatan";

    CHECK(repo.tasks->update(changed).code == ErrorCode::none);

    auto found = repo.tasks->find(changed.id);
    REQUIRE(found.ok());
    REQUIRE(found.value().has_value());
    CHECK(found.value()->title == "judul baru");
    CHECK(found.value()->priority == Priority::urgent);
    CHECK(found.value()->notes == "catatan");
}

TEST_CASE("task repository: update id yang tidak ada dilaporkan not_found")
{
    Repo repo = make_repo();

    Task task;
    task.id = 999;
    task.title = "hantu";

    CHECK(repo.tasks->update(task).code == ErrorCode::not_found);
}

TEST_CASE("task repository: due date tersimpan dan terbaca kembali")
{
    Repo repo = make_repo();

    auto due = parse_iso8601("2026-12-01T09:00:00Z");
    REQUIRE(due.ok());

    Task task;
    task.title = "rapat";
    task.due_at = due.value();

    auto created = repo.tasks->create(task);
    REQUIRE(created.ok());

    auto found = repo.tasks->find(created.value().id);
    REQUIRE(found.ok());
    REQUIRE(found.value().has_value());
    REQUIRE(found.value()->due_at.has_value());
    CHECK(to_iso8601(*found.value()->due_at) == "2026-12-01T09:00:00Z");
}

TEST_CASE("task repository: set_status mengatur completed_at")
{
    Repo repo = make_repo();

    Task task;
    task.title = "selesaikan";

    auto created = repo.tasks->create(task);
    REQUIRE(created.ok());

    CHECK(repo.tasks->set_status(created.value().id, TaskStatus::done).code == ErrorCode::none);

    auto done = repo.tasks->find(created.value().id);
    REQUIRE(done.ok());
    REQUIRE(done.value().has_value());
    CHECK(done.value()->status == TaskStatus::done);
    CHECK(done.value()->completed_at.has_value());

    CHECK(repo.tasks->set_status(created.value().id, TaskStatus::todo).code == ErrorCode::none);

    auto reopened = repo.tasks->find(created.value().id);
    REQUIRE(reopened.ok());
    REQUIRE(reopened.value().has_value());
    CHECK(reopened.value()->status == TaskStatus::todo);
    CHECK_FALSE(reopened.value()->completed_at.has_value());
}

TEST_CASE("task repository: list_by_status memfilter")
{
    Repo repo = make_repo();

    Task todo;
    todo.title = "belum";
    Task done;
    done.title = "sudah";
    done.status = TaskStatus::done;
    done.completed_at = now_utc();

    REQUIRE(repo.tasks->create(todo).ok());
    REQUIRE(repo.tasks->create(done).ok());

    auto pending = repo.tasks->list_by_status(TaskStatus::todo);
    REQUIRE(pending.ok());
    REQUIRE(pending.value().size() == 1);
    CHECK(pending.value()[0].title == "belum");

    auto finished = repo.tasks->list_by_status(TaskStatus::done);
    REQUIRE(finished.ok());
    REQUIRE(finished.value().size() == 1);
    CHECK(finished.value()[0].title == "sudah");
}

TEST_CASE("task repository: remove menghapus permanen")
{
    Repo repo = make_repo();

    Task task;
    task.title = "sementara";

    auto created = repo.tasks->create(task);
    REQUIRE(created.ok());

    CHECK(repo.tasks->remove(created.value().id).code == ErrorCode::none);

    auto found = repo.tasks->find(created.value().id);
    REQUIRE(found.ok());
    CHECK_FALSE(found.value().has_value());

    CHECK(repo.tasks->remove(created.value().id).code == ErrorCode::not_found);
}
