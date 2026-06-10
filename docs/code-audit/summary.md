# clawd_mochi.ino 代码盘点总结

## 当前规模

- 主 sketch：`clawd_mochi/clawd_mochi.ino`，1992 行。
- 函数：86 个（含 `setup()`/`loop()` 与 4 个 `inline` 工具），详见 `function-inventory.md`。
- 全局状态/常量：集中在文件前 120 行，另有 5 份 HTML/PROGMEM 大字符串与 2 组 logo PROGMEM 表，详见 `globals-inventory.md`。
- HTTP 路由：已注册 13 条 `server.on(...)` 路由 + 1 个 `onNotFound`，注册点在 L1961-L1974。
- 仓库：主固件、文档、工具脚本、图片、3D 模型、设计归档混放；详见 `file-map.md`。

## 健康度评估

整体可运行路径清晰：启动、AP/STA、OTA、状态上报、进度屏、串口命令都集中在单文件内，路由注册也集中在 `setup()`。但历史功能残留比较明显，尤其是旧版 Web 控制页、画板、终端、速度/待机开关等代码与当前注册路由不一致。

主要风险不是语法层面，而是“可达性漂移”：部分函数、全局变量和前端路径仍存在，但入口已断，后续维护者容易误判这些功能仍在线。

## 推荐整理优先级 Top 5

1. 补齐或删除未注册 Web 路由：`routeChar`、`routeSpeed`、`routeRedraw`、`routeCanvas`、`routeDrawClear`、`routeDrawStroke`、`routeIdle` 均未在 L1961-L1974 注册；对应前端引用见 `http-routes.md`。
2. 清理旧 HTML 大字符串：`INDEX_HTML` L799 与 `INDEX_HTML_CLEAN` L1308 未被 `server.send*_P` 引用，当前首页使用 `INDEX_HTML_LITE` L1407。
3. 明确闲置动画策略：`idleTick()` L383-L403 未被 `loop()` L1987-L1992 调用，导致 `idleEnabled` L81、`lastIdleMs` L84 基本失效。
4. 收敛命令分发：`runNamedCommand()` L1646-L1651 只接受 `normal/w`，但旧前端仍尝试 `s/d/q` 等路径，`showSquish()` L1630-L1635 与 `showCodeTerminal()` L1637-L1644 因此不可达。
5. 确认发布副本策略：`dist/clawd_mochi/clawd_mochi.ino` 是主 sketch 的副本，但 `AGENTS.md` 指定主代码为 `clawd_mochi/clawd_mochi.ino`；如果继续保留，需要说明生成/同步流程，否则容易漂移。

## 本次审计产物

- `function-inventory.md`：函数范围、职责、调用方、未调用函数候选。
- `globals-inventory.md`：全局变量/常量、读写位置、失效变量候选。
- `http-routes.md`：HTTP 路由、请求格式、未注册处理函数。
- `file-map.md`：仓库文件树、用途分类、疑似废弃依据。
- `summary.md`：本页总结。

本次只做文档盘点，不修改 `.ino`、README、脚本或其他已有项目文件。
