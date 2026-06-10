# 多客户端状态显示模式设计

日期：2026-06-07

## 目标

支持 Codex 和 Claude Code 两种客户端同时运行，并通过设备 Web 页面控制屏幕当前显示模式。

用户需要能够在 Web 端选择：

- `Auto`：自动选择当前最重要或最近活跃的客户端状态。
- `Codex`：固定显示 Codex 状态形象。
- `Claude`：固定显示 Claude Code 状态形象。

Codex 和 Claude Code 的屏幕形象不同：Codex 继续使用现有核心脉冲动画；Claude Code 使用 Claude Code 风格动画层。这里不是精确复刻 Claude 官方 logo、字体或动画资源，而是在固件绘图 primitives 内实现可区分的 Claude Code 风格视觉。两者都离线时，设备回到默认 Clawd 动画形象。

## 范围

包含：

- 新增统一客户端 watcher，用一个脚本同时观察 Codex 和 Claude Code。
- 新增 Web 端显示模式控制：`Auto / Codex / Claude`。
- 扩展设备 HTTP 协议以携带显式状态来源 `source=codex|claude|none`。
- `/state` 返回当前显示来源和显示模式。
- 设备端根据来源选择 Codex 或 Claude Code 显示层。
- 保持旧 `/progress?state=...&msg=...` 调用兼容。

不包含：

- 接入更多客户端。
- 精确识别 Codex 或 Claude Code 内部工具调用类型。
- 引入图片资源、外部字体、官方品牌资产或网络依赖。
- 改动 WiFi、OTA、USB 基础控制协议。
- 重写已有 Codex watcher 状态机。
- Windows 版统一 watcher。第一版保留现有 PowerShell 单客户端脚本，Windows 统一 watcher 单独作为后续增强。
- 持久化 Display Mode。设备重启后恢复默认 `AUTO`。
- 自动识别 `TEST` 或 `BLOCK`。第一版只保留这些状态的优先级和转发能力。

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
- 模式切换后，设备只保存 `agentMode` 并刷新 Web 高亮，不直接调用 `/progress`。统一 watcher 在下一次轮询时读取新模式并推送对应状态，默认约 2 秒内反映到屏幕。
- `agentMode` 是运行时偏好，不做断电持久化；设备重启后回到 `AUTO`。

如果统一 watcher 没有运行，Web 页面仍能显示当前设备状态，但不会自行判断 Codex 或 Claude Code 是否在线。

## 显示语义

设备有三类显示层：

| 来源 | 显示层 | 说明 |
| --- | --- | --- |
| `codex` | Codex 核心脉冲层 | 使用现有 Codex 抽象核心、状态色、英文状态词和阶段条。 |
| `claude` | Claude Code 风格动画层 | 使用不同于 Codex 核心脉冲的固件绘制动画、状态色、英文状态词和阶段条。 |
| `none` | 默认 Clawd 层 | 两个客户端都离线，或当前固定模式对应客户端离线。 |

`OFFLINE` 不是工作阶段。进入 `OFFLINE` 时设备显示默认 Clawd 动画，并保留离线原因到 `progressMsg`。

## 统一客户端 watcher

新增脚本：

```bash
tools/agent-watch.sh
```

统一 watcher 负责：

- 观察 Codex session 目录，默认 `~/.codex/sessions`。
- 观察 Claude Code session 目录，默认 `~/.claude/projects`。
- 复用现有 watcher 的状态推断规则，得到两份候选状态。
- 周期性读取设备 `/state`，取得 `agentMode`。
- 根据 `agentMode` 和优先级计算最终输出。
- 只向设备 `/progress` 推送一个最终状态，避免两个 watcher 互相覆盖。

同时使用 Codex 和 Claude Code 时，统一 watcher 是唯一支持的用户入口。`codex-watch.sh` 和 `claude-watch.sh` 暂时保留为单客户端历史用法、内部复用或排错工具，但用户文档不再推荐它们作为多客户端入口。

统一 watcher 默认从设备 `/state.agentMode` 读取 Web 端 Display Mode。调试时可以用 `--mode auto|codex|claude` 临时覆盖。若 `/state` 拉取失败，继续使用上一次成功读取到的模式；如果从未成功读取过，则使用 `AUTO`。

状态变化时立即推送；状态不变时默认每 60 秒重发当前最终状态，避免设备 120 秒心跳超时。长期 watcher 退出时推送 `OFFLINE agents-offline`；`--once` 退出不推送退出离线状态。

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
source=none, OFFLINE agents-offline
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

`DONE` 高于 `IDLE`、低于 `PLAN / CODE / TEST / BLOCK`。同级状态使用最近活动时间打平，例如两个客户端都为 `DONE` 时显示最近完成的一方。

## HTTP 协议

扩展 `/progress`：

```text
/progress?state=CODE&msg=active&source=codex
/progress?state=CODE&msg=active&source=claude
/progress?state=OFFLINE&msg=agents-offline&source=none
```

兼容规则：

- `source` 可选。
- 未传 `source` 时不改变当前 `progressSource`，只更新 `progressState` 和 `progressMsg`。
- 合法来源：`codex`、`claude`、`none`。
- 非法来源返回 `400`。
- `OFFLINE` 不调用 `markAction()`；其他状态可以调用。
- USB 串口 `PROGRESS` 第一版不支持来源，等价于不传 `source`：只更新状态和 message，不改变当前 `progressSource`。
- `OFFLINE` message 约定为：`codex-offline`、`claude-offline`、`agents-offline`。设备端心跳超时仍可使用 `codex-timeout`。

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
String progressSource = "none";
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
- 使用 Claude Code 风格动画层，但在实现上仍用固件绘图 primitives 完成。
- 不引入图片、字体或外部资源。
- 与 Codex 层共用状态色和阶段条语义。
- 顶部标签显示 `CLAUDE`。
- 使用不同于 Codex 核心脉冲的圆环、花瓣或轨道构图。
- `PLAN / CODE / TEST` 有轻量动态变化，`DONE` 稳定显示，`BLOCK` 红色强调。

## 错误处理

- 设备不可达时统一 watcher 静默失败或低噪声失败，不影响 Codex / Claude Code 正常工作。
- `/state` 拉取失败时，统一 watcher 使用上一次成功读取到的模式；没有历史模式时使用 `AUTO`。
- 任一 session 目录不存在时，对应客户端视为离线。
- 固定模式对应客户端离线时，不自动切到另一个客户端，避免违背用户选择。
- `BLOCK` 不被普通完成或待机计时器自动降级，只能由明确新状态或离线超时替换。
- 第一版不要求自动产生 `TEST` 或 `BLOCK`，只要求统一 watcher 能转发和排序这些状态。

## 验收标准

- Web 页面可以切换 `Auto / Codex / Claude`。
- `/state` 返回 `agentMode` 和 `progressSource`。
- `/agent-mode?mode=auto|codex|claude` 可以成功保存模式。
- `/progress` 支持 `source=codex|claude|none`，不传 `source` 时不改变当前 `progressSource`。
- `progressSource` 初始值为 `none`。
- USB `PROGRESS` 不支持来源，且不会改变当前 `progressSource`。
- 同时使用 Codex 和 Claude Code 时，只保留统一 watcher 作为用户入口，避免状态互相覆盖。
- `agent-watch.sh` 默认读取设备 `/state.agentMode`；`--mode auto|codex|claude` 只作为调试覆盖。
- `/state` 拉取失败时保留上一次成功模式，首次失败时使用 `AUTO`。
- Web 切换 Display Mode 后不直接推送 `/progress`，由统一 watcher 默认 2 秒内反映到屏幕。
- Display Mode 不做断电持久化，设备重启后为 `AUTO`。
- `Auto` 模式按状态优先级和最近活动时间选择显示来源。
- `Codex` 模式不会被 Claude Code 状态抢屏。
- `Claude` 模式不会被 Codex 状态抢屏。
- Claude Code 状态显示 Claude Code 风格动画层，Codex 状态显示 Codex 核心脉冲动画。
- 两者都离线或固定模式对应客户端离线时，设备回到默认 Clawd 动画形象。
- 第一版不要求 Windows 统一 watcher。
- 第一版不要求自动识别 `TEST` 或 `BLOCK`。
- 长运行统一 watcher 在最终状态不变时每 60 秒重发心跳，退出时推 `OFFLINE agents-offline`；`--once` 退出不推离线状态。
- 现有 `tools/codex-watch.sh` 和 `tools/claude-watch.sh` 保留用于兼容、内部复用或排错，但多客户端用户文档只推荐 `tools/agent-watch.sh`。
- 固件主程序与 `dist/clawd_mochi/clawd_mochi.ino` 在实现后保持一致。

## 实施注意

- 先写统一 watcher 的行为测试，再实现脚本。
- 固件测试应检查 `/progress` source、`/agent-mode`、`/state` 字段和 Web 控件字符串。
- Web 控制只负责保存显示模式，不直接推送工作状态。
- 文档需要明确：多客户端同时使用时只使用 `agent-watch.sh` 作为用户入口，不要让 `codex-watch.sh` 和 `claude-watch.sh` 同时连接同一设备。
- Windows 统一 watcher 后续补齐；本阶段只要求 WSL/Linux 脚本可用，并保留现有 PowerShell 单客户端脚本。
