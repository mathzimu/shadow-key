# Changelog

## [0.1.0] — 2026-05-28

### Added

- **录制模块** (`core/input_hook`)
  - 基于 `SetWindowsHookEx` 的低层键盘/鼠标钩子
  - 实时捕获 KeyDown、KeyUp、MouseMove、MouseClick、MouseWheel 事件

- **回放模块** (`core/input_sim`)
  - 基于 `SendInput` 的键盘鼠标模拟
  - 线性插值鼠标移动（防瞬移检测）

- **屏幕截图** (`core/screen_capture`)
  - `BitBlt` 全屏/区域/窗口截图 → OpenCV Mat
  - 支持保存为图片文件

- **图像匹配** (`core/image_matcher`)
  - `cv::matchTemplate` 多尺度模板匹配
  - 非极大值抑制（NMS）
  - 多目标匹配 + 置信度阈值过滤

- **防检测引擎** (`core/anti_detect`)
  - Box-Muller 高斯分布随机延时
  - 可配置的点击偏移（±5px）
  - 鼠标移动步数自适应

- **脚本系统** (`script/`)
  - `.sks` JSON 格式定义与序列化（基于 nlohmann/json）
  - 脚本执行引擎，支持循环回放
  - 图像触发等待（wait_for_match + timeout）

- **用户界面** (`ui/main_window`)
  - Dear ImGui 主窗口（Main / Settings / Log 标签页）
  - 录制/回放/暂停/停止 状态机
  - 反检测参数实时调节

- **工具层** (`utils/`)
  - spdlog 日志封装（同时输出到控制台和文件）
  - QPC 高精度定时器

### Project

- CMake 构建系统（FetchContent 自动管理依赖）
- 10 条核心测试用例（TEST_CASES.md）
- MIT 许可证
