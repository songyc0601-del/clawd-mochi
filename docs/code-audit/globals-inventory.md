# Globals inventory

审计对象：`clawd_mochi/clawd_mochi.ino`

本清单只记录当前源码仍存在的全局变量、常量、宏和静态数据。

## 硬件 / 框架对象

| 名称 | 类型 / 形式 | 用途 |
| --- | --- | --- |
| `TFT_CS`, `TFT_DC`, `TFT_RST`, `TFT_BLK` | `#define` | ST7789 SPI 和背光引脚。 |
| `tft` | `Adafruit_ST7789` | 全局显示对象。 |
| `AP_SSID`, `AP_PASS` | `const char*` | 默认 AP 名称和密码。 |
| `AP_IP`, `AP_GATEWAY`, `AP_SUBNET` | `const IPAddress` | 默认 AP 网络配置。 |
| `server` | `WebServer` | 本地 HTTP server。 |
| `wifiPrefs` | `Preferences` | 保存 WiFi 配置。 |
| `savedWifiSsid`, `savedWifiPassword` | `String` | 运行时 WiFi 配置缓存。 |

## 显示和视图常量

| 名称 | 类型 / 形式 | 用途 |
| --- | --- | --- |
| `DISP_W`, `DISP_H` | `#define` | 屏幕宽高，当前为 240x240。 |
| `EYE_W`, `EYE_H`, `EYE_GAP`, `EYE_OX`, `EYE_OY` | `#define` | 眼睛布局参数。 |
| `C_ORANGE`, `C_DARKBG`, `C_MUTED`, `C_GREEN` | `uint16_t` | 运行时初始化的主题色。 |
| `C_WHITE`, `C_BLACK` | `#define` | ST7789 白/黑色别名。 |
| `VIEW_EYES_NORMAL`, `VIEW_EYES_SQUISH`, `VIEW_CODE`, `VIEW_DRAW`, `VIEW_PROGRESS`, `VIEW_COMPANION` | `#define` | 视图枚举值。 |
| `EXPR_FOCUS`, `EXPR_HAPPY`, `EXPR_SLEEPY`, `EXPR_STARE`, `EXPR_BREAK` | `#define` | 陪伴表情枚举值。 |

## 运行时状态

| 名称 | 类型 | 初始值 | 用途 |
| --- | --- | --- | --- |
| `currentView` | `uint8_t` | `VIEW_EYES_NORMAL` | 当前显示视图。 |
| `busy` | `bool` | `false` | 动画 / OTA 忙碌标记。 |
| `backlightOn` | `bool` | `true` | 背光状态。 |
| `animSpeed` | `uint8_t` | `1` | 动画速度倍率输入。 |
| `companionExpr` | `uint8_t` | `EXPR_FOCUS` | 当前陪伴表情。 |
| `lastActionMs` | `uint32_t` | `0` | 最近交互时间。 |
| `lastProgressBlinkMs` | `uint32_t` | `0` | 进度动画 tick 时间。 |
| `progressBlinkOn` | `bool` | `true` | 进度闪烁状态。 |
| `animBgColor` | `uint16_t` | `0`，由 `initColours()` 设为 `C_ORANGE` | 眼睛和 Logo 背景色。 |
| `drawBgColor` | `uint16_t` | `0`，由 `initColours()` 设为 `C_ORANGE` | 保留的画板背景色状态。 |
| `progressState` | `String` | `PROGRESS_OFFLINE` | 当前进度状态。 |
| `progressMsg` | `String` | `""` | 当前进度短消息。 |
| `progressSource` | `String` | `"none"` | 当前进度来源。 |
| `agentMode` | `String` | `"AUTO"` | agent 显示模式。 |
| `progressPulsePhase` | `uint8_t` | `0` | 进度动画相位。 |
| `lastCodexProgressMs` | `uint32_t` | `0` | Codex 最近进度更新时间。 |
| `serialLine` | `String` | `""` | 串口输入行缓冲。 |

## 进度常量

| 名称 | 类型 | 值 |
| --- | --- | --- |
| `PROGRESS_OFFLINE` | `const String` | `"OFFLINE"` |
| `PROGRESS_IDLE` | `const String` | `"IDLE"` |
| `PROGRESS_PLAN` | `const String` | `"PLAN"` |
| `PROGRESS_CODE` | `const String` | `"CODE"` |
| `PROGRESS_TEST` | `const String` | `"TEST"` |
| `PROGRESS_DONE` | `const String` | `"DONE"` |
| `PROGRESS_BLOCK` | `const String` | `"BLOCK"` |
| `CODEX_OFFLINE_TIMEOUT_MS` | `const uint32_t` | `120000UL` |

## 终端状态

| 名称 | 类型 / 形式 | 用途 |
| --- | --- | --- |
| `TERM_COLS`, `TERM_ROWS`, `TERM_CHAR_W`, `TERM_CHAR_H`, `TERM_PAD_X`, `TERM_PAD_Y` | `#define` | 终端布局参数。 |
| `termMode` | `bool` | 终端模式标记。 |
| `termLines` | `String[TERM_ROWS]` | 终端文本缓冲。 |
| `termRow`, `termCol` | `uint8_t` | 当前终端光标位置。 |
| `PREFIX_PX` | `#define` | 终端提示符像素宽度。 |

## Logo 静态数据

| 名称 | 类型 / 形式 | 用途 |
| --- | --- | --- |
| `LOGO_CX`, `LOGO_CY` | `#define` | Logo 中心点。 |
| `LOGO_TRI_COUNT` | `#define` | Logo 三角形数量。 |
| `LOGO_TRIS` | `static const int16_t[][6] PROGMEM` | Logo 填充三角形数据。 |
| `LOGO_SEG_COUNT` | `#define` | Logo 线段数量。 |
| `LOGO_SEGS` | `static const int16_t[][4] PROGMEM` | Logo reveal 线段数据。 |

## HTML 常量

| 名称 | 类型 | 用途 |
| --- | --- | --- |
| `INDEX_HTML_LITE` | `const char[] PROGMEM` | 当前首页。 |
| `OTA_HTML` | `const char[] PROGMEM` | OTA 上传页。 |
| `NETWORK_HTML` | `const char[] PROGMEM` | WiFi 配网页。 |

当前 HTML 首页入口只记录 `INDEX_HTML_LITE`；源码中不再保留旧版首页大字符串。

## 已删除的旧全局状态

当前源码中不再存在旧审计记录里的 idle 闲置状态变量。
