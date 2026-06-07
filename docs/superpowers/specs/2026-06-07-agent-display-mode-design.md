# 多客户端状态显示模式设计

日期：2026-06-07

## 目标

支持 Codex 和 Claude Code 两种客户端同时运行，并通过设备 Web 页面控制屏幕当前显示模式。

用户需要能够在 Web 端选择：

- `Auto`：自动选择当前最重要或最近活跃的客户端状态。
- `Codex`：固定显示 Codex 状态形象。
- `Claude`：固定显示 Claude Code 状态形象。

Codex 和 Claude Code 的屏幕形象不同：Codex 继续使用现有核心脉冲动画；Claude Code 使用 Claude 官方风格动画形象。两者都离线时，设备回到默认 Clawd 动画形象。

## 范围

包含：

- 新增统一客户端 watcher，用一个脚本同时观察 Codex 和 Claude Code。
- 新增 Web 端显示模式控制：`Auto / Codex / Claude`。
- 扩展设备 HTTP 协议以携带状态来源 `source=codex|claude`。
- `/state` 返回当前显示来源和显示模式。
- 设备端根据来源选择 Codex 或 Claude Code 显示层。
- 保持旧 `/progress?state=...&msg=...` 调用兼容。

不包含：

- 接入更多客户端。
- 精确识别 Codex 或 Claude Code 内部工具调用类型。
- 引入图片资源、外部字体或网络依赖。
- 改动 WiFi、OTA、USB 基础控制协议。
- 重写已有 Codex watcher 状态机。

## 用户体验

Web 首页的“工作状态”区域增加一个模式切换控件：

```text
工作状态
模式：Auto | Codex | Claude
当前：Claude Code / CODE
```

模式行为：

- `Auto` 是默认模式。
- `Codex` 只允许 Codex 状态成为屏幕输出；Codex 离线时显示默认 Clawd 动画。
- `Claude` 只允许 Claude Code 状态成为屏幕输出；Claude Code 离线时显示默认 Clawd 动画。
- 模式切换后，统一 watcher 在下一次轮询时读取新模式并推送对应状态。

如果统一 watcher 没有运行，Web 页面仍能显示当前设备状态，但不会自行判断 Codex 或 Claude Code 是否在线。

## 显示语义

设备有三类显示层：

| 来源 | 显示层 | 说明 |
| --- | --- | --- |
| `codex` | Codex 核心脉冲层 | 使用现有 Codex 抽象核心、状态色、英文状态词和阶段条。 |
| `claude` | Claude Code 动画层 | 使用 Claude 官方风格动画形象、状态色、英文状态词和阶段条。 |
| `none` | 默认 Clawd 层 | 两个客户端都离线，或当前固定模式对应客户端离线。 |

`OFFLINE` 不是工作阶段。进入 `OFFLINE` 时设备显示默认 Clawd 动画，并保留离线原因到 `progressMsg`。

## 统一客户端 watcher

新增脚本：

```bash
tools/agent-watch.sh
```

后续 Windows 可补充：

```powershell
tools/agent-watch.ps1
```

统一 watcher 负责：

- 观察 Codex session 目录，默认 `~/.codex/sessions`。
- 观察 Claude Code session 目录，默认 `~/.claude/projects`。
- 复用现有 watcher 的状态推断规则，得到两份候选状态。
- 周期性读取设备 `/state`，取得 `agentMode`。
- 根据 `agentMode` 和优先级计算最终输出。
- 只向设备 `/progress` 推送一个最终状态，避免两个 watcher 互相覆盖。

推荐用户同时使用 Codex 和 Claude Code 时只启动统一 watcher，不再分别启动 `codex-watch.sh` 和 `claude-watch.sh` 连接同一台设备。

## 模式与优先级

设备保存的显示模式：

```text
AUTO
CODEX
CLAUDE
```

统一 watcher 使用以下规则：

### CODEX

- Codex 在线：推送 Codex 当前状态。
- Codex 离线：推送 `OFFLINE codex-offline`，来源为 `codex` 或 `none` 均可，但设备最终显示默认 Clawd。
- Claude Code 状态不参与选择。

### CLAUDE

- Claude Code 在线：推送 Claude Code 当前状态。
- Claude Code 离线：推送 `OFFLINE claude-offline`，设备显示默认 Clawd。
- Codex 状态不参与选择。

### AUTO

两者都离线：

```text
OFFLINE agents-offline
```

只有一个在线：

```text
显示在线客户端状态
```

两者都在线时先按状态优先级比较：

```text
BLOCK > TEST > CODE > PLAN > DONE > IDLE > OFFLINE
```

状态优先级相同则按最近活动时间比较；最近活动者胜出。仍然完全相同则使用默认客户端顺序：

```text
Codex > Claude Code
```

这个默认顺序只用于打平，不代表 Codex 永远优先。

## HTTP 协议

扩展 `/progress`：

```text
/progress?state=CODE&msg=active&source=codex
/progress?state=CODE&msg=active&source=claude
```

兼容规则：

- `source` 可选。
- 未传 `source` 时按 `codex` 处理，旧脚本保持兼容。
- 合法来源：`codex`、`claude`。
- 非法来源返回 `400`。
- `OFFLINE` 不调用 `markAction()`；其他状态可以调用。

新增 `/agent-mode`：

```text
/agent-mode?mode=auto
/agent-mode?mode=codex
/agent-mode?mode=claude
```

规则：

- 合法模式：`auto`、`codex`、`claude`。
- 固件内部保存为大写枚举或字符串。
- 成功返回 `{"ok":1}`。
- 非法模式返回 `400`。
- 修改模式属于用户操作，可以调用 `markAction()`。

扩展 `/state`：

```json
{
  "progress": "CODE",
  "progressMsg": "active",
  "progressSource": "claude",
  "agentMode": "AUTO"
}
```

兼容规则：

- 保留现有字段。
- 新增字段不改变旧客户端读取行为。

## 固件显示分支

新增状态来源变量：

```cpp
String progressSource = "codex";
String agentMode = "AUTO";
```

`setProgress()` 只做状态设置和绘制，不决定是否记为用户活动。

`drawProgressView()` 根据状态和来源分支：

```text
OFFLINE -> drawDefaultClawdView()
source=codex -> drawCodexProgressView()
source=claude -> drawClaudeProgressView()
```

Claude Code 显示层要求：

- 不复用 Clawd 眼睛或默认角色。
- 使用 Claude 官方风格动画形象，但在实现上仍用固件绘图 primitives 完成。
- 不引入图片、字体或外部资源。
- 与 Codex 层共用状态色和阶段条语义。
- `PLAN / CODE / TEST / DONE / BLOCK` 必须有可区分的颜色和动效。

## 错误处理

- 设备不可达时统一 watcher 静默失败或低噪声失败，不影响 Codex / Claude Code 正常工作。
- `/state` 拉取失败时，统一 watcher 使用本地默认 `AUTO` 模式继续推送。
- 任一 session 目录不存在时，对应客户端视为离线。
- 固定模式对应客户端离线时，不自动切到另一个客户端，避免违背用户选择。
- `BLOCK` 不被普通完成或待机计时器自动降级，只能由明确新状态或离线超时替换。

## 验收标准

- Web 页面可以切换 `Auto / Codex / Claude`。
- `/state` 返回 `agentMode` 和 `progressSource`。
- `/agent-mode?mode=auto|codex|claude` 可以成功保存模式。
- `/progress` 支持 `source=codex|claude`，不传 `source` 时仍兼容旧 Codex 行为。
- 同时启动 Codex 和 Claude Code 时，只需要运行统一 watcher 即可避免状态互相覆盖。
- `Auto` 模式按状态优先级和最近活动时间选择显示来源。
- `Codex` 模式不会被 Claude Code 状态抢屏。
- `Claude` 模式不会被 Codex 状态抢屏。
- Claude Code 状态显示 Claude 官方风格动画形象，Codex 状态显示 Codex 核心脉冲动画。
- 两者都离线或固定模式对应客户端离线时，设备回到默认 Clawd 动画形象。
- 现有 `tools/codex-watch.sh` 和 `tools/claude-watch.sh` 保留，可用于单客户端场景。
- 固件主程序与 `dist/clawd_mochi/clawd_mochi.ino` 在实现后保持一致。

## 实施注意

- 先写统一 watcher 的行为测试，再实现脚本。
- 固件测试应检查 `/progress` source、`/agent-mode`、`/state` 字段和 Web 控件字符串。
- Web 控制只负责保存显示模式，不直接推送工作状态。
- 文档需要明确：多客户端同时使用时推荐运行 `agent-watch.sh`，不要让 `codex-watch.sh` 和 `claude-watch.sh` 同时连接同一设备。
- Windows 统一 watcher 可以后续补齐；本阶段至少保证 WSL/Linux 脚本可用，并保留现有 PowerShell 单客户端脚本。
