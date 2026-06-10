# 仓库文件地图

范围：仓库根目录，排除 `.git`。运行必需表示当前固件/用户交付直接依赖；开发辅助表示文档、脚本、设计资料或验证工具；疑似废弃表示当前代码或文档没有明显引用，或已有更新版本替代。

## 文件树

```text
.
├── .gitignore
├── .superpowers/brainstorm/web-ui-20260605/
│   ├── content/*.html
│   └── state/{launcher.pid,server-stopped}
├── AGENTS.md
├── CONTEXT.md
├── LICENSE
├── README.md
├── README.zh-CN.md
├── clawd_mochi/clawd_mochi.ino
├── dist/clawd_mochi/clawd_mochi.ino
├── docs/
│   ├── adr/0001-client-side-agent-status-arbitration.md
│   ├── claude-code-status-sync.zh-CN.md
│   ├── codex-client-status-sync.zh-CN.md
│   ├── offline-install-windows.zh-CN.md
│   ├── ota-update.zh-CN.md
│   ├── prd/2026-06-07-agent-display-mode.zh-CN.md
│   ├── prd/issues/*.md
│   ├── superpowers/plans/*.md
│   ├── superpowers/specs/*.md
│   ├── usb-serial-codex.zh-CN.md
│   ├── user-manual.zh-CN.md
│   └── wsl-ubuntu-setup.zh-CN.md
├── mintty.2026-06-06_10-58-10.png
├── models/
│   ├── clawd_3d/*.{3mf,stl}
│   ├── clawd_3d_squished_eyes/*.{3mf,stl}
│   └── clawd_mochi/*.{3mf,stl}
├── pics/*.{jpg,jpeg,png}
└── tools/*.{sh,ps1}
```

## 顶层文件

| 文件 | 用途 | 分类 | 依据 |
|---|---|---|---|
| `.gitignore` | Git 忽略规则 | 开发辅助 | Git 配置文件 |
| `AGENTS.md` | 仓库级 agent 工作约束 | 开发辅助 | 当前任务明确要求读取；最近提交 `e25679e 2026-06-10 docs: 重写 AGENTS.md 适配 Multica 工作流` |
| `CONTEXT.md` | 项目背景上下文 | 开发辅助 | 被 `git ls-files` 跟踪；文档性文件 |
| `LICENSE` | 许可证文本 | 运行必需 | 项目分发必备元数据 |
| `README.md` | 英文/主 README | 运行必需 | 用户入口文档 |
| `README.zh-CN.md` | 中文 README | 运行必需 | 用户入口文档 |
| `mintty.2026-06-06_10-58-10.png` | 终端截图/记录图片 | 疑似废弃 | 文件名带时间戳；`rg "mintty.2026-06-06_10-58-10.png"` 仅命中文件自身列表，无代码/文档引用 |

## 固件代码

| 文件 | 用途 | 分类 | 依据 |
|---|---|---|---|
| `clawd_mochi/clawd_mochi.ino` | 主 Arduino sketch，包含显示、WiFi、HTTP、串口、OTA 逻辑 | 运行必需 | `AGENTS.md` 指定主代码；本次审计对象，1992 行 |
| `dist/clawd_mochi/clawd_mochi.ino` | 发布/拷贝用 sketch 副本 | 疑似废弃 | 与主文件同名且不在 Arduino 默认 sketch 目录；`AGENTS.md` 指定主代码为 `clawd_mochi/clawd_mochi.ino`，未提到 `dist/`；最近相关提交 `1938073 2026-06-07 实现多客户端状态显示模式` |

## 文档

| 文件 | 用途 | 分类 | 依据 |
|---|---|---|---|
| `docs/adr/0001-client-side-agent-status-arbitration.md` | 多客户端状态仲裁设计决策 | 开发辅助 | ADR 文档路径 |
| `docs/claude-code-status-sync.zh-CN.md` | Claude Code 状态同步说明 | 运行必需 | 对应 `tools/claude-watch.*` 使用文档 |
| `docs/codex-client-status-sync.zh-CN.md` | Codex 状态同步说明 | 运行必需 | 对应 `tools/codex-watch.*`/`codex-stage.*` 使用文档 |
| `docs/offline-install-windows.zh-CN.md` | Windows 离线安装说明 | 运行必需 | 用户安装文档 |
| `docs/ota-update.zh-CN.md` | OTA 更新说明 | 运行必需 | 固件包含 `/ota` 路由（L1966-L1967） |
| `docs/prd/2026-06-07-agent-display-mode.zh-CN.md` | agent 显示模式 PRD | 开发辅助 | 需求归档 |
| `docs/prd/issues/*.md` | PRD 拆分 issue 记录 | 开发辅助 | 规划/追踪文档 |
| `docs/superpowers/plans/*.md` | Superpowers 计划文档 | 开发辅助 | 设计/实现计划归档；AGENTS.md 明确不要修改 `docs/superpowers/` |
| `docs/superpowers/specs/*.md` | Superpowers 规格文档 | 开发辅助 | 设计规格归档；AGENTS.md 明确不要修改 |
| `docs/usb-serial-codex.zh-CN.md` | USB 串口 Codex 协议说明 | 运行必需 | 固件有 `handleSerialCommand()`/`handleSerial()`（L1842-L1899） |
| `docs/user-manual.zh-CN.md` | 用户手册 | 运行必需 | 用户入口文档 |
| `docs/wsl-ubuntu-setup.zh-CN.md` | WSL/Ubuntu 设置说明 | 开发辅助 | 环境配置文档 |

## 工具脚本

| 文件 | 用途 | 分类 | 依据 |
|---|---|---|---|
| `tools/agent-watch.sh` | Linux/macOS agent 状态 watcher | 运行必需 | 与 `/progress`、`/agent-mode` 路由配套 |
| `tools/claude-watch.sh` | Linux/macOS Claude watcher | 运行必需 | 与 `/progress?source=claude` 配套 |
| `tools/codex-watch.sh` | Linux/macOS Codex watcher | 运行必需 | 与 `/progress?source=codex` 配套 |
| `tools/codex-stage.sh` | Linux/macOS 手动阶段上报 | 运行必需 | 与 `/progress` 路由配套 |
| `tools/test-agent-watch-behavior.sh` | watcher 行为测试脚本 | 开发辅助 | `test-*` 命名，仅用于验证 |
| `tools/test-codex-status-avatar.sh` | Codex 状态头像测试脚本 | 开发辅助 | `test-*` 命名，仅用于验证 |
| `tools/test-codex-watch-behavior.sh` | Codex watcher 行为测试脚本 | 开发辅助 | `test-*` 命名，仅用于验证 |
| `tools/claude-watch.ps1` | Windows Claude watcher | 运行必需 | Windows 配套脚本；AGENTS.md 要求 Linux 不执行 |
| `tools/codex-watch.ps1` | Windows Codex watcher | 运行必需 | Windows 配套脚本；AGENTS.md 要求 Linux 不执行 |
| `tools/codex-progress.ps1` | Windows 进度上报辅助 | 运行必需 | Windows 配套脚本 |
| `tools/codex-stage.ps1` | Windows 手动阶段上报 | 运行必需 | Windows 配套脚本 |
| `tools/codex-notify.ps1` | Windows 通知/状态辅助 | 开发辅助 | 与 Codex 工作流配套 |
| `tools/test-network-page.ps1` | Windows 网络页测试 | 开发辅助 | `test-*` 命名 |
| `tools/test-web-page.ps1` | Windows Web 页测试 | 开发辅助 | `test-*` 命名 |

## 图片与模型资源

| 路径 | 用途 | 分类 | 依据 |
|---|---|---|---|
| `pics/clawd_mochi_*.jpg/jpeg/png` | README/文档展示图片、网页截图、logo/avatar | 运行必需 | 产品说明和页面展示资源 |
| `pics/clawd_3D_*.png`、`pics/clawd_3D_squished_eyes_*.png` | 3D 外观展示图片 | 运行必需 | 产品外观文档资源 |
| `models/clawd_mochi/*.{3mf,stl}` | 机壳/主体模型文件 | 运行必需 | 硬件项目可打印模型 |
| `models/clawd_3d/*.{3mf,stl}` | Clawd 3D 普通眼模型 | 运行必需 | 硬件项目可打印模型 |
| `models/clawd_3d_squished_eyes/*.{3mf,stl}` | Clawd 3D 眯眼模型 | 运行必需 | 硬件项目可打印模型 |

## 疑似废弃汇总

| 文件/目录 | 依据 |
|---|---|
| `dist/clawd_mochi/clawd_mochi.ino` | 主入口已由 `AGENTS.md` 固定为 `clawd_mochi/clawd_mochi.ino`；`dist/` 副本容易与主文件漂移，未见文档说明如何生成 |
| `.superpowers/brainstorm/web-ui-20260605/state/launcher.pid` | 运行时状态文件，被 Git 跟踪；文件名语义为进程状态，不应是固件运行必需 |
| `.superpowers/brainstorm/web-ui-20260605/state/server-stopped` | 同上，是设计原型服务的停止标记 |
| `mintty.2026-06-06_10-58-10.png` | 截图文件名带时间戳；未见代码/文档引用 |

本清单不建议在本 issue 中删除任何文件；删除前应单独确认 `dist/` 是否仍作为发布包来源、`.superpowers` 是否需要保留完整设计历史。
