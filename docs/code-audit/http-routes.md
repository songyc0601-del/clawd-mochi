# clawd_mochi.ino HTTP 路由清单

文件：`clawd_mochi/clawd_mochi.ino`。路由注册集中在 `setup()` L1961-L1974；未注册但存在的 `route*` 函数见末尾汇总。

## 已注册路由

| 路径 | 方法 | 处理函数 | 注册行 | 请求参数 / 请求体 | 响应 |
|---|---|---|---:|---|---|
| `/` | GET | `routeRoot()` | L1961 | 无 | `INDEX_HTML_LITE` HTML（L1501-L1505） |
| `/network` | GET | `routeNetworkPage()` | L1962 | 无 | `NETWORK_HTML` HTML（L1523-L1526） |
| `/wifi/scan` | GET | `routeWifiScan()` | L1963 | 无 | JSON 数组：`[{ "ssid": "...", "rssi": -60 }]`，由 L1528-L1539 推断 |
| `/wifi/connect` | POST | `routeWifiConnect()` | L1964 | 表单或 query 参数：`ssid` 必填，`password` 可空；代码通过 `server.arg("ssid")`/`server.arg("password")` 读取（L1542-L1548） | `{"ok":1}`；缺少 SSID 时 `{"e":"ssid-required"}` |
| `/wifi/clear` | POST | `routeWifiClear()` | L1965 | 无 | `{"ok":1}`，随后重启 AP（L1556-L1564） |
| `/ota` | GET | `routeOtaPage()` | L1966 | 无 | `OTA_HTML` HTML（L1577-L1580） |
| `/ota` | POST | `routeOtaResult()` + `handleOtaUpload()` | L1967 | multipart OTA 上传；上传流来自 `server.upload()`（L1594-L1617） | text/plain：`OK rebooting` 或 `OTA failed`（L1582-L1592） |
| `/cmd` | GET | `routeCmd()` | L1968 | query：`k` 必填；`runNamedCommand` 仅接受 `normal` 或 `w`（L1646-L1651、L1653-L1662） | `{"ok":1}` 或 `{"e":1}` |
| `/backlight` | GET | `routeBacklight()` | L1969 | query：`on=1` 开，其它/缺省关（L1745-L1749） | `{"ok":1}` |
| `/progress` | GET | `routeProgress()` | L1970 | query：`state` 必填且须为 `OFFLINE/IDLE/PLAN/CODE/TEST/DONE/BLOCK`；`msg` 可选；`source` 可选且须为 `codex/claude/none`（L1751-L1775） | `{"ok":1}` 或 `{"e":1}` |
| `/agent-mode` | GET | `routeAgentMode()` | L1971 | query：`mode` 必填且须为 `AUTO/CODEX/CLAUDE`（L1777-L1785，校验在 L441-L447） | `{"ok":1}` 或 `{"e":1}` |
| `/expr` | GET | `routeExpr()` | L1972 | query：`name` 必填；实际只支持 `focus`、`happy`（L371-L381、L1787-L1795） | `{"ok":1}` 或 `{"e":1}` |
| `/state` | GET | `routeState()` | L1973 | 无 | 聚合 JSON：`view/busy/bl/progress/progressMsg/progressSource/agentMode/expr/wifi*`（L1819-L1840） |
| `onNotFound` | 任意 | `routeNotFound()` | L1974 | 未匹配路径 | text/plain `not found`（L1901） |

## 前端引用但未注册的路径

`INDEX_HTML` / `INDEX_HTML_CLEAN` / `INDEX_HTML_LITE` 中的 JavaScript 仍引用多条旧接口；其中当前首页 `INDEX_HTML_LITE`（L1407 起）引用的核心路径包括 `/cmd`、`/expr`、`/backlight`、`/agent-mode`、`/progress`、`/state`。老版 HTML 还引用了 `/speed`、`/redraw`、`/canvas`、`/draw/clear`、`/draw/stroke`、`/char`、`/idle`（证据：L1114-L1124、L1180-L1207、L1262-L1278、L1387-L1394、L1398-L1399），但对应路由没有在 L1961-L1974 注册。

## 未注册到 server 的处理函数

| 函数 | 定义行 | 推断路径 / 参数 | 证据 |
|---|---:|---|---|
| `routeChar()` | L1664 | `/char?c=<char>` | 函数读取 `server.arg("c")`（L1666-L1667），前端引用见 L1200-L1207、L1393-L1394；未在 L1961-L1974 注册 |
| `routeSpeed()` | L1671 | `/speed?v=1..3` | 函数读取 `server.arg("v")`（L1673），前端引用见 L1124、L1387；未注册 |
| `routeRedraw()` | L1678 | `/redraw?bg=#rrggbb` | 注释和代码见 L1677-L1682，前端引用见 L1116；未注册 |
| `routeCanvas()` | L1694 | `/canvas?on=1` | 函数读取 `server.arg("on")`（L1696），前端引用见 L1180、L1392；未注册 |
| `routeDrawClear()` | L1701 | `/draw/clear?bg=#rrggbb` | 函数读取 `server.arg("bg")`（L1703-L1704），前端引用见 L1114、L1184、L1278、L1392、L1399；未注册 |
| `routeDrawStroke()` | L1711 | `/draw/stroke?pen=<hex>&pts=x,y;x,y` | 函数要求 `pts` 和 `pen`（L1713-L1717），前端引用见 L1262、L1398；未注册 |
| `routeIdle()` | L1797 | `/idle?on=1` | 函数读取 `server.arg("on")`（L1799），前端引用见 L1390；未注册 |

按 issue 要求，本文件仅记录现状，不修改路由注册。
