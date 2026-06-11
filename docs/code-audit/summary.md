# Code audit summary

审计对象：`clawd_mochi/clawd_mochi.ino`

审计日期：2026-06-11

## 当前快照

| 项目 | 当前状态 |
| --- | --- |
| 主 sketch | `clawd_mochi/clawd_mochi.ino` |
| 行数 | 1252 行 |
| 函数数量 | 76 个 |
| HTML 入口 | 只使用 `INDEX_HTML_LITE` |
| HTTP 路由 | 13 条 `server.on(...)` 注册，加 1 个 `server.onNotFound(...)` |
| 命令入口 | `runNamedCommand()` 只支持 `normal` 和 `w` |
| dist 固件副本 | `dist/clawd_mochi/clawd_mochi.ino` 已删除 |

## 全局变量 / 常量概况

- 硬件与显示常量集中在文件顶部：TFT 引脚、显示尺寸、眼睛几何参数、视图枚举、表情枚举、终端布局、Logo 数据尺寸。
- 运行时状态仍集中在少量全局变量：当前视图、忙碌状态、背光状态、动画速度、表情、进度状态、进度来源、agent 模式、串口输入缓冲、终端文本缓冲。
- Web 页面常量为 `INDEX_HTML_LITE`、`OTA_HTML`、`NETWORK_HTML`。
- 当前源码中没有旧版首页大字符串。
- 当前源码中没有旧版 idle 闲置状态变量。

## 已完成的清理项

- 已删除未使用的历史首页大字符串，只保留轻量首页 `INDEX_HTML_LITE`。
- 已删除未注册的旧 Web 路由处理函数。
- 已删除不可达的旧命令显示函数。
- 已删除失效的 idle 闲置动画状态与函数。
- 已删除 `dist/clawd_mochi/clawd_mochi.ino` 固件副本，不再需要维护 dist 镜像一致性。
- 审计文档已改为以当前主 sketch 为唯一代码事实来源。

## 当前健康度评估

当前主 sketch 的结构比旧审计时更收敛：Web 路由集中在 `setup()` 注册，首页按钮只调用仍存在的接口，命令分发范围明确，dist 副本漂移风险已消除。

仍需注意的是，`clawd_mochi.ino` 依然是单文件 sketch，Logo 数据、HTML 字符串、Web 路由、显示逻辑和串口协议都在同一文件中。对 Arduino 单文件项目这是可接受的，但后续修改应继续保持小步、局部、可审计。

## 后续建议优先级

1. 保持现状：新增或删除 HTTP 接口时，同步更新 `http-routes.md`。
2. 若后续继续缩减体积，优先评估 `INDEX_HTML_LITE`、`NETWORK_HTML`、`OTA_HTML` 这类内嵌页面，而不是动显示核心逻辑。
3. 若新增状态或协议字段，同步更新 `globals-inventory.md` 和相关用户文档。
4. 不恢复 dist sketch 副本；发布产物应由明确脚本或人工导出流程生成。
