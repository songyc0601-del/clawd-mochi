# Codex 状态动画形象屏设计

日期：2026-06-06

## 目标

把 Codex 工作状态屏从静态仪表盘改为“Codex 核心脉冲”动画形象。

屏幕应通过一个抽象的发光核心、状态颜色、英文状态词和轻量动画表达当前阶段，避免使用 Clawd、眼睛、动物或外壳角色语义。

## 范围

本设计只覆盖 240×240 TFT 上的 Codex 状态显示效果。

包含：

- `IDLE / PLAN / CODE / TEST / DONE / BLOCK` 六个状态的视觉映射。
- Codex 核心脉冲形象。
- 状态色、英文状态词、短 message、阶段条。
- 轻量动画规则。

不包含：

- watcher 判定逻辑。
- `/progress`、`/state`、USB 串口协议变更。
- WiFi、OTA、Web 控制页面结构变更。
- 新图片、字体、外部资源或复杂帧动画。

## 视觉方向

主视觉是一个位于屏幕中上部的抽象 Codex 核心：

- 中心实心圆表示当前 Codex 活动核心。
- 外层圆环表示思考、执行或验证中的能量场。
- 外圈状态边框使用当前状态色。
- 底部显示大号英文状态词。
- 状态词下方或附近显示短 message，例如 `active`、`planning`、`turn-complete`。
- 阶段条保留 4 段，辅助表达流程进度。

整体风格应偏 Codex、终端、AI 工作状态，而不是角色表情。

## 状态映射

| 状态 | 颜色 | 核心表现 | 阶段条 | message 示例 |
| --- | --- | --- | --- | --- |
| `IDLE` | 低亮灰 / 暗橙 | 核心低亮慢呼吸 | 0/4 | `waiting` |
| `PLAN` | 蓝色 | 外环缓慢扩张，像规划中 | 1/4 | `planning` |
| `CODE` | 橙色 | 核心脉冲更明显，状态点闪烁 | 2/4 | `active` |
| `TEST` | 青绿 / 绿色 | 加一条横向扫描线或更快脉冲 | 3/4 | `verifying` |
| `DONE` | 绿色 | 核心稳定亮起，不再闪烁 | 4/4 | `turn-complete` |
| `BLOCK` | 红色 | 边框和核心红色高亮，可短促闪烁 | 4/4 | `need-input` |

## 布局

推荐坐标分区：

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
- 运行态 `PLAN / CODE / TEST` 可以持续动画。
- `DONE` 使用稳定亮起或一次性完成态。
- `BLOCK` 可以使用低频红色闪烁，但不能影响可读性。

核心动画可以通过以下简单元素组合：

- `fillCircle()` 绘制中心点。
- `drawCircle()` 绘制外环。
- 可选 `drawLine()` 或 `fillRect()` 绘制扫描线。
- 局部重绘核心区域、状态点或扫描线。

## 数据与接口

继续使用现有状态数据：

- `progressState`
- `progressMsg`
- `/progress?state=...&msg=...`
- `/state`
- USB 串口 `PROGRESS ...`

不增加新状态，不改变现有状态字符串，不要求 watcher 修改输出。

## 错误处理

- 非法状态继续由现有 `setProgress()` 返回失败。
- message 继续只保留安全 ASCII，并限制长度。
- message 为空时显示状态默认短语，例如 `waiting`、`active` 或不显示。
- 如果屏幕重绘过程中当前 view 变化，应遵循现有 view 逻辑，不强制抢回屏幕。

## 验收标准

- 每个状态都显示 Codex 核心脉冲形象、状态色、英文状态词和阶段条。
- 屏幕不出现 Clawd、眼睛或动物/角色表情语义。
- `DONE` 在实机屏幕上不会被明显误读成 `NONE`。
- `PLAN / CODE / TEST` 有轻量动画反馈。
- `DONE` 和 `BLOCK` 静止或低频动画时状态仍清晰。
- `/progress`、`/state` 和 USB `PROGRESS` 协议保持兼容。
- 不新增图片、字体、框架或网络依赖。
- 固件主程序与 `dist/clawd_mochi/clawd_mochi.ino` 在实现后保持一致。

## 实施注意

- 优先改 `drawProgressView()`、`drawCodexIdleView()` 和 `progressTick()` 附近逻辑。
- 保持现有状态颜色函数或只做局部颜色调整。
- 如果新增绘制 helper，应保持小而明确，例如 `drawCodexCore()`、`drawProgressBars()`。
- 先实现静态完整画面，再加局部动画。
- ESP32 core 未安装完成前，不运行 Arduino 编译。
