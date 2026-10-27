cmake_minimum_required(VERSION 4.2)
project(stapikplanner VERSION 1.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_NAME_DISPLAY "Stapik Planner")
set(PROJECT_AUTHOR "Sebastian Smoliński")
set(PROJECT_GITHUB_URL "https://github.com/Stapik-Group/stapik-planner")
set(PROJECT_APPLICATION_ID "pl.stapik.planner")

option(STAPIK_PLANNER_BUILD_TESTS "Build the Stapik Planner unit tests" OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)

find_package(CURL REQUIRED)

include(FetchContent)
FetchContent_Declare(
        stapikcommon
        GIT_REPOSITORY https://github.com/Stapik-Group/stapik-common.git
        GIT_TAG v1.6.0
)
FetchContent_MakeAvailable(stapikcommon)

add_library(stapikplannercore STATIC
        src/core/model/ActivityName.cpp
        src/core/model/PlannerDocument.cpp
        src/core/model/PlannerModel.cpp
        src/infrastructure/storage/PlannerStorage.cpp
)

target_include_directories(stapikplannercore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(stapikplannercore PUBLIC stapikcommon PkgConfig::GTKMM CURL::libcurl nlohmann_json::nlohmann_json)
stapik_enable_warnings(stapikplannercore)

# The Debian package: the vendor and the contact are read by CPack when the helper below includes it.
set(CPACK_PACKAGE_VENDOR "${PROJECT_AUTHOR}")
set(CPACK_PACKAGE_CONTACT "sebastian.smolinski6@gmail.com")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

stapik_add_application(
        NAME stapikplanner
        DISPLAY_NAME "${PROJECT_NAME_DISPLAY}"
        DESCRIPTION "A retro-styled desktop planner application"
        APPLICATION_ID "${PROJECT_APPLICATION_ID}"
        MAINTAINER "${PROJECT_AUTHOR} <${CPACK_PACKAGE_CONTACT}>"
        HOMEPAGE "${PROJECT_GITHUB_URL}"
        SECTION utils
        DESKTOP_CATEGORIES Office Planner
        RESOURCES_DIRECTORY resources
        PACKAGING_DIRECTORY packaging
        SOURCES
                src/app/PlannerAppInfo.cpp
                src/ui/action/MenuActionHandler.cpp
                src/ui/action/PlanActionHandler.cpp
                src/ui/dialog/ActivityDialog.cpp
                src/ui/view/ActivityCatalogView.cpp
                src/ui/view/ScheduleView.cpp
                src/ui/widget/catalog/ActivityCatalogItemRow.cpp
                src/ui/widget/schedule/ScheduleDayColumn.cpp
                src/ui/widget/schedule/ScheduleDaySlotContainer.cpp
                src/ui/widget/schedule/ScheduleGrid.cpp
                src/ui/widget/schedule/ScheduleDayLoadBar.cpp
                src/ui/widget/schedule/ScheduleHeader.cpp
                src/ui/widget/MainMenu.cpp
                src/ui/window/MainWindow.cpp
                src/main.cpp
        LINK_LIBRARIES
                stapikplannercore
                PkgConfig::GTKMM
                CURL::libcurl
)

# The name, the display name and the version are defined by the helper.
target_compile_definitions(stapikplanner PRIVATE
        STAPIK_APP_APPLICATION_ID="${PROJECT_APPLICATION_ID}"
        STAPIK_APP_AUTHOR="${PROJECT_AUTHOR}"
        STAPIK_APP_REPOSITORY_URL="${PROJECT_GITHUB_URL}"
)

if(STAPIK_PLANNER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
