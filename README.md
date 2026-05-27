# ShadowKey 智能按键精灵

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Windows](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)](https://github.com/mathzimu/shadow-key)
[![C++](https://img.shields.io/badge/C%2B%2B-20-00599C)](https://en.cppreference.com/w/cpp/20)

ShadowKey 是一款轻量级的 Windows 按键精灵工具，支持用户行为录制、智能回放、图像识别驱动点击，以及基础脚本管理。MVP 阶段聚焦个人用户办公/游戏辅助场景，追求性能、隐蔽性与可维护性的平衡。

## 特性

- **行为录制** — 实时捕获键盘按键、鼠标移动和点击事件
- **脚本存储** — 将录制的事件序列化为 `.sks`（JSON）脚本文件
- **智能回放** — 精确还原录制事件，支持循环次数和速度倍率
- **图像识别** — 基于 OpenCV 模板匹配，自动查找目标并点击
- **文本输入** — 模拟人工打字，支持随机间隔和大小写
- **全局热键** — Ctrl+Alt+R 录制 / Ctrl+Alt+S 停止回放
- **防检测** — 贝塞尔曲线鼠标轨迹、随机延时/偏移、高斯分布
- **脚本编辑器** — UI 内直接编辑动作、延时、名称和速度倍率
- **录制过滤** — 可选跳过鼠标移动事件，精简脚本体积
- **轻量 UI** — 基于 Dear ImGui，多标签页界面

## 快速开始

### 环境要求

| 依赖         | 版本         | 说明                               |
| ------------ | ------------ | ---------------------------------- |
| Visual Studio | 2022        | 需要"Desktop development with C++" |
| CMake        | 3.20+        | 构建系统                           |
| OpenCV       | 4.8+         | 图像处理（需手动安装并设置 OpenCV_DIR） |
| 网络连接     | —            | 首次构建时 FetchContent 自动拉取依赖 |

### 构建

```batch
# 克隆仓库
git clone https://github.com/mathzimu/shadow-key.git
cd shadow-key

# 一键构建
scripts\setup_build.bat
```

或手动执行：

```batch
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

构建产物位于 `build\Release\ShadowKey.exe`。

### 依赖说明

以下库由 CMake FetchContent 自动下载：

- [Dear ImGui](https://github.com/ocornut/imgui) — 即时模式 GUI
- [spdlog](https://github.com/gabime/spdlog) — 高性能日志
- [nlohmann/json](https://github.com/nlohmann/json) — JSON 解析

## 使用指南

### 录制与回放

1. 运行 `ShadowKey.exe`
2. 点击 **Start Recording**（或按 Ctrl+Alt+R），执行你希望录制的操作
3. 点击 **Stop Recording**（或再按 Ctrl+Alt+R）完成录制
4. 点击 **Save Script** 将录制内容保存为 `.sks` 文件
5. 点击 **Load Script** 加载已有脚本
6. 点击 **Play** 开始回放，按 Ctrl+Alt+S 停止

### 脚本编辑器

在 **Script Editor** 标签页中可：

- 修改脚本名称和描述
- 调整回放速度倍率（0.1x ~ 5.0x）
- 修改循环次数
- 删除不需要的动作（点击 X → Yes 确认）
- 编辑每个动作的延时

### 文本输入

在 `.sks` 文件中添加 `typing` 动作：

```json
{
  "type": "key_down",
  "vk_code": 0,
  "typing": {
    "text": "Hello, ShadowKey!",
    "min_delay_ms": 30,
    "max_delay_ms": 120
  }
}
```

脚本执行时引擎会逐字符模拟键盘输入，大小写和标点符号自动处理。

### 图像识别触发

```json
{
  "type": "mouse_left_down",
  "x": 0,
  "y": 0,
  "image_trigger": {
    "template": "icon.png",
    "threshold": 0.8,
    "wait_for_match": true,
    "timeout_ms": 5000,
    "click_offset_x": 0,
    "click_offset_y": 0
  }
}
```

脚本执行时，引擎会先在屏幕上搜索 `icon.png`，匹配成功后在目标位置点击，匹配失败则抛出超时错误。

### 防检测配置

在 **Settings** 标签页中可调整：

- 操作延时范围（50-300ms）
- 点击偏移量（±5px）
- 鼠标移动步数
- 截图间隔（≥500ms）

## 项目结构

```
src/
├── main.cpp               # 入口：WinMain + 消息循环
├── core/                   # 核心引擎层
│   ├── input_hook.*        # 录制：SetWindowsHookEx
│   ├── input_sim.*         # 回放：SendInput + 线性插值
│   ├── screen_capture.*    # 截图：BitBlt → OpenCV Mat
│   ├── image_matcher.*     # 模板匹配（多尺度 + NMS）
│   └── anti_detect.*       # 防检测（随机延时/偏移/高斯）
├── script/                 # 脚本层
│   ├── script_format.h     # .sks JSON 格式定义
│   ├── script_parser.cpp   # JSON 序列化/反序列化
│   └── script_executor.*   # 执行引擎（含图像触发）
├── ui/                     # UI 层（Dear ImGui）
│   └── main_window.*       # 主窗口（Main / Settings / Log 标签页）
└── utils/                  # 工具层
    ├── logger.*            # spdlog 封装
    └── timer.*             # QPC 高精度计时器
```

## 测试用例

参见 [TEST_CASES.md](TEST_CASES.md)，涵盖录制、回放、脚本存取、图像识别、防检测 5 个维度共 10 条测试用例。

## 版本历史

参见 [CHANGELOG.md](CHANGELOG.md)。

## 许可证

[MIT](LICENSE) © mathzimu
