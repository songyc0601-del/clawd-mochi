# clawd_mochi.ino 全局变量清单

文件：`clawd_mochi/clawd_mochi.ino`。涵盖文件级 `static const` 数组、`const` 常量、可变全局对象与状态变量。`#define` 宏未单列（如 `TFT_CS`/`EYE_W`/`TERM_COLS`/`PROGRESS_*` 整型枚举值等），仅在被特别引用时提及。

## 显示与外设

| 行号 | 名称 | 类型 | 初值 | 写入处 | 读取处 |
|---|---|---|---|---|---|
| 34 | `tft` | `Adafruit_ST7789` | 由构造器绑定 `TFT_CS/DC/RST` | `setup` L1914–1916（`init/setSPISpeed/setRotation`）| 几乎所有绘图函数 |

## WiFi / 网络

| 行号 | 名称 | 类型 | 初值 | 写入处 | 读取处 |
|---|---|---|---|---|---|
| 37 | `AP_SSID` | `const char*` | `"ClaWD-Mochi"` | 仅初始化 | `startSetupAp` L1514 |
| 38 | `AP_PASS` | `const char*` | `"clawd1234"` | 仅初始化 | `startSetupAp` L1514 |
| 39 | `AP_IP` | `const IPAddress` | `192.168.4.1` | 仅初始化 | `startSetupAp` L1515 |
| 40 | `AP_GATEWAY` | `const IPAddress` | `192.168.4.1` | 仅初始化 | `startSetupAp` L1515 |
| 41 | `AP_SUBNET` | `const IPAddress` | `255.255.255.0` | 仅初始化 | `startSetupAp` L1515 |
| 42 | `server` | `WebServer(80)` | 端口 80 | `setup` 中 `server.on/begin` | `animLogoReveal` L789、`loop` L1988、各路由内部 `server.arg/send`、`stateJson` |
| 43 | `wifiPrefs` | `Preferences` | 默认 | `setup` L1931（`begin`）、`routeWifiConnect` L1549–1550（`putString`）、`routeWifiClear` L1557（`clear`）| 同上 + `setup` L1932–1933（`getString`）|
| 44 | `savedWifiSsid` | `String` | `""` | `setup` L1932、`routeWifiConnect` L1547、`routeWifiClear` L1558 | `connectSavedWifi` L1508–1509、`setup` L1936 |
| 45 | `savedWifiPassword` | `String` | `""` | `setup` L1933、`routeWifiConnect` L1548、`routeWifiClear` L1559 | `connectSavedWifi` L1509 |

## 颜色

| 行号 | 名称 | 类型 | 初值 | 写入处 | 读取处 |
|---|---|---|---|---|---|
| 59 | `C_ORANGE` | `uint16_t` | 0（运行时由 `initColours` 设置）| `initColours` L252 | 大量绘图函数（含 `drawCodeView`、`drawCodexProgressView`、`drawProgressBars`、`handleOtaUpload`、`setup` 启动屏等）|
| 59 | `C_DARKBG` | `uint16_t` | 0 | `initColours` L253 | `drawCodeView`、`drawCodexCore`、`drawClaudeCodeStyleLayer`、`drawCodexProgressView`/`drawClaudeProgressView`、`termFullRedraw`/`termDrawHeader`/`termDrawLine`/`termDrawLastChar`/`termDrawBackspace`、`drawOtaStatus`、`setup` |
| 59 | `C_MUTED` | `uint16_t` | 0 | `initColours` L254 | `progressColor` 兜底 L425、`drawCodexProgressView` L532、`drawClaudeProgressView` L564、`setup` 文案 |
| 59 | `C_GREEN` | `uint16_t` | 0 | `initColours` L255 | `termDrawPrefix` L665、`termDrawLine` L682、`termDrawLastChar` L697、`termDrawBackspace` L706、`handleOtaUpload` REBOOTING L1607 |

## 视图与陪伴状态

| 行号 | 名称 | 类型 | 初值 | 写入处 | 读取处 |
|---|---|---|---|---|---|
| 77 | `currentView` | `uint8_t` | `VIEW_EYES_NORMAL` | `setCompanionExpr` L378、`drawCodexProgressView` L524、`drawClaudeProgressView` L556、`drawDefaultClawdView` L586、`showNormal` L1626、`showSquish` L1632、`showCodeTerminal` L1639、`routeCanvas` L1697、`routeDrawClear` L1706、`routeDrawStroke` L1718 | `idleTick` L385–386/390/394/398、`progressTick` L624、`routeRedraw` L1684、`stateJson` L1820 |
| 78 | `busy` | `bool` | `false` | `animNormalEyes` L759/766、`animSquishEyes` L770/776、`animLogoReveal` L780/793、`drawOtaStatus` L1567、`routeOtaResult` L1591、`handleOtaUpload` L1610/1615 | `idleTick` L384、`stateJson` L1821 |
| 79 | `backlightOn` | `bool` | `true` | `setBacklight` L246 | `stateJson` L1822 |
| 80 | `animSpeed` | `uint8_t` | `1` | `routeSpeed` L1673（**该路由未注册**）| `speedMs` L233–234 |
| 81 | `idleEnabled` | `bool` | `true` | `routeIdle` L1799（**该路由未注册**）| `idleTick` L384（**该函数未被 `loop()` 调用**）|
| 82 | `companionExpr` | `uint8_t` | `EXPR_FOCUS` | `setCompanionExpr` L374–375 | `idleTick` L391/393、`drawCompanionEyes` 形参、`stateJson` L1827 |
| 83 | `lastActionMs` | `uint32_t` | `0` | `markAction` L1620 | `idleTick` L388 |
| 84 | `lastIdleMs` | `uint32_t` | `0` | `idleTick` L389 | `idleTick` L388 |
| 85 | `lastProgressBlinkMs` | `uint32_t` | `0` | `setProgress` L617、`progressTick` L627 | `progressTick` L626 |
| 86 | `progressBlinkOn` | `bool` | `true` | `setProgress` L615、`progressTick` L629 | （未在任何绘图分支中读取，仅自我翻转）|

## 背景颜色

| 行号 | 名称 | 类型 | 初值 | 写入处 | 读取处 |
|---|---|---|---|---|---|
| 88 | `animBgColor` | `uint16_t` | `0` → `initColours` 设为 `C_ORANGE` | `initColours` L256、`routeRedraw` L1681/1682（**未注册**）、`routeDrawClear` L1705（**未注册**）| `drawNormalEyes` L291、`drawSquishEyes` L316、`drawCompanionEyes` L332/364–365、`animLogoReveal` L781/791、`setup` L1920 |
| 89 | `drawBgColor` | `uint16_t` | `0` → `initColours` 设为 `C_ORANGE` | `initColours` L257、`routeRedraw` L1682（**未注册**）、`routeDrawClear` L1704（**未注册**）| `routeCanvas` L1697（**未注册**）、`routeDrawClear` L1707（**未注册**）、`routeRedraw` L1688（**未注册**）|

## 进度状态文字常量

| 行号 | 名称 | 类型 | 初值 | 写入处 | 读取处 |
|---|---|---|---|---|---|
| 90 | `PROGRESS_OFFLINE` | `const String` | `"OFFLINE"` | 仅初始化 | `progressColor`、`isProgressState`、`progressDefaultMessage`、`drawProgressView` L592、`setProgress` L618、`routeProgress` L1773、`handleSerialCommand` L1875、`setup` L1978 |
| 91 | `PROGRESS_IDLE` | `const String` | `"IDLE"` | 仅初始化 | `progressColor`、`isCodexLayerState`、`progressDefaultMessage` |
| 92 | `PROGRESS_PLAN` | `const String` | `"PLAN"` | 仅初始化 | `progressColor`、`isCodexLayerState`、`progressStage`、`progressDefaultMessage` |
| 93 | `PROGRESS_CODE` | `const String` | `"CODE"` | 仅初始化 | 同上 |
| 94 | `PROGRESS_TEST` | `const String` | `"TEST"` | 仅初始化 | `drawCodexProgressView` L538、`progressTick` L636，及 `progressColor`/`progressStage`/`progressDefaultMessage`/`isCodexLayerState` |
| 95 | `PROGRESS_DONE` | `const String` | `"DONE"` | 仅初始化 | `drawClaudeProgressView` L568、`progressTick` L628，及色/阶段/默认消息 |
| 96 | `PROGRESS_BLOCK` | `const String` | `"BLOCK"` | 仅初始化 | `drawCodexProgressView` L540、`drawClaudeProgressView` L569，及色/阶段/默认消息 |

## 进度运行时状态

| 行号 | 名称 | 类型 | 初值 | 写入处 | 读取处 |
|---|---|---|---|---|---|
| 98 | `progressState` | `String` | `PROGRESS_OFFLINE` | `setProgress` L612、`setup` L1978 | `drawCodexProgressView`/`drawClaudeProgressView` L525/527/538/540 等、`drawProgressView`、`progressTick`、`checkCodexOfflineTimeout`、`stateJson` |
| 99 | `progressMsg` | `String` | `""` | `setProgress` L613–614、`setup` L1979 | `drawCodexProgressView` L527、`drawClaudeProgressView` L559、`stateJson` L1824 |
| 100 | `progressSource` | `String` | `"none"` | `routeProgress` L1772 | `drawProgressView` L597、`progressTick` L632、`stateJson` L1825 |
| 101 | `agentMode` | `String` | `"AUTO"` | `setAgentMode` L445 | `stateJson` L1826 |
| 102 | `progressPulsePhase` | `uint8_t` | `0` | `setProgress` L616、`progressTick` L630 | `drawCodexProgressView` L537/538、`drawClaudeProgressView` L568、`progressTick` L633–636 |
| 103 | `lastCodexProgressMs` | `uint32_t` | `0` | `setProgress` L618 | `checkCodexOfflineTimeout` L641–642 |
| 105 | `CODEX_OFFLINE_TIMEOUT_MS` | `const uint32_t` | `120000UL` | 仅初始化 | `checkCodexOfflineTimeout` L642 |
| 106 | `serialLine` | `String` | `""` | `handleSerial` L1890/1892/1894 | `handleSerial` L1889 |

## 终端缓冲

宏 `TERM_COLS`/`TERM_ROWS`/`TERM_CHAR_W`/`TERM_CHAR_H`/`TERM_PAD_X`/`TERM_PAD_Y`/`PREFIX_PX`（L109–114, L670）被终端绘制函数使用。

| 行号 | 名称 | 类型 | 初值 | 写入处 | 读取处 |
|---|---|---|---|---|---|
| 116 | `termMode` | `bool` | `false` | `setCompanionExpr` L377、`drawCodeView` L406、`drawDefaultClawdView` L585、`drawProgressView` L591、`drawOtaStatus` L1568、`showNormal` L1625、`showSquish` L1631、`showCodeTerminal` L1641、`routeDrawClear` L1706 | `idleTick` L384、`routeChar` L1665（**未注册**）|
| 117 | `termLines[TERM_ROWS]` | `String[8]` | `""` ×8 | `termClear` L652、`termScroll` L720–721、`termAddChar` L738/748 | `termDrawLine` L679、`termAddChar` L738/748 |
| 118 | `termRow` | `uint8_t` | `0` | `termClear` L653、`termScroll` L722、`termAddChar` L732/743 | `termDrawLine` L676/680、`termAddChar` L730/733/734/738/743/747/748 |
| 119 | `termCol` | `uint8_t` | `0` | `termClear` L653、`termAddChar` L732/737/743/749 | `termDrawLine` L681、`termDrawLastChar` L687–697、`termDrawBackspace` L702/706、`termAddChar` |

## Logo PROGMEM 表

| 行号 | 名称 | 类型 | 初值 | 写入处 | 读取处 |
|---|---|---|---|---|---|
| 125–181 | `LOGO_TRIS` + `LOGO_TRI_COUNT(=162)` | `static const int16_t[162][6] PROGMEM` | 字面量 | 只读 | `drawLogoFilled` L266–271 |
| 183–226 | `LOGO_SEGS` + `LOGO_SEG_COUNT(=162)` | `static const int16_t[162][4] PROGMEM` | 字面量 | 只读 | `animLogoReveal` L782–786 |

## 大字符串（HTML/PROGMEM）

| 行号 | 名称 | 类型 | 用途 | 是否使用 |
|---|---|---|---|---|
| 799 | `INDEX_HTML` | `const char[] PROGMEM` | 老版完整控制页 | **未被任何 `server.send*` 引用**（`routeRoot` L1504 用的是 `INDEX_HTML_LITE`）|
| 1308 | `INDEX_HTML_CLEAN` | `const char[] PROGMEM` | 中间清理版控制页 | **同样未被引用** |
| 1407 | `INDEX_HTML_LITE` | `const char[] PROGMEM` | 当前线上首页 | `routeRoot` L1504（✓ 使用）|
| 1449 | `OTA_HTML` | `const char[] PROGMEM` | OTA 上传页 | `routeOtaPage` L1579（✓ 使用）|
| 1478 | `NETWORK_HTML` | `const char[] PROGMEM` | 网络设置页 | `routeNetworkPage` L1525（✓ 使用）|

## 汇总：只声明未使用 / 实质失效的全局

判定口径：变量本身或其唯一调用链已断（写入方/读取方均不可达），按 issue 要求只盘点不删除。

| 名称 | 行号 | 失效证据 |
|---|---|---|
| `INDEX_HTML` | 799 | 全文 `grep INDEX_HTML\b` 仅命中定义行；`routeRoot` L1504 改用 `INDEX_HTML_LITE` |
| `INDEX_HTML_CLEAN` | 1308 | 同上，无 `server.send*_P` 引用 |
| `idleEnabled` | 81 | 唯一写入者 `routeIdle` 未注册到 `server.on(...)`；唯一读取者 `idleTick` 不被 `loop()` 调用 |
| `lastIdleMs` | 84 | 仅在已死的 `idleTick` 中读写 |
| `progressBlinkOn` | 86 | `setProgress` L615 与 `progressTick` L629 只翻转它，未在任何绘图分支中被读取，对显示无影响 |
| `animSpeed` 的写入端 | 80 | 变量本身仍被 `speedMs` 用于动画时基（功能上活），但唯一写入路径 `routeSpeed` 未注册——结论：**值永远停留在初值 1（slow）**，速度切换 UI 实际无效 |
| `drawBgColor` 的所有写入与读取 | 89 | 除 `initColours` 外，全部触点 (`routeRedraw`/`routeCanvas`/`routeDrawClear`) 都在未注册路由内——结论：变量恒为 `C_ORANGE`，画板 (`VIEW_DRAW`) 不可达 |

补充：`AP_GATEWAY`、`AP_SUBNET` 仅在 `startSetupAp` 一次性使用，属正常的 "用一次的常量"，不算未使用。
