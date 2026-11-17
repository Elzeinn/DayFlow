#include <catch2/catch_test_macros.hpp>

#include <memory>

#include "core/note.hpp"
#include "data/database.hpp"
#include "data/note_repository.hpp"

using namespace dayflow;

namespace {

std::unique_ptr<NoteRepository> make_repo(std::unique_ptr<Database>& database)
{
    auto opened = Database::open_in_memory();
    REQUIRE(opened.ok());
    database = std::move(opened.value());
    return std::make_unique<NoteRepository>(*database);
}

}

TEST_CASE("note: judul diambil dari baris pertama")
{
    CHECK(note_title("belanja bulanan") == "belanja bulanan");
    CHECK(note_title("  belanja  \n detail kedua") == "belanja");
    CHECK(note_title("   ") == "Catatan kosong");
    CHECK(note_title("") == "Catatan kosong");
}

TEST_CASE("note: judul panjang dipotong")
{
    const std::string body(100, 'a');
    const std::string title = note_title(body, 10);
    CHECK(title.size() == 13);
    CHECK(title.substr(title.size() - 3) == "...");
}

TEST_CASE("note: validasi body")
{
    Note note;
    CHECK(validate(note).code == ErrorCode::invalid_argument);

    note.body = "   \n ";
    CHECK(validate(note).code == ErrorCode::invalid_argument);

    note.body = "isi";
    CHECK(validate(note).code == ErrorCode::none);

    note.body = std::string(k_max_note_body + 1, 'x');
    CHECK(validate(note).code == ErrorCode::invalid_argument);
}

TEST_CASE("note repository: create, update, dan hapus")
{
    std::unique_ptr<Database> database;
    auto repo = make_repo(database);

    Note note;
    note.body = "ide aplikasi";
    auto created = repo->create(note);
    REQUIRE(created.ok());
    CHECK(created.value().id > 0);

    auto found = repo->find(created.value().id);
    REQUIRE(found.ok());
    REQUIRE(found.value().has_value());
    CHECK(found.value()->body == "ide aplikasi");
    CHECK_FALSE(found.value()->pinned);

    Note changed = *found.value();
    changed.body = "ide aplikasi v2";
    changed.pinned = true;
    CHECK(repo->update(changed).code == ErrorCode::none);

    auto reloaded = repo->find(created.value().id);
    REQUIRE(reloaded.ok());
    REQUIRE(reloaded.value().has_value());
    CHECK(reloaded.value()->body == "ide aplikasi v2");
    CHECK(reloaded.value()->pinned);

    CHECK(repo->remove(created.value().id).code == ErrorCode::none);
    auto gone = repo->find(created.value().id);
    REQUIRE(gone.ok());
    CHECK_FALSE(gone.value().has_value());
    CHECK(repo->remove(created.value().id).code == ErrorCode::not_found);
}

TEST_CASE("note repository: catatan disematkan tampil lebih dulu")
{
    std::unique_ptr<Database> database;
    auto repo = make_repo(database);

    Note biasa;
    biasa.body = "biasa";
    REQUIRE(repo->create(biasa).ok());

    Note penting;
    penting.body = "penting";
    REQUIRE(repo->create(penting).ok());

    Note disematkan;
    disematkan.body = "disematkan";
    disematkan.pinned = true;
    REQUIRE(repo->create(disematkan).ok());

    auto listed = repo->list_all();
    REQUIRE(listed.ok());
    REQUIRE(listed.value().size() == 3);
    CHECK(listed.value().front().body == "disematkan");
}
