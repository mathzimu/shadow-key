cmake_minimum_required(VERSION 3.20)
project(ShadowKey VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

find_package(OpenCV 4.8 REQUIRED COMPONENTS core imgproc imgcodecs highgui)

include(FetchContent)

# spdlog: try system first, fall back to FetchContent
if(NOT spdlog_FOUND)
    find_package(spdlog QUIET)
endif()
if(NOT spdlog_FOUND)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.14.1
    )
    set(spdlog_FETCHED TRUE)
endif()

# nlohmann_json: try system first, fall back to FetchContent
if(NOT nlohmann_json_FOUND)
    find_package(nlohmann_json QUIET)
endif()
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    set(nlohmann_json_FETCHED TRUE)
endif()

# imgui: always fetch (no system package available)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.0
)

# Declare all before making available
set(_FETCH_DEPS)
if(spdlog_FETCHED)   list(APPEND _FETCH_DEPS spdlog)   endif()
if(nlohmann_json_FETCHED) list(APPEND _FETCH_DEPS nlohmann_json) endif()
list(APPEND _FETCH_DEPS imgui)

if(_FETCH_DEPS)
    FetchContent_MakeAvailable(${_FETCH_DEPS})
endif()

add_subdirectory(src)

target_include_directories(ShadowKey PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
