set(SOURCES
    main.cpp
    core/input_hook.cpp
    core/input_sim.cpp
    core/screen_capture.cpp
    core/image_matcher.cpp
    core/anti_detect.cpp
    core/hotkey_manager.cpp
    script/script_parser.cpp
    script/script_executor.cpp
    ui/main_window.cpp
    utils/logger.cpp
    utils/timer.cpp
)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)

add_executable(ShadowKey WIN32 ${SOURCES} ${IMGUI_SOURCES})

target_include_directories(ShadowKey PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

target_link_libraries(ShadowKey
    PRIVATE
    ${OpenCV_LIBS}
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    winmm
    user32
    gdi32
)
