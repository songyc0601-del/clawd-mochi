# Clawd Mochi 用户操作手册

本文面向日常使用者，说明如何给 Clawd Mochi 配网、打开网页控制、接入 Codex / Claude Code 工作状态屏、执行 OTA 升级和处理常见问题。

## 设备用途

Clawd Mochi 是一个基于 ESP32-C3 和 ST7789 屏幕的桌面状态摆件。当前支持：

- 默认 Clawd 动画形象。
- 手机或电脑网页控制。
- Codex / Claude Code 工作状态显示。
- WiFi OTA 在线升级。
- USB 串口控制和排错。

## 首次开机与配网

1. 用 USB-C 数据线或稳定电源给设备供电。
2. 等待屏幕启动，设备会开启热点 `ClaWD-Mochi`。
3. 手机或电脑连接这个热点。
4. WiFi 密码输入：

```text
clawd1234
```

5. 浏览器打开：

```text
http://192.168.4.1
```

6. 点击“网络设置”。
7. 扫描并选择家里或办公室的 2.4 GHz WiFi。
8. 输入 WiFi 密码并保存。
9. 等待设备连接成功，屏幕或网页会显示设备的局域网 IP。

注意：

- ESP32-C3 只支持 2.4 GHz WiFi，不支持 5 GHz WiFi。
- `ClaWD-Mochi` 热点会保留，用于兜底维护。
- 配网失败时，重新连接 `ClaWD-Mochi`，打开 `http://192.168.4.1/network` 再配置。

## 日常网页控制

设备连入局域网后，电脑和手机不需要再连接 `ClaWD-Mochi` 热点。只要和设备在同一个 WiFi 或局域网内，浏览器打开：

```text
http://192.168.5.4
```

网页首页常用功能：

- 刷新状态：读取设备当前状态。
- 屏幕开关：打开或关闭背光。
- 基础表情：回到默认正常形象。
- 陪伴表情：
  - `focus`：专注
  - `happy`：开心
- 网络设置：重新配置 WiFi。
- OTA 在线升级：上传新固件。

如果忘记设备局域网 IP，可以重新连接 `ClaWD-Mochi` 热点，然后打开：

```text
http://192.168.4.1
```

## 工作状态屏

工作状态屏用于显示电脑上 Codex 或 Claude Code 的工作状态。多客户端场景只推荐启动统一 watcher：`tools/agent-watch.sh`。它会同时观察 Codex 和 Claude Code，本机计算最终状态后只向设备推送一条结果，避免两个 watcher 互相覆盖。

Web 首页的工作状态区域可以切换 Display Mode：

| 模式 | 行为 |
| --- | --- |
| `Auto` | 默认模式。按 `BLOCK > TEST > CODE > PLAN > DONE > IDLE > OFFLINE` 选择，优先级相同再比较最近活动时间。 |
| `Codex` | 只显示 Codex。Codex 离线时回到默认 Clawd 动画，不切到 Claude Code。 |
| `Claude` | 只显示 Claude Code。Claude Code 离线时回到默认 Clawd 动画，不切到 Codex。 |

Display Mode 只在运行时有效，设备重启后回到 `AUTO`。Web 切换模式后不直接推送 `/progress`，统一 watcher 会在下一轮轮询中读取新模式并更新屏幕，默认约 2 秒内生效。

### 状态含义

| 状态 | 屏幕表现 | 含义 |
| --- | --- | --- |
| `OFFLINE` | 默认 Clawd 动画形象 | 客户端未打开、watcher 未运行、watcher 停止或设备超时 |
| `IDLE` | 核心低亮待机屏 | 客户端已在线但暂无活动任务 |
| `PLAN` | 状态屏，阶段 1/4 | 开始处理任务或规划中 |
| `CODE` | 状态屏，阶段 2/4 | 正在工作 |
| `TEST` | 状态屏，阶段 3/4 | 正在验证、测试、构建或上传 |
| `DONE` | 状态屏，阶段 4/4 | 当前回合完成 |
| `BLOCK` | 红色满格状态 | 需要用户输入或确实被阻塞 |

Codex 使用核心脉冲形象；Claude Code 使用 Claude Code 风格动画层。两者共用状态颜色、英文状态词和 4 段阶段条。

离线消息约定：

- `codex-offline`：Codex 离线。
- `claude-offline`：Claude Code 离线。
- `agents-offline`：统一 watcher 判定所有客户端离线或 watcher 退出。
- `codex-timeout`：设备 120 秒没有收到非 `OFFLINE` 心跳后的设备端超时原因。

### 统一 watcher 后台启动

WSL / Linux / macOS：

在项目根目录运行：

```bash
./tools/agent-watch.sh --device-url http://192.168.5.4 --background
```

查看 watcher 是否运行：

```bash
./tools/agent-watch.sh --status
```

停止 watcher：

```bash
./tools/agent-watch.sh --stop
```

调试时只运行一轮：

```bash
./tools/agent-watch.sh --device-url http://192.168.5.4 --once --mode auto --verbose
```

日常使用建议保持脚本默认配置：

```bash
./tools/agent-watch.sh --device-url http://192.168.5.4 --background
```

默认配置为：每 2 秒检查一次 Codex session 和 Claude Code session，60 秒重发一次心跳。Codex 默认观察 `~/.codex/sessions`，Claude Code 默认观察 `~/.claude/projects`。第一版不自动识别真实 `TEST` 或 `BLOCK`，但会保留这些状态的协议优先级和转发能力。

模式说明：

- 默认 `--mode device`：读取设备 Web 端 `/state.agentMode`。
- 调试可用 `--mode auto|codex|claude` 临时覆盖 Web 设置。
- 如果 `/state` 临时失败，watcher 使用上一次成功读取的模式；从未读取成功时使用 `AUTO`。
- 长运行 watcher 退出时推送 `OFFLINE agents-offline`；`--once` 退出不推离线状态。

Windows 第一版暂不提供统一 watcher。现有 `codex-watch.ps1` 和 `claude-watch.ps1` 保留为单客户端兼容和排错工具，不建议同时连接同一台设备。

### 单客户端排错入口

只排查某一个客户端时，可以临时使用旧 watcher：

```bash
./tools/codex-watch.sh --device-url http://192.168.5.4 --once --verbose
./tools/claude-watch.sh --device-url http://192.168.5.4 --once --verbose
```

不要把 `codex-watch.sh` 和 `claude-watch.sh` 同时作为多客户端日常入口。

### 手动推送状态

WSL / Linux / macOS：

```bash
./tools/codex-stage.sh -DeviceUrl http://192.168.5.4 -State CODE -Message active
./tools/codex-stage.sh -DeviceUrl http://192.168.5.4 -State OFFLINE -Message codex-offline
./tools/codex-stage.sh -DeviceUrl http://192.168.5.4 -State CODE -Message active -Source codex
```

Windows：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\codex-stage.ps1 -DeviceUrl http://192.168.5.4 -State CODE -Message active
powershell -ExecutionPolicy Bypass -File .\tools\codex-stage.ps1 -DeviceUrl http://192.168.5.4 -State OFFLINE -Message codex-offline
```

协议 smoke test：

```bash
curl "http://192.168.5.4/agent-mode?mode=auto"
curl "http://192.168.5.4/state"
curl "http://192.168.5.4/progress?state=OFFLINE&msg=agents-offline&source=none"
```

## OTA 在线升级

OTA 适合设备已经完成首次 USB 烧录后的后续升级。

1. 编译生成 ESP32-C3 对应的 `.bin` 固件。
2. 确认电脑和设备在同一个局域网。
3. 浏览器打开：

```text
http://192.168.5.4/ota
```

4. 选择 `.bin` 文件。
5. 点击上传并升级。
6. 等待设备自动重启。

兜底方式：连接 `ClaWD-Mochi` 热点，打开：

```text
http://192.168.4.1/ota
```

升级过程中不要断电。不要上传 `.ino`、`.zip` 或非 ESP32-C3 固件。

## USB 串口控制

USB 串口适合首次烧录、自动化脚本和排错。

串口参数：

```text
115200 baud
```

常用命令：

```text
STATE
CMD normal
BL 0
BL 1
PROGRESS IDLE
PROGRESS PLAN planning
PROGRESS CODE active
PROGRESS TEST verifying
PROGRESS DONE complete
PROGRESS BLOCK need-input
PROGRESS OFFLINE codex-offline
```

`STATE` 会返回 JSON 状态，可用于确认当前页面、背光、WiFi、`agentMode`、`progressSource` 和当前工作状态。USB `PROGRESS` 第一版不支持来源参数，不会改变当前 `progressSource`。

## 常见问题

### 打不开 `http://192.168.4.1`

确认手机或电脑当前连接的是 `ClaWD-Mochi` 热点。连接家庭 WiFi 时，`192.168.4.1` 通常打不开，需要改用设备局域网 IP。

### 设备连上 WiFi 但网页打不开

确认电脑和设备在同一个局域网。部分路由器开启了 AP 隔离，手机或电脑可能无法访问同 WiFi 下的设备，需要关闭 AP 隔离或换一个网络。

### watcher 启动后屏幕还是默认形象

先确认 watcher 状态：

```bash
./tools/agent-watch.sh --status
```

再手动推送一次测试状态：

```bash
./tools/codex-stage.sh -DeviceUrl http://192.168.5.4 -State CODE -Message test -Source codex
```

如果手动推送有效，说明设备网络正常，问题通常在 watcher 没找到客户端 session 或客户端当前没有活动。

### 状态停在 `DONE`

这是正常现象。Codex watcher 会优先等待 session 中的 `task_complete`，因此回合结束后通常在下一个轮询周期进入 `DONE`；300 秒后进入 `IDLE`。如果正在运行客户端时 `CODE` 和 `DONE` 来回切换，通常是使用了没有生命周期事件的来源并且 `--done-after` 设置过短；恢复默认配置后重新启动 watcher。

### 设备显示 `OFFLINE codex-timeout`

设备超过 120 秒没有收到非 `OFFLINE` 心跳。检查 watcher 是否运行、设备 IP 是否正确、电脑和设备是否在同一网络。

### OTA 上传失败

确认上传的是为 ESP32-C3 编译出的 `.bin` 文件。升级过程中不要断电。失败后重新上电，再打开 `/ota` 页面重试。

### USB 找不到串口

换一根支持数据传输的 USB-C 线。Windows 下查看设备管理器；WSL 下确认已经用 `usbipd` attach 到 WSL，并能看到 `/dev/ttyACM0` 或 `/dev/ttyUSB0`。

## 维护入口

- 网页首页：`http://192.168.5.4`
- 网络设置：`http://192.168.5.4/network`
- OTA 升级：`http://192.168.5.4/ota`
- 热点兜底首页：`http://192.168.4.1`
- 热点兜底网络设置：`http://192.168.4.1/network`
- 热点兜底 OTA：`http://192.168.4.1/ota`
