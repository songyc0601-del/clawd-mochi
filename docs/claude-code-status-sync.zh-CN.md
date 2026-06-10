# Claude Code 状态同步说明

本文说明如何把 Claude Code 的本地会话活动同步到 Clawd Mochi。

多客户端场景只推荐统一入口：

```bash
./tools/agent-watch.sh --device-url http://设备局域网IP --background
```

`tools/agent-watch.sh` 会同时观察 Codex 和 Claude Code，读取设备 Web 端 Display Mode，并只推送一个最终状态。不要把 `codex-watch.sh` 和 `claude-watch.sh` 同时连接同一台设备作为日常入口，否则两个来源会互相覆盖。

## 原理

Claude Code 会把项目会话记录写入本机 `~/.claude/projects` 目录。统一 watcher 会递归观察该目录下最新的 `.jsonl` 文件，同时观察 Codex 的 `~/.codex/sessions`。

设备端协议使用状态和来源：

```text
OFFLINE / IDLE / PLAN / CODE / TEST / DONE / BLOCK
source=codex|claude|none
```

Claude Code 候选状态映射：

- 新会话：`PLAN claude-session`
- 活动写入：`CODE active`
- 如果会话日志包含 `task_complete`：发现最新完成事件后推送 `DONE turn-complete`
- 如果会话日志没有生命周期事件：20 秒无写入后回退推送 `DONE turn-complete`
- 300 秒无活动：`IDLE claude-ready`
- watcher 停止、无会话或离线：`OFFLINE claude-offline`

Display Mode 行为：

- `Auto`：按 `BLOCK > TEST > CODE > PLAN > DONE > IDLE > OFFLINE` 选择，优先级相同再比较最近活动时间。
- `Codex`：只显示 Codex；Claude Code 不会抢屏。
- `Claude`：只显示 Claude Code；Claude Code 离线时回到默认 Clawd 动画。

Display Mode 不持久化，设备重启后回到 `AUTO`。

## WSL / Linux / macOS

在项目根目录启动统一 watcher：

```bash
./tools/agent-watch.sh --device-url http://设备局域网IP --background
```

查看状态：

```bash
./tools/agent-watch.sh --status
```

停止 watcher：

```bash
./tools/agent-watch.sh --stop
```

调试时只运行一轮：

```bash
./tools/agent-watch.sh --device-url http://设备局域网IP --once --mode auto --verbose
```

默认配置为：每 2 秒检查一次，60 秒重发心跳，300 秒无活动进入 `IDLE`。如果会话日志有 `task_complete`，watcher 会优先用完成事件判断 `DONE`；如果没有生命周期事件，才回退到 20 秒无写入进入 `DONE`。

如果 Claude Code 使用了非默认目录，可以指定 session 目录：

```bash
./tools/agent-watch.sh \
  --claude-sessions-dir /path/to/claude/projects \
  --device-url http://设备局域网IP \
  --background
```

## Windows PowerShell

第一版暂不提供 Windows 统一 watcher。Windows 侧 `tools/claude-watch.ps1` 保留为单客户端兼容和排错工具：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\claude-watch.ps1 -DeviceUrl http://设备局域网IP -Background
```

查看或停止：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\claude-watch.ps1 -Status
powershell -ExecutionPolicy Bypass -File .\tools\claude-watch.ps1 -Stop
```

调试时运行一轮：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\claude-watch.ps1 -DeviceUrl http://设备局域网IP -Once -VerboseLog
```

默认观察目录是：

```text
%USERPROFILE%\.claude\projects
```

## 与 Codex watcher 的关系

`claude-watch` 是 `codex-watch` 的薄包装，保留用于单客户端兼容和排错：

- `codex-watch` 默认观察 `~/.codex/sessions`，消息前缀为 `codex`。
- `claude-watch` 默认观察 `~/.claude/projects`，消息前缀为 `claude`。
- 两者共用同一套状态机、默认时间阈值、后台运行、pid 文件和日志机制。

多客户端日常使用只推荐 `tools/agent-watch.sh`。
