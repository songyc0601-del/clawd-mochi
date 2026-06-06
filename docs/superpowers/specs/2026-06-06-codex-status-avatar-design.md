# Codex 状态动画形象屏设计

日期：2026-06-06

## 目标

把 Codex 工作状态屏从静态仪表盘改为“Codex 核心脉冲”动画形象。

屏幕应通过一个抽象的发光核心、状态颜色、英文状态词和轻量动画表达当前阶段，避免使用 Clawd、眼睛、动物或外壳角色语义。

## 范围

本设计只覆盖 240×240 TFT 上的 Codex 状态显示效果，以及 Codex 状态屏与默认 Clawd 动画形象之间的切换语义。

包含：

- 默认 Clawd 动画形象与 Codex 状态屏的切换规则。
- `OFFLINE / IDLE / PLAN / CODE / TEST / DONE / BLOCK` 七个状态的视觉映射。
- Codex 核心脉冲形象。
- 状态色、英文状态词、短 message、阶段条。
- 轻量动画规则。

不包含：

- watcher 判定逻辑。
- `/progress`、`/state`、USB 串口协议变更。
- WiFi、OTA、Web 控制页面结构变更。
- 新图片、字体、外部资源或复杂帧动画。

## 视觉方向

设备有两层显示语义：

1. 默认层：显示原有 Clawd 动画形象。设备刚开机、Codex 未连接、watcher 停止或 Codex 离线时都回到这一层。
2. Codex 层：只有客户端明确推送 Codex 在线状态后才进入。此层主视觉是一个位于屏幕中上部的抽象 Codex 核心。

Codex 层的核心形象：

- 中心实心圆表示当前 Codex 活动核心。
- 外层圆环表示思考、执行或验证中的能量场。
- 外圈状态边框使用当前状态色。
- 底部显示大号英文状态词。
- 状态词下方或附近显示短 message，例如 `active`、`planning`、`turn-complete`。
- 阶段条保留 4 段，辅助表达流程进度。

Codex 层整体风格应偏 Codex、终端、AI 工作状态，而不是角色表情。默认层可以继续使用现有 Clawd 动画形象，但不在 Codex 状态屏中混用 Clawd 角色。

## 状态语义

新增 `OFFLINE` 作为内部/客户端语义状态：

- `OFFLINE`：Codex 没有打开、watcher 没有运行、Codex 超时离线，或客户端显式告知离线。设备显示默认 Clawd 动画形象，不显示 Codex 核心脉冲屏。
- `IDLE`：Codex 已打开且 watcher 在线，但当前没有活动任务。只有这个状态才显示 Codex 待机屏。
- `PLAN / CODE / TEST / DONE / BLOCK`：Codex 已在线并处于对应工作阶段。

实现时可以选择不把 `OFFLINE` 暴露为长期用户协议状态；但客户端、watcher 或超时逻辑必须能触发“回到默认 Clawd 动画形象”的行为。若为了简单实现而复用现有 `IDLE`，则需要另有字段或命令区分“Codex 在线待机”和“Codex 离线”，不能继续让设备开机默认显示 Codex 待机。

## 状态映射

| 状态 | 颜色 | 显示层 | 核心表现 | 阶段条 | message 示例 |
| --- | --- | --- | --- | --- |
| `OFFLINE` | 默认配色 | 默认层 | 显示默认 Clawd 动画形象 | 不显示 | 空 |
| `IDLE` | 低亮灰 / 暗橙 | Codex 层 | Codex 核心低亮慢呼吸 | 0/4 | `codex-ready` |
| `PLAN` | 蓝色 | Codex 层 | 外环缓慢扩张，像规划中 | 1/4 | `planning` |
| `CODE` | 橙色 | Codex 层 | 核心脉冲更明显，状态点闪烁 | 2/4 | `active` |
| `TEST` | 青绿 / 绿色 | Codex 层 | 加一条横向扫描线或更快脉冲 | 3/4 | `verifying` |
| `DONE` | 绿色 | Codex 层 | 核心稳定亮起，不再闪烁 | 4/4 | `turn-complete` |
| `BLOCK` | 红色 | Codex 层 | 边框和核心红色高亮，可短促闪烁 | 4/4 | `need-input` |

## 布局

Codex 层推荐坐标分区：

- 顶部 0-8px：状态色边框。
- 12-30px：小号 `CODEX` 标签和右上状态点。
- 44-130px：核心脉冲形象。
- 146-165px：4 段阶段条。
- 172-205px：大号英文状态词。
- 212-228px：短 message，超长时截断。
- 底部 232-240px：状态色边框。

状态词需要保证 `BLOCK` 也能完整居中显示，避免 `DONE` 被误读成 `NONE`。如果现有字号导致可读性差，应降低字号或改用更清晰的字符间距。

## 动画规则

动画应轻量、可预测，并基于现有 `progressTick()` 类似机制刷新局部区域：

- 不使用图片帧。
- 不引入外部资源。
- 不做全屏高频重绘。
- 每 400-700ms 更新一次脉冲阶段即可。
- `IDLE` 使用低亮慢呼吸，表示 Codex 已打开但暂无任务。
- 运行态 `PLAN / CODE / TEST` 可以持续动画。
- `DONE` 使用稳定亮起或一次性完成态。
- `BLOCK` 可以使用低频红色闪烁，但不能影响可读性。
- `OFFLINE` 不显示 Codex 层动画，回到默认 Clawd 动画形象。

核心动画可以通过以下简单元素组合：

- `fillCircle()` 绘制中心点。
- `drawCircle()` 绘制外环。
- 可选 `drawLine()` 或 `fillRect()` 绘制扫描线。
- 局部重绘核心区域、状态点或扫描线。

## 数据与接口

优先继续使用现有状态数据：

- `progressState`
- `progressMsg`
- `/progress?state=...&msg=...`
- `/state`
- USB 串口 `PROGRESS ...`

允许新增一个离线/默认语义，但要保持向后兼容：

- 客户端打开 Codex 或 watcher 启动时推送 `IDLE codex-ready`，设备进入 Codex 待机屏。
- 客户端检测 Codex 离线、watcher 停止或长时间无 Codex 心跳时，设备回到默认 Clawd 动画形象。
- 若新增 `OFFLINE` 为 `/progress` 和 USB `PROGRESS` 可接收状态，它的行为是回到默认 Clawd 动画形象。
- 若不新增公开状态，则需要提供等价命令或内部超时行为实现同样效果。
- 现有 `PLAN / CODE / TEST / DONE / BLOCK` 状态字符串保持不变。

## 错误处理

- 非法状态继续由现有 `setProgress()` 返回失败。
- message 继续只保留安全 ASCII，并限制长度。
- message 为空时显示状态默认短语，例如 `codex-ready`、`active` 或不显示。
- 如果屏幕重绘过程中当前 view 变化，应遵循现有 view 逻辑，不强制抢回屏幕。
- 如果 Codex 心跳超时或客户端离线，设备应退出 Codex 层，回到默认 Clawd 动画形象。

## 验收标准

- 设备开机且 Codex 未打开时显示默认 Clawd 动画形象，而不是 Codex 待机屏。
- Codex 打开且 watcher 在线但无任务时显示 `IDLE` Codex 待机屏。
- Codex 离线、watcher 停止或离线状态被推送后，设备回到默认 Clawd 动画形象。
- `IDLE / PLAN / CODE / TEST / DONE / BLOCK` 都显示 Codex 核心脉冲形象、状态色、英文状态词和阶段条。
- Codex 层不出现 Clawd、眼睛或动物/角色表情语义。
- `DONE` 在实机屏幕上不会被明显误读成 `NONE`。
- `PLAN / CODE / TEST` 有轻量动画反馈。
- `IDLE` 有低亮慢呼吸，`DONE` 和 `BLOCK` 静止或低频动画时状态仍清晰。
- `/progress`、`/state` 和 USB `PROGRESS` 协议保持兼容。
- 不新增图片、字体、框架或网络依赖。
- 固件主程序与 `dist/clawd_mochi/clawd_mochi.ino` 在实现后保持一致。

## 实施注意

- 优先改 `drawProgressView()`、`drawCodexIdleView()`、默认动画回退路径和 `progressTick()` 附近逻辑。
- 保持现有状态颜色函数或只做局部颜色调整。
- 如果新增绘制 helper，应保持小而明确，例如 `drawCodexCore()`、`drawProgressBars()`。
- 先实现静态完整画面，再加局部动画。
- 同步更新 watcher 计划：启动或检测到 Codex 在线时推 `IDLE codex-ready`，离线或停止时触发默认形象回退。
- ESP32 core 未安装完成前，不运行 Arduino 编译。
