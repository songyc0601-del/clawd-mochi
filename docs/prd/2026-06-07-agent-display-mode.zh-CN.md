# 多客户端状态显示模式 PRD

日期：2026-06-07

## Problem Statement

Clawd Mochi 现在可以通过本地 watcher 显示 Codex 工作状态，也可以通过 Claude Code watcher 推送类似状态。但当 Codex 和 Claude Code 同时运行时，两个 watcher 会竞争 `/progress`，导致设备屏幕在不同客户端状态之间互相覆盖，用户无法明确控制当前屏幕应显示哪个客户端。

用户需要一个稳定的多客户端状态显示体验：设备 Web 页面负责选择 Display Mode，本机只运行一个 Unified Agent Watcher，由它观察 Codex 和 Claude Code，并向设备推送唯一最终状态。

## Solution

新增 Web 控制的 Display Mode：`Auto / Codex / Claude`。

- `Auto`：自动从 Codex 和 Claude Code 中选择当前最重要的工作状态；同级状态用最近活动时间打平。
- `Codex`：只允许 Codex 驱动 Work Status Layer；Codex 离线时回到 Default Clawd Layer。
- `Claude`：只允许 Claude Code 驱动 Work Status Layer；Claude Code 离线时回到 Default Clawd Layer。

新增 Unified Agent Watcher 作为唯一支持的多客户端入口。它同时观察 Codex 和 Claude Code session，读取设备 `/state.agentMode`，计算最终输出，并通过 `/progress?source=...` 推送给设备。

设备端扩展协议和显示分支：Codex 使用 Codex Core-Pulse Layer；Claude Code 使用 Claude Code Style Layer；`OFFLINE` 或 `source=none` 时显示 Default Clawd Layer。

## User Stories

1. 作为同时使用 Codex 和 Claude Code 的开发者，我希望只运行一个同步脚本，从而避免两个 watcher 抢占设备屏幕。
2. 作为开发者，我希望在 Web 页面选择 Auto 模式，从而让设备自动显示更重要的客户端状态。
3. 作为开发者，我希望在 Web 页面固定 Codex 模式，从而在 Claude Code 活跃时仍然只看 Codex 状态。
4. 作为开发者，我希望在 Web 页面固定 Claude 模式，从而在 Codex 活跃时仍然只看 Claude Code 状态。
5. 作为开发者，我希望固定模式对应客户端离线时设备回到默认 Clawd 动画，而不是自动切换到另一个客户端。
6. 作为开发者，我希望 Codex 和 Claude Code 使用不同的状态形象，从而能一眼识别当前来源。
7. 作为开发者，我希望 Claude Code 状态层不依赖图片、字体或官方品牌资源，从而保持固件简单且可离线构建。
8. 作为开发者，我希望状态来源是显式协议字段，从而避免旧脚本或手动命令误改当前来源。
9. 作为开发者，我希望旧的 `/progress?state=...&msg=...` 调用继续可用，从而不破坏现有手动调试流程。
10. 作为开发者，我希望不传 `source` 时只更新状态和消息，不改变当前 Progress Source，从而保持兼容行为可预测。
11. 作为开发者，我希望 USB `PROGRESS` 第一版不引入来源参数，从而避免扩大串口协议范围。
12. 作为开发者，我希望设备 `/state` 返回 `agentMode` 和 `progressSource`，从而让 Web 页面和 watcher 能确认当前状态。
13. 作为开发者，我希望 Display Mode 不做断电持久化，从而设备重启后总是回到安全默认的 `AUTO`。
14. 作为开发者，我希望 Web 切换模式后由 watcher 下一次轮询更新屏幕，从而避免 Web 端自行做客户端在线判断。
15. 作为开发者，我希望 Unified Agent Watcher 在状态不变时定期重发心跳，从而避免设备端 120 秒超时误回默认动画。
16. 作为开发者，我希望 Unified Agent Watcher 退出时推送 `OFFLINE agents-offline`，从而让设备回到 Default Clawd Layer。
17. 作为开发者，我希望 `--once` 模式退出时不推送离线状态，从而便于脚本测试和诊断。
18. 作为开发者，我希望 `BLOCK` 和 `TEST` 保留优先级和转发能力，从而后续接入 hook 时不需要重新设计协议。
19. 作为开发者，我希望第一版不自动识别真实 `TEST` 或 `BLOCK`，从而降低误判和实现复杂度。
20. 作为维护者，我希望现有 `codex-watch` 和 `claude-watch` 保留为兼容和排错工具，从而降低迁移风险。

## Implementation Decisions

- Multi-client 仲裁发生在客户端电脑侧，而不是 ESP32 固件侧；该决策记录在 ADR 0001。
- Unified Agent Watcher 是唯一支持的多客户端用户入口；单客户端 watcher 保留但不作为多客户端文档入口。
- HTTP `/progress` 新增可选 `source` 参数，合法值为 `codex`、`claude`、`none`。
- 未传 `source` 时不改变当前 Progress Source，只更新状态和消息。
- `progressSource` 初始值为 `none`。
- 新增 `/agent-mode` 保存运行时 Display Mode，合法值为 `auto`、`codex`、`claude`，内部返回大写 `AUTO / CODEX / CLAUDE`。
- `/state` 保留现有字段，并新增 `progressSource` 和 `agentMode`。
- Web 页面只负责修改 Display Mode，不直接调用 `/progress` 推送工作状态。
- Display Mode 不持久化，设备重启后恢复 `AUTO`。
- 固件显示分支按最终状态绘制：`OFFLINE` 显示 Default Clawd Layer，`source=codex` 显示 Codex Core-Pulse Layer，`source=claude` 显示 Claude Code Style Layer。
- Claude Code Style Layer 使用固件绘图 primitives 实现，不使用官方图片、字体或网络资源。
- Auto Display Mode 的优先级为 `BLOCK > TEST > CODE > PLAN > DONE > IDLE > OFFLINE`；同级按最近活动时间打平；完全相同则 `Codex > Claude Code`。
- `DONE` 高于 `IDLE`，低于活动状态。
- 固定模式严格遵守用户选择，不被另一个在线客户端抢屏。
- Unified Agent Watcher 默认读取设备 `/state.agentMode`；`--mode auto|codex|claude` 仅作为调试覆盖。
- `/state` 读取失败时使用上一次成功读取的模式；首次失败时使用 `AUTO`。
- 长运行 watcher 默认每 60 秒重发当前最终状态，退出时推送 `OFFLINE agents-offline`；`--once` 不推送退出离线状态。

## Testing Decisions

- 优先测试外部行为和协议契约，不测试内部绘图函数细节。
- 使用静态契约测试覆盖主 sketch `clawd_mochi/clawd_mochi.ino` 的固件协议、Web 字段、`progressSource` 和 `agentMode`；历史 dist sketch 副本已删除，不再测试镜像一致性。
- 使用 shell 行为测试覆盖 Unified Agent Watcher 的公开 CLI、状态选择、固定模式、离线输出、心跳和 `--once` 行为。
- 继续保持现有单客户端 watcher 行为测试通过，确保迁移不破坏兼容脚本。
- 固件实现完成后运行 Arduino compile 验证 ESP32-C3 构建。

## Out of Scope

- 不删除现有 `codex-watch` 或 `claude-watch`。
- 不实现 Windows 版 Unified Agent Watcher。
- 不自动识别真实 `TEST` 或 `BLOCK`。
- 不引入图片、字体、官方品牌资源或网络依赖。
- 不持久化 Display Mode。
- 不新增 Codex / Claude Code 以外的客户端。
- 不重写 WiFi、OTA、USB 基础控制协议。

## Further Notes

- PRD 使用 `CONTEXT.md` 中的术语：Default Clawd Layer、Work Status Layer、Codex Core-Pulse Layer、Claude Code Style Layer、Display Mode、Progress Source、Auto Display Mode、Unified Agent Watcher。
- 设计细节见 `docs/superpowers/specs/2026-06-07-agent-display-mode-design.md`。
- 实施计划见 `docs/superpowers/plans/2026-06-07-agent-display-mode.md`。
- 架构决策见 `docs/adr/0001-client-side-agent-status-arbitration.md`。
