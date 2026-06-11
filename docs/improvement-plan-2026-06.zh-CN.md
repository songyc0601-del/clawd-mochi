# Clawd-Mochi 功能完善方案与 Issue 草稿

> 扫描日期：2026-06-11
> 扫描对象：`clawd_mochi/clawd_mochi.ino`（1252 行）
> 用途：按优先级拆分为 issue，逐个派给 Multica Agent 实施。

## 背景

设备核心功能 = 接收开发工作流状态（PLAN/CODE/TEST/BLOCK/DONE/IDLE/OFFLINE）→ 屏幕表情/动画显示。

经完整源码走查 + grep 验证，发现三类问题：**设计与实现脱节**、**功能接通不全**、**死代码残留**。本文件将其拆为 5 个独立 issue（A–E），并给出建议派发顺序。

## 现有功能盘点

| 模块 | 入口 | 状态 |
| --- | --- | --- |
| 工作状态层 | `/progress`、串口 `PROGRESS` | 主功能，Codex 核心脉冲 / Claude 风格两套绘制 |
| 陪伴表情 | `/expr`、首页按钮 | 仅接通 2/5 种表情 |
| 设备控制 | `/cmd` `/backlight` `/state`、串口 | 正常 |
| WiFi 配网 | `/network` `/wifi/*` | 正常，AP+STA |
| OTA 升级 | `/ota` | 正常 |
| 显示模式 | `/agent-mode`、首页 Auto/Codex/Claude | **设置了但不生效** |

---

## Issue A — 死代码清理

- **类型**：`refactor`（行为不变）
- **风险**：低
- **依赖**：无（建议最先做，让后续 issue 在干净基线上展开）

### 删除范围（grep 已确认 0 调用 / 永不触发）

| 项 | 行号 | 说明 |
| --- | --- | --- |
| 整套终端 `term*` | 函数 `622-723` + 常量 `107-117` | `termMode` 从无处写 `true`，约 100 行永不激活 |
| `drawCodeView()` + `VIEW_CODE` | `376-386`、`66` | 0 调用 |
| `animSquishEyes()` / `drawSquishEyes()` + `VIEW_EYES_SQUISH` | `740-748`、`313-327`、`65` | 0 调用 |
| `VIEW_DRAW` + `drawBgColor` | `67`、`87/255` | 画板视图已删，残留枚举/状态（只写不读） |
| `rgb565ToHex` / `hexToRgb565` | `1064`、`236` | 0 调用 |
| `progressBlinkOn` | `84/586/600` | 只写不读，死状态 |
| `drawCodexIdleView` / `isProgressSource` | `575`、`408` | 0 调用 |

### 注意

`drawChevron`（`300`）被 `drawCompanionEyes`（表情视图）复用，**不可删**；删 `drawSquishEyes` 时只删 squish 本身。`eye*` helper 仍被保留视图引用，保留。

### 验收

- 编译通过（用户侧 WSL `arduino-cli`）
- 首页 / 进度 / 表情 / OTA / 配网功能不变
- 预计 -130 行左右

---

## Issue B — 陪伴表情接通全部 5 种

- **类型**：`feat`
- **风险**：低
- **依赖**：无

### 问题

`drawCompanionEyes()`（`329`）已实现 5 种绘制分支：focus / happy / **sleepy / stare / break**，但 `setCompanionExpr()`（`364`）只接受 `focus` / `happy`，另外 3 种永不可达；首页也只有 2 个按钮。

### 改动

- `setCompanionExpr()` `364-374`：扩展接受 `sleepy` / `stare` / `break`，映射到已有枚举 `EXPR_SLEEPY` / `EXPR_STARE` / `EXPR_BREAK`。
- 首页 `INDEX_HTML_LITE` 表情区 `786-790`：补 3 个按钮。
- 串口同步见 Issue D。

### 验收

5 种表情均可经 `/expr?name=` 和首页按钮触发并正确绘制。

### 风险

`stare` / `break` 使用硬编码坐标（`343/354`），需在 240×240 实机确认不越界。

---

## Issue C — agentMode 真正生效（双来源并存 + 择优）⭐ 最高价值

- **类型**：`feat`
- **风险**：中（重写状态模型）
- **依赖**：建议在 Issue A 之后

### 问题

`agentMode`（AUTO/CODEX/CLAUDE）能通过 `/agent-mode` 设置（`416`）、`/state` 回显（`1086`）、首页有按钮并高亮（`782-784`），但 `drawProgressView()`（`561`）**只读 `progressSource`，完全不读 `agentMode`** —— 模式设置无任何显示效果。这是 CONTEXT 旧术语表描述的 Display Mode / Auto Display Mode 语义从未落地。

### 目标状态模型

从单一 `progressState/progressSource` 改为 codex / claude 两套独立并存：

```
codexState / codexMsg / codexLastMs      // codex 来源独立一套
claudeState / claudeMsg / claudeLastMs   // claude 来源独立一套
```

- `/progress?source=codex` 只更新 codex 那套；`source=claude` 只更新 claude 那套。
- `source=none` 或省略：建议更新「当前被显示的那一套」（实现时与用户确认）。

### 显示决策 `chooseDisplay()`

- `agentMode == CODEX` → 只显示 codex 那套
- `agentMode == CLAUDE` → 只显示 claude 那套
- `agentMode == AUTO` → 两套**择优**：按状态优先级，同级用 `lastMs` 近因打破平局

### 状态优先级表（已确认）

高 → 低：

```
BLOCK > DONE > TEST > CODE > PLAN > IDLE > OFFLINE
```

- BLOCK 最高：需人工介入最紧急。
- DONE 提到进行中（TEST/CODE/PLAN）之上：让完成结果优先被看见，避免一端跑完后被另一端持续的 CODE/TEST 盖住。

### 配套改动

- `checkCodexOfflineTimeout()` `611`：改为**每套来源独立超时**回 OFFLINE，超时消息去掉 codex 专属措辞。
- `stateJson()` `1079`：扩展暴露两套状态，供首页区分显示。
- 首页 `paintProgress`：按当前 agentMode / 择优结果渲染。

### 验收

- CODEX / CLAUDE 模式锁定对应来源
- AUTO 下两端交替推送时按优先级正确切换
- 各来源超时独立生效

### 风险

- 状态膨胀，注意 ESP32-C3 内存占用
- 首页协议字段变更需前后端同步
- **派发后实现前，与 Agent 确认 `source` 省略时的行为**

---

## Issue D — 串口协议对齐 HTTP

- **类型**：`feat`
- **风险**：低
- **依赖**：Issue C（MODE 语义随 C 定稿）

### 问题

`handleSerialCommand()`（`1102`）当前只有 `STATE` / `CMD` / `BL` / `PROGRESS`，而 HTTP 已有 `/expr`、`/agent-mode`，协议不对称。若串口是 watcher 主通道，则无法经串口设表情 / 模式。

### 改动

- `EXPR <name>` → 复用 `setCompanionExpr`
- `MODE <auto|codex|claude>` → 复用 `setAgentMode`

### 验收

串口可设表情与模式，输出 `OK` / `ERR`，与 HTTP 行为一致。

---

## Issue E — 健壮性修补

- **类型**：`fix`
- **风险**：低

### 改动

- `drawOtaStatus()` `929`：进入时设置 `currentView`（新增 VIEW_OTA 或置为非 PROGRESS），杜绝 `progressTick` 在 OTA 屏上叠画核心脉冲。
- `/wifi/scan` `892`：可选改异步扫描，避免 `WiFi.scanNetworks()` 同步阻塞数秒（评估后再定）。

### 验收

OTA 升级过程屏幕不被进度动画干扰；配网扫描期间设备响应正常。

---

## 建议派发顺序

```
A（死代码清理）→ B（表情接通）→ C（双来源择优，核心）→ D（串口对齐）→ E（健壮性）
```

- A 先清死代码，C 重写状态模型时噪音最小。
- D 的 MODE 语义依赖 C 定稿。
- B / E 相对独立，可灵活插入或并行。

## 派发约束（摘自 AGENTS.md）

- 不本地编译（VPS 无 arduino-cli）；编译验证由用户侧 WSL 完成。
- 一个 issue 一个 PR；每个任务建 `agent/<slug>` 分支，push 后由用户开 PR / review / merge。
- 绝不修改默认 AP SSID `ClaWD-Mochi` / 密码 `clawd1234`。
- 修改 `.ino` 给局部 diff，不全文重写。
- 任务描述模糊先在 issue 里问清再动手。
