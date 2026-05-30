cmake_minimum_required(VERSION 3.22)

project(incident-backend-cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Find packages
find_package(Drogon CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SqliteOrm CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

# Main executable
file(GLOB_RECURSE SRC_FILES src/*.cpp src/**/*.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Drogon::Drogon
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    sqlite_orm::sqlite_orm
)

# Enable C++20
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)

# Tests subdirectory
add_subdirectory(tests)
