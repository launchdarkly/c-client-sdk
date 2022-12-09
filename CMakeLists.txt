cmake_minimum_required(VERSION 3.10)

cmake_policy(SET CMP0048 NEW)

file(READ "include/launchdarkly/api.h" HeaderVersionFile)

string(REGEX MATCH
    "#define LD_SDK_VERSION \"([0-9\\.]+).*\""
    _
    ${HeaderVersionFile}
)

message("Extracted version: ${CMAKE_MATCH_1}")

project(ldclientapi VERSION ${CMAKE_MATCH_1})

option(BUILD_BENCHMARKS "Also build benchmarks" OFF)

# Contains various Find files, code coverage, 3rd party library FetchContent scripts,
# and the project's Package Configuration script.
set(CMAKE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/CMakeFiles")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_FILES}")

include(CTest)

if(BUILD_TESTING)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    configure_file(CMakeLists.txt.in
            ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )


    add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
            ${CMAKE_BINARY_DIR}/googletest-build EXCLUDE_FROM_ALL)

    enable_testing()
    add_subdirectory(tests)
endif()

include(generate_product_version)

if (COVERAGE)
    include(CMakeFiles/CodeCoverage.cmake)
    append_coverage_compiler_flags()
    setup_target_for_coverage_gcovr_html(NAME coverage)
endif (COVERAGE)

find_package(CURL REQUIRED)

if(NOT DEFINED MSVC)
    set(LD_LIBRARIES pthread m)
endif()

generate_product_version(
   WindowsVersionInfo
   NAME "ldclientapi"
   COMPANY_NAME "Catamorphic"
   VERSION_MAJOR ${PROJECT_VERSION_MAJOR}
   VERSION_MINOR ${PROJECT_VERSION_MINOR}
   VERSION_PATCH ${PROJECT_VERSION_PATCH}
)

set(LD_INCLUDE_PATHS
    "src"
    "include"
    "third-party/include"
    "c-sdk-common/include"
    "c-sdk-common/src"
    ${CURL_INCLUDE_DIR}
)

if(APPLE)
    set(LD_INCLUDE_PATHS ${LD_INCLUDE_PATHS} "/usr/local/include")
    set(LD_PUBLIC_LIBRARIES "-framework CoreFoundation" "-framework IOKit")
endif(APPLE)

file(GLOB SOURCES "src/*" "c-sdk-common/src/*" "third-party/src/*")

if(DEFINED MSVC)
    list(APPEND SOURCES "${WindowsVersionInfo}")
endif()

set(LD_LIBRARIES ${LD_LIBRARIES} ${CURL_LIBRARIES})

configure_file(include/launchdarkly/api.h include/launchdarkly/api.h)

# ldclientapi target -----------------------------------------------------------

add_library(ldclientapi ${SOURCES})

target_link_libraries(ldclientapi
    PUBLIC  ${LD_PUBLIC_LIBRARIES}
    PRIVATE ${LD_LIBRARIES}
)

target_include_directories(ldclientapi
    PUBLIC  "include"
            "c-sdk-common/include"
            "c-sdk-common/src"
    PRIVATE ${LD_INCLUDE_PATHS}
)

target_compile_definitions(ldclientapi
    PRIVATE -D LAUNCHDARKLY_CONCURRENCY_ABORT
            -D LAUNCHDARKLY_USE_ASSERT
            -D LAUNCHDARKLY_DEFENSIVE
)

if(MSVC)
    target_compile_definitions(ldclientapi
        PRIVATE -D CURL_STATICLIB
                -D PCRE_STATIC
                -D _CRT_SECURE_NO_WARNINGS
    )
else()
    target_compile_definitions(ldclientapi
        PRIVATE -D __USE_XOPEN
                -D _GNU_SOURCE
    )

    message(STATUS "Extra compile options: ${EXTRA_COMPILE_OPTIONS}")

    target_compile_options(ldclientapi
        PRIVATE -fno-omit-frame-pointer
                -pedantic
                -Wall
                -Wextra
                -Werror
                -Wstrict-prototypes
                -Wmissing-prototypes
                -Wmissing-declarations
                -std=c89
                ${EXTRA_COMPILE_OPTIONS}
    )
endif(MSVC)

if(BUILD_SHARED_LIBS)
    set_property(TARGET ldclientapi PROPERTY C_VISIBILITY_PRESET hidden)
endif(BUILD_SHARED_LIBS)

if(NOT SKIP_BASE_INSTALL)
    INSTALL(
        TARGETS     ldclientapi
        DESTINATION lib
    )

    INSTALL(
        DIRECTORY              ${PROJECT_SOURCE_DIR}/include/
        DESTINATION            include
        FILES_MATCHING PATTERN "*.h*"
    )

    INSTALL(
        DIRECTORY              ${PROJECT_SOURCE_DIR}/c-sdk-common/include/
        DESTINATION            include
        FILES_MATCHING PATTERN "*.h*"
    )
endif()

# ldclientapicpp target --------------------------------------------------------

if(DEFINED MSVC)
    add_library(ldclientapicpp "cpp/api.cpp" "${WindowsVersionInfo}")
else()
    add_library(ldclientapicpp "cpp/api.cpp")
endif()

target_include_directories(ldclientapicpp PUBLIC "cpp/include")

target_link_libraries(ldclientapicpp PUBLIC ldclientapi)

# test targets -----------------------------------------------------------------

if(BUILD_TESTING)
    file(GLOB TEST_UTILS_SRC "test-utils/src/*" "c-sdk-common/test-utils/src/*")

    add_library(test-utils STATIC ${TEST_UTILS_SRC})

    target_link_libraries(test-utils ldclientapi)

    target_include_directories(test-utils
            PUBLIC ${LD_INCLUDE_PATHS}
                   "test-utils/include"
                   "c-sdk-common/test-utils/include"
            )

    target_compile_definitions(test-utils
            PRIVATE -D LAUNCHDARKLY_USE_ASSERT
                    -D LAUNCHDARKLY_CONCURRENCY_ABORT
            )
endif()

# benchmark targets ------------------------------------------------------------

if(BUILD_BENCHMARKS)
  file(GLOB BENCHMARKS "benchmarks/benchmark-*")
  foreach(benchmarksource ${BENCHMARKS})
      get_filename_component(benchmarksourceleaf ${benchmarksource} NAME)
      string(REPLACE ".c" "" benchmarkexe ${benchmarksourceleaf})
      add_executable(${benchmarkexe} ${benchmarksource})
      target_link_libraries(${benchmarkexe} ldclientapi)
      target_include_directories(${benchmarkexe} PRIVATE ${LD_INCLUDE_PATHS})
      target_compile_definitions(${benchmarkexe}
          PRIVATE -D LAUNCHDARKLY_USE_ASSERT
                  -D LAUNCHDARKLY_CONCURRENCY_ABORT
      )
  endforeach(benchmarksource)
endif()


if (BUILD_TEST_SERVICE)
    add_subdirectory(contract-tests)
endif()
