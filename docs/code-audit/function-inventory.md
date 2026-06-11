# Function inventory

审计对象：`clawd_mochi/clawd_mochi.ino`

当前主 sketch 共 76 个函数。

| 函数 | 职责 | 主要调用方 / 入口 |
| --- | --- | --- |
| `speedMs(int ms)` | 根据 `animSpeed` 调整动画延时。 | 眼睛动画、Logo 动画 |
| `hexToRgb565(String hex)` | 将 `#RRGGBB` 字符串转为 RGB565。 | 保留的颜色工具函数 |
| `setBacklight(bool on)` | 更新背光状态并写 GPIO。 | `setup()`、`routeBacklight()`、串口 `BL` |
| `initColours()` | 初始化主题色和背景色。 | `setup()` |
| `drawLogoFilled(uint16_t bg, uint16_t fg)` | 绘制填充版 Logo。 | `animLogoReveal()` |
| `eyeLX(int16_t ox)` | 计算左眼 X 坐标。 | 眼睛绘制函数 |
| `eyeRX(int16_t ox)` | 计算右眼 X 坐标。 | 眼睛绘制函数 |
| `eyeY()` | 计算眼睛 Y 坐标。 | 眼睛绘制函数 |
| `eyeCY()` | 计算眼睛中心 Y 坐标。 | 眼睛绘制函数 |
| `drawNormalEyes(int16_t ox, bool blink)` | 绘制正常方眼，可眨眼。 | `drawDefaultClawdView()`、`animNormalEyes()` |
| `drawChevron(...)` | 绘制 V 形眼部线条。 | `drawSquishEyes()`、`drawCompanionEyes()` |
| `drawSquishEyes(bool closed)` | 绘制眯眼 / 闭眼状态。 | `animSquishEyes()` |
| `drawCompanionEyes(uint8_t expr, int16_t ox)` | 按表情枚举绘制陪伴眼。 | `setCompanionExpr()` |
| `setCompanionExpr(String name)` | 解析并设置陪伴表情。 | `routeExpr()` |
| `drawCodeView()` | 绘制 Claude Code 文字屏。 | 当前保留但无 HTTP/串口入口直接调用 |
| `progressColor(const String& state)` | 根据进度状态选择颜色。 | 进度绘制函数 |
| `isCodexLayerState(const String& state)` | 判断状态是否属于 Codex 进度层。 | `isProgressState()`、tick/timeout |
| `isProgressState(const String& state)` | 校验进度状态。 | `setProgress()` |
| `isProgressSource(const String& source)` | 校验进度来源字符串。 | 当前保留工具函数 |
| `setAgentMode(String mode)` | 设置显示模式。 | `routeAgentMode()` |
| `progressStage(const String& state)` | 将状态映射为 0-4 阶段。 | 进度绘制函数 |
| `progressDefaultMessage(const String& state)` | 给状态生成默认短消息。 | `setProgress()`、进度绘制函数 |
| `cleanAscii(String text, uint8_t maxLen)` | 清理消息文本为 ASCII 并限长。 | `setProgress()` |
| `drawProgressBars(uint8_t stage, uint16_t col)` | 绘制四段进度条。 | Codex/Claude 进度视图 |
| `drawCodexCore(uint16_t col, uint8_t pulse)` | 绘制 Codex 核心脉冲层。 | `drawCodexProgressView()`、`progressTick()` |
| `drawCodexScanLine(uint16_t col, uint8_t pulse)` | 绘制测试态扫描线。 | Codex 进度视图 / tick |
| `drawClaudeCodeStyleLayer(uint16_t col, uint8_t pulse)` | 绘制 Claude 风格进度层。 | `drawClaudeProgressView()`、`progressTick()` |
| `drawCodexProgressView()` | 绘制 Codex 来源的进度页。 | `drawProgressView()` |
| `drawClaudeProgressView()` | 绘制 Claude 来源的进度页。 | `drawProgressView()` |
| `drawDefaultClawdView()` | 回到默认正常眼睛视图。 | `setup()`、`drawProgressView()` |
| `drawProgressView()` | 根据状态和来源选择进度绘制路径。 | `setProgress()`、`drawCodexIdleView()` |
| `drawCodexIdleView()` | 兼容入口，转调 `drawProgressView()`。 | 当前保留工具函数 |
| `setProgress(String state, String msg)` | 校验并更新进度状态、消息、动画计时。 | `/progress`、串口 `PROGRESS`、timeout |
| `progressTick()` | 驱动进度视图的周期动画。 | `loop()` |
| `checkCodexOfflineTimeout()` | Codex 进度超时后切回离线。 | `loop()` |
| `termClear()` | 清空终端缓冲。 | 当前保留终端工具函数 |
| `termDrawHeader()` | 绘制终端标题栏。 | `termFullRedraw()` |
| `termDrawPrefix(int16_t yy)` | 绘制终端提示符。 | 终端绘制函数 |
| `termDrawLine(uint8_t r)` | 绘制终端单行。 | `termFullRedraw()`、`termAddChar()` |
| `termDrawLastChar()` | 绘制最新输入字符和光标。 | `termAddChar()` |
| `termDrawBackspace()` | 处理退格后的局部重绘。 | `termAddChar()` |
| `termFullRedraw()` | 重绘整个终端。 | `termScroll()` |
| `termScroll()` | 终端缓冲上滚。 | `termAddChar()` |
| `termAddChar(char c)` | 处理终端字符输入。 | 当前保留终端工具函数 |
| `animNormalEyes()` | 播放正常眼睛左右移动和眨眼动画。 | `showNormal()` |
| `animSquishEyes()` | 播放眯眼动画。 | 当前保留动画函数 |
| `animLogoReveal()` | 开机 Logo 线条 reveal 动画。 | `setup()` |
| `routeRoot()` | 返回首页。 | `GET /` |
| `connectSavedWifi()` | 使用保存的 WiFi 配置发起 STA 连接。 | `setup()`、`routeWifiConnect()` |
| `startSetupAp()` | 启动 AP+STA 模式和固定 AP IP。 | `setup()`、`restartSetupAp()` |
| `restartSetupAp()` | 重启 AP。 | `routeWifiClear()` |
| `routeNetworkPage()` | 返回网络配置页。 | `GET /network` |
| `routeWifiScan()` | 扫描 WiFi 并返回 JSON。 | `GET /wifi/scan` |
| `routeWifiConnect()` | 保存 WiFi 配置并连接。 | `POST /wifi/connect` |
| `routeWifiClear()` | 清除 WiFi 配置并重启 AP。 | `POST /wifi/clear` |
| `drawOtaStatus(const char*, const char*, uint16_t)` | 在屏幕上显示 OTA 状态。 | OTA 上传流程 |
| `routeOtaPage()` | 返回 OTA 页面。 | `GET /ota` |
| `routeOtaResult()` | 返回 OTA 最终结果，成功则重启。 | `POST /ota` |
| `handleOtaUpload()` | 处理 OTA 上传分块。 | `POST /ota` upload handler |
| `markAction()` | 记录最近操作时间。 | 命令、背光、进度、表情等入口 |
| `showNormal()` | 切回正常眼睛并播放动画。 | `runNamedCommand()` |
| `runNamedCommand(String name)` | 命令分发；当前只支持 `normal` / `w`。 | `/cmd`、串口 `CMD` |
| `routeCmd()` | HTTP 命令入口。 | `GET /cmd` |
| `routeBacklight()` | HTTP 背光入口。 | `GET /backlight` |
| `routeProgress()` | HTTP 进度入口。 | `GET /progress` |
| `routeAgentMode()` | HTTP agent 模式入口。 | `GET /agent-mode` |
| `routeExpr()` | HTTP 表情入口。 | `GET /expr` |
| `rgb565ToHex(uint16_t c)` | RGB565 转十六进制字符串。 | 当前保留工具函数 |
| `jsonEscape(String s)` | JSON 字符串转义。 | 状态和 WiFi JSON |
| `stateJson()` | 组装设备状态 JSON。 | `/state`、串口 `STATE` |
| `routeState()` | HTTP 状态入口。 | `GET /state` |
| `handleSerialCommand(String line)` | 解析串口命令。 | `handleSerial()` |
| `handleSerial()` | 读取串口行并输出响应。 | `loop()` |
| `routeNotFound()` | 返回 404。 | `server.onNotFound(...)` |
| `setup()` | 初始化串口、背光、屏幕、WiFi、路由和默认视图。 | Arduino runtime |
| `loop()` | 处理 HTTP、串口、离线超时和进度动画。 | Arduino runtime |

## 已删除的旧函数

旧版命令显示函数、idle 闲置动画函数，以及旧版绘图 / 终端 / 速度相关 HTTP handler 已从源码中移除；当前只以本文件函数表为准。
