include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
        FIND_PACKAGE_ARGS NAMES GTest
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(stapikplanner_tests
        core/ActivityNameTest.cpp
        core/DragPayloadTest.cpp
        core/PlannerDocumentTest.cpp
        core/PlannerModelTest.cpp
        core/PlannerModelCloudTest.cpp
        infrastructure/PlannerStorageTest.cpp
        infrastructure/PlannerSyncCompatibilityTest.cpp
        resources/PlannerResourcesTest.cpp
)

target_link_libraries(stapikplanner_tests PRIVATE
        stapikplannercore
        GTest::gtest_main
)

# The fake cloud storage and the environment helper are shared with the stapik-common tests.
target_include_directories(stapikplanner_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${stapikcommon_SOURCE_DIR}/tests
)

target_compile_definitions(stapikplanner_tests PRIVATE
        STAPIK_PLANNER_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
        STAPIK_COMMON_RESOURCES_DIR="${STAPIK_COMMON_RESOURCES_DIR}"
)

stapik_enable_warnings(stapikplanner_tests)

gtest_discover_tests(stapikplanner_tests)
