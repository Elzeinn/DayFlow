cmake_minimum_required(VERSION 3.25)

project(DayFlow
    VERSION 0.1.0
    DESCRIPTION "Windows personal planner: tasks, calendar, routines, habits"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DAYFLOW_BUILD_TESTS "Build unit tests (Catch2 diunduh saat configure)" ON)
option(DAYFLOW_WITH_SQLITE "Aktifkan SQLite untuk lapisan data" ON)

# ---------------------------------------------------------------------------
# Flag peringatan yang dipakai semua target
# ---------------------------------------------------------------------------
add_library(dayflow_warnings INTERFACE)
if(MSVC)
    target_compile_options(dayflow_warnings INTERFACE
        /W4 /permissive- /utf-8 /Zc:__cplusplus)
endif()
if(WIN32)
    target_compile_definitions(dayflow_warnings INTERFACE
        WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
endif()

# ---------------------------------------------------------------------------
# Qt dicari lebih dulu supaya AUTOMOC/AUTOUIC berlaku untuk semua modul
# ---------------------------------------------------------------------------
find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup(REQUIRES 6.5)

# ---------------------------------------------------------------------------
# Membuat target library dari isi src/<nama>.
# Folder yang masih kosong dilewati, jadi project tetap bisa di-configure
# sambil modul diisi satu per satu.
# ---------------------------------------------------------------------------
function(dayflow_add_module name)
    set(module_dir "${CMAKE_CURRENT_SOURCE_DIR}/src/${name}")
    file(GLOB_RECURSE module_sources CONFIGURE_DEPENDS
        "${module_dir}/*.cpp")
    file(GLOB_RECURSE module_headers CONFIGURE_DEPENDS
        "${module_dir}/*.h" "${module_dir}/*.hpp")

    if(NOT module_sources)
        message(STATUS "DayFlow: src/${name} belum punya file .cpp -> target dayflow_${name} dilewati")
        return()
    endif()

    add_library(dayflow_${name} STATIC ${module_sources} ${module_headers})
    target_include_directories(dayflow_${name} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(dayflow_${name} PUBLIC dayflow_warnings)
    set_target_properties(dayflow_${name} PROPERTIES FOLDER "modules")
endfunction()

dayflow_add_module(core)
dayflow_add_module(data)
dayflow_add_module(platform)
dayflow_add_module(ui)

set(DAYFLOW_MODULES "")
foreach(module core data platform ui)
    if(TARGET dayflow_${module})
        list(APPEND DAYFLOW_MODULES dayflow_${module})
    endif()
endforeach()

# ---------------------------------------------------------------------------
# SQLite: vcpkg (find_package) atau amalgamation di third_party/sqlite/
# ---------------------------------------------------------------------------
if(DAYFLOW_WITH_SQLITE)
    find_package(SQLite3 QUIET)

    if(NOT SQLite3_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite/sqlite3.c")
        add_library(sqlite3_vendored STATIC third_party/sqlite/sqlite3.c)
        target_include_directories(sqlite3_vendored PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite")
        target_compile_definitions(sqlite3_vendored PRIVATE
            SQLITE_THREADSAFE=1
            SQLITE_ENABLE_FTS5
            SQLITE_OMIT_LOAD_EXTENSION
            SQLITE_DQS=0)
        add_library(SQLite::SQLite3 ALIAS sqlite3_vendored)
        set(SQLite3_FOUND TRUE)
        message(STATUS "DayFlow: memakai SQLite amalgamation dari third_party/sqlite")
    endif()

    if(SQLite3_FOUND)
        message(STATUS "DayFlow: SQLite siap dipakai")
    else()
        message(WARNING
            "DayFlow: SQLite3 tidak ditemukan. Pilih salah satu: pasang lewat vcpkg, "
            "taruh sqlite3.c + sqlite3.h di third_party/sqlite/, atau configure dengan "
            "-DDAYFLOW_WITH_SQLITE=OFF (lapisan data belum bisa dipakai).")
    endif()
endif()

# ---------------------------------------------------------------------------
# Dependency antar modul (satu arah, tanpa siklus)
#   core     <- data, platform, ui
#   data     <- ui
#   platform <- ui
# ---------------------------------------------------------------------------
foreach(module data platform ui)
    if(TARGET dayflow_${module} AND TARGET dayflow_core)
        target_link_libraries(dayflow_${module} PUBLIC dayflow_core)
    endif()
endforeach()

if(TARGET dayflow_ui AND TARGET dayflow_data)
    target_link_libraries(dayflow_ui PUBLIC dayflow_data)
endif()

if(TARGET dayflow_ui)
    target_link_libraries(dayflow_ui PUBLIC Qt6::Widgets)
    qt_add_resources(dayflow_ui "dayflow_ui_resources"
        PREFIX "/dayflow"
        BASE "src/ui"
        FILES src/ui/theme.qss)
endif()

if(TARGET dayflow_ui AND TARGET dayflow_platform)
    target_link_libraries(dayflow_ui PUBLIC dayflow_platform)
endif()

if(DAYFLOW_WITH_SQLITE AND SQLite3_FOUND AND TARGET dayflow_data)
    target_link_libraries(dayflow_data PUBLIC SQLite::SQLite3)
    target_compile_definitions(dayflow_data PUBLIC DAYFLOW_HAVE_SQLITE=1)
endif()

# ---------------------------------------------------------------------------
# Aplikasi
# ---------------------------------------------------------------------------
qt_add_executable(dayflow WIN32 src/app/main.cpp)
target_link_libraries(dayflow PRIVATE Qt6::Widgets ${DAYFLOW_MODULES})
set_target_properties(dayflow PROPERTIES
    WIN32_EXECUTABLE TRUE
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:dayflow>")

# ---------------------------------------------------------------------------
# Test: hanya core + data, supaya tidak butuh QApplication
# ---------------------------------------------------------------------------
if(DAYFLOW_BUILD_TESTS)
    enable_testing()

    file(GLOB_RECURSE dayflow_test_sources CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")

    if(NOT dayflow_test_sources)
        message(STATUS "DayFlow: tests/ masih kosong -> target dayflow_tests dilewati")
    else()
        include(FetchContent)
        FetchContent_Declare(Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.7.1
            GIT_SHALLOW TRUE
            SYSTEM)
        FetchContent_MakeAvailable(Catch2)

        set(DAYFLOW_TESTABLE_MODULES "")
        foreach(module core data)
            if(TARGET dayflow_${module})
                list(APPEND DAYFLOW_TESTABLE_MODULES dayflow_${module})
            endif()
        endforeach()

        add_executable(dayflow_tests ${dayflow_test_sources})
        target_link_libraries(dayflow_tests PRIVATE
            Catch2::Catch2WithMain
            dayflow_warnings
            ${DAYFLOW_TESTABLE_MODULES})
        add_test(NAME dayflow_tests COMMAND dayflow_tests)
    endif()
endif()

# ---------------------------------------------------------------------------
# Ringkasan configure
# ---------------------------------------------------------------------------
message(STATUS "DayFlow ${PROJECT_VERSION}")
message(STATUS "  compiler   : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Qt         : ${Qt6_VERSION}")
message(STATUS "  modul aktif: ${DAYFLOW_MODULES}")
message(STATUS "  test       : ${DAYFLOW_BUILD_TESTS}")
