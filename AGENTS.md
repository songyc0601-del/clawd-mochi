# AGENTS.md — Clawd-Mochi

## 项目概述
ESP32-C3 + 1.54" ST7789 桌面状态形象屏。
当前主功能：接收开发工作流状态（PLAN/CODE/TEST/BLOCK/DONE）
并通过表情/动画显示。

## 硬件 & 默认配置
- MCU: ESP32-C3
- 显示: 1.54" ST7789 (240x240)
- AP 模式: SSID `ClaWD-Mochi` / 密码 `clawd1234`
- 本地 Web: http://192.168.4.1
- **绝不修改默认 AP SSID / 密码**

## 代码结构
- 主代码: `clawd_mochi/clawd_mochi.ino`（单文件，约 85KB）
- 工具脚本: `tools/`（Windows PowerShell，**Linux 不要执行**）
- 文档: `docs/`（中英文混合）
- 设计稿存档: `.superpowers/brainstorm/`（不要修改）

## Git 工作流（强制）
- **绝不直接在 main 提交**
- 每个任务：`git checkout -b agent/<task-slug>`
- 完成后 push 分支 + 开 PR 到 main
- 不要自己 merge PR，由用户审核
- 提交信息用 Conventional Commits：
  - `feat:` 新功能
  - `fix:` 修 bug
  - `refactor:` 重构（行为不变）
  - `docs:` 文档
  - `chore:` 杂项
  - `test:` 测试

## 编辑约束
- **不要本地编译**：VPS 没有 arduino-cli 和 ESP32 工具链
- 不要执行 PowerShell 脚本（`tools/*.ps1`）
- 不要引入需要联网编译的新库
- 修改 .ino 时，给出的 diff 应该是局部的，不要全文重写
- 大函数（>100 行）的修改要分段说明改了什么

## 范围控制
- 一个 issue 一个 PR，一个 PR 一件事
- 不要"顺手"做范围外的改动
- 看到代码风格不一致、似乎有 bug 等情况，记在 issue 里报告，不要"顺手修"
- 如果任务描述模糊，**先在 issue 里问清楚再动手**

## 沟通规范
- 用中文回复 issue
- 完成后汇报四件事：
  1. 完成了什么（按需求逐条对照）
  2. 改动了哪些文件（带行数变化）
  3. 跑了什么命令（特别是 git 操作）
  4. 剩余风险 / 需要用户验证的点

## 红线
- 不执行 `rm -rf`、`git push --force` 到 main、`git reset --hard origin/main` 等危险命令
- 不修改 `.git/`、`.gitignore` 之外的隐藏目录
- 不修改 LICENSE
- 不动 `docs/superpowers/`、`.superpowers/` 目录
- 遇到不确定的事，停下来问，不要自由发挥
