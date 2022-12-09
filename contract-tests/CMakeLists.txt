cmake_minimum_required(VERSION 3.14)

project(ContractTests)

include(FetchContent)

set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
include(${CMAKE_FILES}/httplib.cmake)
include(${CMAKE_FILES}/json.cmake)

add_library(http-helpers
        src/http_helpers.cpp
        include/helpers/http_helpers.hpp
        )

target_link_libraries(http-helpers PUBLIC httplib::httplib)
target_include_directories(http-helpers PUBLIC include)

add_library(definitions
        src/definitions.cpp
        include/definitions/definitions.hpp
        )
target_compile_features(definitions PUBLIC cxx_std_17)
target_link_libraries(definitions PUBLIC nlohmann_json::nlohmann_json)
target_include_directories(definitions PUBLIC include)

add_library(service
        src/service.cpp
        src/client_entity.cpp
        include/service/service.hpp
        include/service/client_entity.hpp
        )

target_link_libraries(service
        PUBLIC
        httplib
        PRIVATE
        http-helpers
        definitions
        nlohmann_json::nlohmann_json
        ldclientapicpp
        )
target_include_directories(service PUBLIC include)
add_executable(testservice main.cpp)
target_compile_features(testservice PRIVATE cxx_std_17)

target_link_libraries(testservice PRIVATE service ldclientapicpp)
