# Codex 客户端状态同步方案

本方案说明 Clawd Mochi 如何从客户端侧自动同步 Codex 状态。

目标不是让模型在每个阶段手动调用脚本，而是在用户电脑上启动一个 watcher、hook、wrapper 或定时任务，由客户端观察 Codex 的运行信号，再把状态推送到 Clawd Mochi 的 HTTP `/progress` 和 `/state` 接口。

## 目标

- 状态同步由客户端负责，不占用模型上下文，也不依赖模型记得执行命令。
- Windows、macOS、Linux、WSL 都能接入同一套设备 HTTP 接口。
- 设备不可达时静默失败或低噪声失败，不影响 Codex 正常工作。
- 保留 `tools/codex-stage.ps1` 和 `tools/codex-stage.sh` 作为跨平台推送原语。

## 非目标

- 不修改 WiFi、OTA、Web 控制页面结构等非状态同步主流程。
- 不要求 Codex 模型主动汇报每个内部阶段。
- 不伪造 Codex 客户端没有公开暴露的精确状态。
- 不把 Arduino 编译、烧录或 OTA 纳入本方案。

## 状态来源

优先级从高到低：

1. Codex 客户端官方 hook/notify 事件。
2. 包装器脚本启动 Codex 前后的生命周期事件。
3. 本地 session 日志或客户端状态文件的变化。
4. 进程活动、文件修改时间等启发式信号。

当前已知可靠信号主要是“会话/回合结束”类通知，适合推送 `DONE`。`PLAN / CODE / TEST / BLOCK` 若客户端没有公开事件，只能通过 hook、日志或包装器约定逐步增强。

## 推荐架构

```text
Codex 客户端
  -> hook / notify / wrapper / watcher
  -> tools/codex-stage.{sh,ps1}
  -> http://设备IP/progress?state=...&msg=...
  -> Clawd Mochi 屏幕
```

`tools/codex-stage.sh` 和 `tools/codex-stage.ps1` 只负责把明确的状态推给设备。它们不负责判断 Codex 当前状态。

## 状态映射

| 客户端信号 | 推送状态 | 说明 |
| --- | --- | --- |
| 没有 Codex session、watcher 停止或客户端明确离线 | `OFFLINE` | 设备回到默认 Clawd 动画形象，`progressMsg` 保留 `codex-offline`。 |
| Codex 已打开但暂无活动任务 | `IDLE` | 表示 Codex 在线待机，不再用于“无 session”。 |
| Codex 进程或会话启动 | `PLAN` | 表示开始处理任务，属于生命周期级别信号。 |
| 模型正在响应或工具调用活跃 | `CODE` | 泛化为工作中，不强行区分写代码和读文件。 |
| 客户端明确进入测试、构建、命令验证 | `TEST` | 只有拿到明确事件或包装器约定时才推送。 |
| 回合结束或任务完成通知 | `DONE` | 当前最可靠的自动信号。 |
| hook 收到失败、被阻塞、需要输入 | `BLOCK` | 仅在客户端能明确判断时推送。 |

`OFFLINE` 是公开协议状态，不是 Codex 工作阶段。HTTP `/progress` 和 USB `PROGRESS` 都可以接收 `OFFLINE`。设备进入 `OFFLINE` 时不会把它记为用户活动；其他进度状态可以记为活动。

## 已实现脚本

当前仓库提供两类脚本：

| 脚本 | 平台 | 作用 |
| --- | --- | --- |
| `tools/codex-stage.sh` | WSL / Linux / macOS | 把指定状态推送到设备 HTTP 接口。 |
| `tools/codex-stage.ps1` | Windows PowerShell | 把指定状态推送到设备，HTTP 不可用时回退 USB 串口。 |
| `tools/codex-watch.sh` | WSL / Linux / macOS | 观察 Codex session JSONL 修改时间，自动推送 `OFFLINE / PLAN / CODE / DONE / IDLE`，并发送心跳。 |
| `tools/codex-watch.ps1` | Windows PowerShell | Windows 版 session watcher，逻辑同上。 |
| `tools/codex-notify.ps1` | Windows PowerShell | Codex notify 回合结束入口，推送 `DONE turn-complete`。 |

Watcher 第一版只使用 session 文件修改时间这个保守信号：

- 没有 session：推送 `OFFLINE codex-offline`。
- 发现新的近期 session：先推送 `PLAN codex-session`，不立即连续推送 `CODE`。
- session 文件后续变化：保持或推送 `CODE active`。
- 默认 20 秒无变化：推送 `DONE turn-complete`。
- 默认 300 秒无变化：推送 `IDLE codex-ready`。
- 默认每 60 秒重发当前有意义状态，避免设备端 120 秒心跳超时。
- `BLOCK` 不被 `DONE` 或 `IDLE` 计时器自动覆盖，只能由明确新状态或设备离线超时替换。
- 长期运行 watcher 退出时推送 `OFFLINE codex-offline`；`--once` / `-Once` 诊断模式退出时不发送 `OFFLINE`。

这不是 Codex 内部阶段的精确事件流，而是客户端侧自动同步的可用基线。后续如果 Codex 暴露正式 hook 或状态 API，应优先替换这套启发式判断。

## Windows 接入

Windows 可以直接启动 watcher：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\codex-watch.ps1
```

如果设备已经连入局域网，可以传入设备 IP：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\codex-watch.ps1 -DeviceUrl http://设备局域网IP
```

也可以继续使用 `tools/codex-notify.ps1` 作为 Codex notify 的回合结束入口：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\codex-notify.ps1
```

任务计划程序适合在登录时启动 watcher。动作示例：

```text
Program/script: powershell
Arguments: -ExecutionPolicy Bypass -File D:\codespace\clawd-mochi\tools\codex-watch.ps1
```

## macOS / Linux / WSL 接入

WSL / Linux / macOS 可以直接启动 watcher：

```bash
./tools/codex-watch.sh
```

设备已经连入局域网时：

```bash
./tools/codex-watch.sh --device-url http://设备局域网IP
```

调试时运行一次：

```bash
./tools/codex-watch.sh --once --verbose
```

可调参数：

```bash
./tools/codex-watch.sh --interval 2 --done-after 20 --idle-after 300 --heartbeat 60
```

macOS 可用 `launchd` 常驻 watcher，Linux 可用 `systemd --user` 或 cron，WSL 可由登录 shell、Windows Terminal 启动脚本，或 Windows 任务计划程序拉起。

Linux systemd user service 示例：

```ini
[Unit]
Description=Clawd Mochi Codex status watcher

[Service]
WorkingDirectory=/home/songyc/code/clawd-mochi
ExecStart=/home/songyc/code/clawd-mochi/tools/codex-watch.sh
Restart=always
RestartSec=5

[Install]
WantedBy=default.target
```

## Watcher 实现建议

第一阶段先做可靠但保守的同步：

- Codex 启动时推 `PLAN codex-started`。
- session 文件或日志在短时间内持续变化时推 `CODE active`。
- Codex notify 或 watcher 发现回合结束时推 `DONE turn-complete`。
- 超过一段时间无变化但 session 仍有效时推 `IDLE codex-ready`。
- 没有 session、watcher 停止或客户端明确离线时推 `OFFLINE codex-offline`。

第二阶段再接入更细状态：

- 如果 Codex 暴露工具调用事件，把 shell/build/test 命令映射到 `TEST`。
- 如果 Codex 暴露阻塞/审批/等待输入事件，把它映射到 `BLOCK`。
- 如果客户端后续提供正式状态 API，优先替换日志和进程启发式判断。

## 当前项目约定

- `AGENTS.md` 不再要求模型在每个阶段主动推状态。
- `tools/codex-watch.sh` 是 WSL/Linux/macOS 的自动 watcher。
- `tools/codex-watch.ps1` 是 Windows 的自动 watcher。
- `tools/codex-stage.sh` 是 WSL/Linux/macOS 的 HTTP 推送原语。
- `tools/codex-stage.ps1` 是 Windows 推送原语，并保留 USB 串口回退能力。
- `tools/codex-notify.ps1` 当前可用于 Windows 回合结束通知。
- 设备不可达不应阻塞 Codex 工作。
