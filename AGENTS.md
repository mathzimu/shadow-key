## Build

### Windows (target)
```bash
# Prerequisites
# 1. Install VS2022 with "Desktop development with C++"
# 2. Install CMake 3.20+
# 3. Install OpenCV 4.8+ and set OpenCV_DIR
# 4. Internet connection (FetchContent downloads imgui/spdlog/nlohmann_json)

scripts\setup_build.bat
```

### macOS (syntax check only)
```bash
brew install cmake opencv spdlog
cmake -B build -S .
# Note: imgui is fetched from GitHub (requires internet)
cmake --build build -j
```

## Project Structure
```
src/
├── main.cpp              # Entry point
├── core/                 # Core engine layer
│   ├── input_hook.*      # Recording: SetWindowsHookEx
│   ├── input_sim.*       # Playback: SendInput
│   ├── screen_capture.*  # Screenshot: BitBlt → OpenCV Mat
│   ├── image_matcher.*   # Template matching (multiscale + NMS)
│   └── anti_detect.*     # Anti-detection: random delay/offset/gaussian
├── script/               # Script layer
│   ├── script_format.h   # .sks JSON format definition
│   ├── script_parser.cpp # JSON serialization
│   └── script_executor.* # Playback engine with image trigger support
├── ui/                   # UI layer (Dear ImGui)
│   └── main_window.*     # Main window with tabs
└── utils/                # Utility layer
    ├── logger.*          # spdlog wrapper
    └── timer.*           # QPC high-precision timer
```

## Dependencies
- OpenCV 4.8+ (core, imgproc, imgcodecs, highgui)
- Dear ImGui 1.91+ (auto-fetched via FetchContent)
- spdlog 1.14+ (auto-fetched, or system)
- nlohmann/json 3.11+ (auto-fetched, or system)

## Test Cases
See TEST_CASES.md for 10 core test cases.
