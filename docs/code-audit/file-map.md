# File map

本文件描述当前仓库中与审计相关的文件分布。

## 代码入口

| 路径 | 状态 | 说明 |
| --- | --- | --- |
| `clawd_mochi/clawd_mochi.ino` | 当前主 sketch | 唯一固件源码入口；本轮审计文档均以它为准。 |
| `dist/clawd_mochi/clawd_mochi.ino` | 已删除 | 不再保留 dist 固件副本，也不再要求维护 dist 与主 sketch 的镜像一致性。 |

## 审计文档

| 路径 | 说明 |
| --- | --- |
| `docs/code-audit/summary.md` | 当前代码审计摘要、健康度和后续建议。 |
| `docs/code-audit/http-routes.md` | 当前真实注册 HTTP 路由清单。 |
| `docs/code-audit/function-inventory.md` | 当前主 sketch 函数清单。 |
| `docs/code-audit/globals-inventory.md` | 当前全局变量、常量、宏和静态数据清单。 |
| `docs/code-audit/file-map.md` | 当前文件地图。 |

## 主要文档目录

| 路径 | 说明 |
| --- | --- |
| `README.md` | 英文项目说明；本次任务不修改。 |
| `README.zh-CN.md` | 中文项目说明；本次任务不修改。 |
| `docs/ota-update.zh-CN.md` | OTA 使用说明。 |
| `docs/codex-client-status-sync.zh-CN.md` | Codex 状态同步说明。 |
| `docs/claude-code-status-sync.zh-CN.md` | Claude Code 状态同步说明。 |
| `docs/user-manual.zh-CN.md` | 用户手册。 |
| `docs/adr/` | 架构决策记录。 |
| `docs/prd/` | 产品需求文档。 |
| `docs/superpowers/` | 历史归档目录；清理任务不建议修改。 |

## 工具和资产

| 路径 | 说明 |
| --- | --- |
| `tools/` | 状态同步和验证脚本；本次任务不修改。 |
| `models/` | 3D 打印模型文件。 |
| `pics/` | 项目图片素材。 |
| `.superpowers/` | 历史设计 / 头脑风暴归档；清理任务不修改。 |

## 当前三层文件树快照

```text
./.gitignore
./AGENTS.md
./CONTEXT.md
./LICENSE
./README.md
./README.zh-CN.md
./clawd_mochi/clawd_mochi.ino
./docs/adr/0001-client-side-agent-status-arbitration.md
./docs/claude-code-status-sync.zh-CN.md
./docs/code-audit/file-map.md
./docs/code-audit/function-inventory.md
./docs/code-audit/globals-inventory.md
./docs/code-audit/http-routes.md
./docs/code-audit/summary.md
./docs/codex-client-status-sync.zh-CN.md
./docs/offline-install-windows.zh-CN.md
./docs/ota-update.zh-CN.md
./docs/prd/2026-06-07-agent-display-mode.zh-CN.md
./docs/usb-serial-codex.zh-CN.md
./docs/user-manual.zh-CN.md
./docs/wsl-ubuntu-setup.zh-CN.md
./mintty.2026-06-06_10-58-10.png
./models/clawd_3d/clawd_3D_AMS.3mf
./models/clawd_3d/clawd_3D_AMS.stl
./models/clawd_3d/clawd_3D_no_AMS.3mf
./models/clawd_3d/clawd_3D_no_AMS.stl
./models/clawd_3d_squished_eyes/clawd_3D_squished_eyes_AMS.3mf
./models/clawd_3d_squished_eyes/clawd_3D_squished_eyes_AMS.stl
./models/clawd_3d_squished_eyes/clawd_3D_squished_eyes_no_AMS.3mf
./models/clawd_3d_squished_eyes/clawd_3D_squished_eyes_no_AMS.stl
./models/clawd_mochi/clawd_mochi_v1.3mf
./models/clawd_mochi/clawd_mochi_v1.stl
./pics/clawd_3D_3_4.png
./pics/clawd_3D_4_3.png
./pics/clawd_3D_squished_eyes_3_4.png
./pics/clawd_3D_squished_eyes_4_3.png
./pics/clawd_mochi_3_4.jpeg
./pics/clawd_mochi_4_3.jpg
./pics/clawd_mochi_avatar.jpg
./pics/clawd_mochi_banner.png
./pics/clawd_mochi_claude_code.jpeg
./pics/clawd_mochi_logo.png
./pics/clawd_mochi_start.jpeg
./pics/clawd_mochi_webpage.jpeg
./tools/agent-watch.sh
./tools/claude-watch.ps1
./tools/claude-watch.sh
./tools/codex-notify.ps1
./tools/codex-progress.ps1
./tools/codex-stage.ps1
./tools/codex-stage.sh
./tools/codex-watch.ps1
./tools/codex-watch.sh
./tools/test-agent-watch-behavior.sh
./tools/test-codex-status-avatar.sh
./tools/test-codex-watch-behavior.sh
./tools/test-network-page.ps1
./tools/test-web-page.ps1
```
