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

if(APPLE)
    list(APPEND SOURCES ui/mac_dialogs.mm)
endif()

if(WIN32)
    set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    )
elseif(APPLE)
    set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${glad_SOURCE_DIR}/src/glad.c
    )
endif()

if(WIN32)
    add_executable(ShadowKey WIN32 ${SOURCES} ${IMGUI_SOURCES})
else()
    add_executable(ShadowKey ${SOURCES} ${IMGUI_SOURCES})
endif()

target_include_directories(ShadowKey PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

if(APPLE)
    target_include_directories(ShadowKey PRIVATE ${glad_SOURCE_DIR}/include)
    target_compile_definitions(ShadowKey PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)
endif()

target_link_libraries(ShadowKey
    PRIVATE
    ${OpenCV_LIBS}
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(ShadowKey PRIVATE winmm user32 gdi32)
elseif(APPLE)
    target_link_libraries(ShadowKey PRIVATE
        glfw
        OpenGL::GL
        "-framework Carbon"
        "-framework CoreGraphics"
        "-framework CoreFoundation"
        "-framework ApplicationServices"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
endif()
