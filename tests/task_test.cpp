#include <catch2/catch_test_macros.hpp>

#include <string>

#include "core/task.hpp"

using namespace dayflow;

namespace {

Task valid_task()
{
    Task task;
    task.title = "belajar sqlite";
    return task;
}

}

TEST_CASE("task: task normal lolos validasi")
{
    CHECK(validate(valid_task()).code == ErrorCode::none);
}

TEST_CASE("task: judul kosong atau hanya spasi ditolak")
{
    Task task = valid_task();
    task.title = "";
    CHECK(validate(task).code == ErrorCode::invalid_argument);

    task.title = "   ";
    CHECK(validate(task).code == ErrorCode::invalid_argument);
}

TEST_CASE("task: batas panjang judul")
{
    Task task = valid_task();
    task.title = std::string(k_max_title_length, 'a');
    CHECK(validate(task).code == ErrorCode::none);

    task.title = std::string(k_max_title_length + 1, 'a');
    CHECK(validate(task).code == ErrorCode::invalid_argument);
}

TEST_CASE("task: completed_at harus konsisten dengan status")
{
    Task task = valid_task();
    task.completed_at = now_utc();
    CHECK(validate(task).code == ErrorCode::invalid_argument);

    task.status = TaskStatus::done;
    CHECK(validate(task).code == ErrorCode::none);

    task.completed_at.reset();
    CHECK(validate(task).code == ErrorCode::invalid_argument);
}

TEST_CASE("task: is_finished hanya untuk done dan cancelled")
{
    CHECK(is_finished(TaskStatus::todo) == false);
    CHECK(is_finished(TaskStatus::doing) == false);
    CHECK(is_finished(TaskStatus::done) == true);
    CHECK(is_finished(TaskStatus::cancelled) == true);
}

TEST_CASE("Priority to_string")
{
    REQUIRE(to_string(Priority::low) == "Low");
    REQUIRE(to_string(Priority::normal) == "Normal");
    REQUIRE(to_string(Priority::high) == "High");
    REQUIRE(to_string(Priority::urgent) == "Urgent");
}

TEST_CASE("TaskStatus to_string")
{
    REQUIRE(to_string(TaskStatus::todo) == "To Do");
    REQUIRE(to_string(TaskStatus::doing) == "Doing");
    REQUIRE(to_string(TaskStatus::done) == "Done");
    REQUIRE(to_string(TaskStatus::cancelled) == "Cancelled");
}