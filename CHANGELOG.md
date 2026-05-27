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

## [0.2.0] — 2026-05-28

### Added

- **全局热键** (`core/hotkey_manager`)
  - `RegisterHotKey` 实现系统级快捷键
  - Ctrl+Alt+R 切换录制开关
  - Ctrl+Alt+S 停止回放

- **文本输入模拟** (`core/input_sim`)
  - `type_text()` 逐字符模拟键盘输入
  - 支持大小写、标点符号、特殊键
  - 随机延时间隔（可在 Settings 中配置）

- **贝塞尔曲线鼠标移动** (`core/input_sim`)
  - 三次贝塞尔曲线插值，轨迹更接近真人
  - 控制点加入随机偏移（±15px）
  - 可在 Settings 中切换 Linear/Bezier

- **回放速度倍率** (`script/script_format`, `script_executor`)
  - `speed_multiplier` 字段（0.1x - 5.0x）
  - 影响操作间延时，不影响事件本身执行

- **录制过滤** (`ui/main_window`, `core/anti_detect`)
  - "Filter MouseMove" 开关
  - 开启时忽略鼠标移动事件，脚本更精简

- **脚本编辑器** (`ui/main_window`)
  - Script Editor 标签页
  - 删除动作（带确认弹窗）
  - 编辑动作间延时
  - 编辑脚本名称/描述
  - 实时调整 Speed Multiplier

### Changed

- **UI 布局** 窗口从 600x500 调整为 700x600，增加 Script Editor 标签页
- **线程安全** 录制回调改用 `events_mutex_` + `pending_events_` 双缓冲模式
- **反检测配置** 新增 `typing_min/max_delay_ms`、`curve_mode`、`record_filter_mousemove` 字段
- **脚本格式** `.sks` 新增 `speed_multiplier`、`typing` 字段

### Testing

- 新增 7 条测试用例（TC-011 ~ TC-017）
