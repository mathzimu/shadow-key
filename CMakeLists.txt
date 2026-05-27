cmake_minimum_required(VERSION 3.20)
project(ShadowKey VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# ---- Dependencies ----------------------------------------------------------

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

# Fetch only what is needed
set(_FETCH_DEPS)
if(spdlog_FETCHED)     list(APPEND _FETCH_DEPS spdlog)       endif()
if(nlohmann_json_FETCHED) list(APPEND _FETCH_DEPS nlohmann_json) endif()
list(APPEND _FETCH_DEPS imgui)

if(_FETCH_DEPS)
    FetchContent_MakeAvailable(${_FETCH_DEPS})
endif()

# ---- Build ----------------------------------------------------------------

add_subdirectory(src)

target_include_directories(ShadowKey PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ---- Install --------------------------------------------------------------

include(GNUInstallDirs)
install(TARGETS ShadowKey
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Copy OpenCV DLLs next to the executable (Windows)
if(WIN32)
    get_target_property(_opencv_dll_dir OpenCV::core IMPORTED_LOCATION)
    if(_opencv_dll_dir)
        get_filename_component(_opencv_dll_dir "${_opencv_dll_dir}" DIRECTORY)
        install(DIRECTORY "${_opencv_dll_dir}/"
                DESTINATION ${CMAKE_INSTALL_BINDIR}
                FILES_MATCHING REGEX "opencv_.*\\.dll$")
    endif()
endif()

# ---- Developer hints -----------------------------------------------------

message(STATUS "ShadowKey ${PROJECT_VERSION}")
message(STATUS "  OpenCV:      ${OpenCV_VERSION}")
message(STATUS "  Install dir: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
