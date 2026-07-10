cmake_minimum_required(VERSION 4.2)
project(stapikplanner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_NAME_DISPLAY "Stapik Planner")
set(PROJECT_AUTHOR "Sebastian Smoliński")
set(PROJECT_GITHUB_URL "https://github.com/Stapik-Group/stapik-planner")

configure_file(
        ${CMAKE_SOURCE_DIR}/src/ui/dialog/Version.hpp.in
        ${CMAKE_BINARY_DIR}/generated/Version.hpp
        @ONLY
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)

find_package(CURL REQUIRED)

include(FetchContent)
FetchContent_Declare(
        stapikcommon
        GIT_REPOSITORY https://github.com/Stapik-Group/stapik-common.git
        GIT_TAG v0.4.0
)
FetchContent_MakeAvailable(stapikcommon)

add_executable(stapikplanner
        src/core/locale/LocaleManager.cpp
        src/core/model/PlannerModel.cpp
        src/infrastructure/storage/Storage.cpp
        src/infrastructure/storage/CloudStorageConfigStorage.cpp
        src/infrastructure/storage/PlannerSyncCoordinator.cpp
        src/ui/action/MenuActionHandler.cpp
        src/ui/style/AppStyleProvider.cpp
        src/ui/dialog/ActivityDialog.cpp
        src/ui/dialog/AboutDialog.cpp
        src/ui/dialog/ConnectDialog.cpp
        src/ui/dialog/DialogUtils.cpp
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
)

target_include_directories(stapikplanner PRIVATE ${GTKMM_INCLUDE_DIRS})
target_include_directories(stapikplanner PRIVATE ${CMAKE_BINARY_DIR}/generated)
target_compile_options(stapikplanner PRIVATE ${GTKMM_CFLAGS_OTHER})
target_link_libraries(stapikplanner PRIVATE ${GTKMM_LIBRARIES} stapikcommon CURL::libcurl nlohmann_json::nlohmann_json)

add_custom_command(TARGET stapikplanner POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:stapikplanner>/resources)

install(TARGETS stapikplanner
        RUNTIME DESTINATION lib/stapikplanner
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/
        DESTINATION lib/stapikplanner/resources
)

install(PROGRAMS ${CMAKE_SOURCE_DIR}/packaging/stapikplanner-launcher.sh
        DESTINATION bin
        RENAME stapikplanner
)

install(FILES ${CMAKE_SOURCE_DIR}/packaging/stapikplanner.desktop
        DESTINATION share/applications
)

install(FILES ${CMAKE_SOURCE_DIR}/packaging/icon.png
        DESTINATION share/icons/hicolor/256x256/apps
        RENAME stapikplanner.png
)

set(CPACK_PACKAGE_NAME "stapikplanner")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR ${PROJECT_AUTHOR})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A retro-styled desktop planner application")
set(CPACK_PACKAGE_CONTACT "sebastian.smolinski6@gmail.com")

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${PROJECT_AUTHOR} <sebastian.smolinski6@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

include(CPack)