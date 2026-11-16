#include <catch2/catch_test_macros.hpp>

#include <memory>

#include "data/database.hpp"
#include "data/settings_repository.hpp"

using namespace dayflow;

namespace {

struct Repo {
    std::unique_ptr<Database> database;
    std::unique_ptr<SettingsRepository> settings;
};

Repo make_repo()
{
    Repo repo;
    auto opened = Database::open_in_memory();
    REQUIRE(opened.ok());
    repo.database = std::move(opened.value());
    repo.settings = std::make_unique<SettingsRepository>(*repo.database);
    return repo;
}

}

TEST_CASE("settings: key yang belum ada mengembalikan nullopt")
{
    Repo repo = make_repo();

    auto value = repo.settings->get("tema");
    REQUIRE(value.ok());
    CHECK_FALSE(value.value().has_value());
}

TEST_CASE("settings: set lalu get")
{
    Repo repo = make_repo();

    CHECK(repo.settings->set("tema", "gelap").code == ErrorCode::none);

    auto value = repo.settings->get("tema");
    REQUIRE(value.ok());
    REQUIRE(value.value().has_value());
    CHECK(*value.value() == "gelap");
}

TEST_CASE("settings: set menimpa nilai lama")
{
    Repo repo = make_repo();

    CHECK(repo.settings->set("tema", "gelap").code == ErrorCode::none);
    CHECK(repo.settings->set("tema", "terang").code == ErrorCode::none);

    auto value = repo.settings->get("tema");
    REQUIRE(value.ok());
    REQUIRE(value.value().has_value());
    CHECK(*value.value() == "terang");
}

TEST_CASE("settings: remove menghapus key")
{
    Repo repo = make_repo();

    CHECK(repo.settings->set("hotkey", "Ctrl+Alt+D").code == ErrorCode::none);
    CHECK(repo.settings->remove("hotkey").code == ErrorCode::none);

    auto value = repo.settings->get("hotkey");
    REQUIRE(value.ok());
    CHECK_FALSE(value.value().has_value());
}
