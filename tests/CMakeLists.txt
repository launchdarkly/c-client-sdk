cmake_minimum_required(VERSION 3.10)
include(GoogleTest)

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/src")

file(GLOB tests "${PROJECT_SOURCE_DIR}/tests/*.cpp")
list(REMOVE_ITEM tests "${PROJECT_SOURCE_DIR}/tests/test-main.cpp")
list(REMOVE_ITEM tests "${PROJECT_SOURCE_DIR}/tests/commonfixture.cpp")

add_executable("google_tests"
               ${sources}
               ${tests}
               "${PROJECT_SOURCE_DIR}/tests/commonfixture.cpp"
               "${PROJECT_SOURCE_DIR}/tests/test-main.cpp")
target_link_libraries(google_tests ldsharedapi test-utils)
target_include_directories(google_tests PRIVATE ${LD_INCLUDE_PATHS})
target_link_libraries("google_tests" gtest_main)

gtest_discover_tests(google_tests)
target_compile_definitions(google_tests
        PRIVATE -D LAUNCHDARKLY_USE_ASSERT
                -D LAUNCHDARKLY_CONCURRENCY_ABORT
        )
