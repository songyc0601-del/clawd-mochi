# clawd_mochi.ino 函数清单

文件：`clawd_mochi/clawd_mochi.ino`（共 1992 行）。该文件是唯一主 sketch；`dist/clawd_mochi/clawd_mochi.ino` 副本已删除，不再维护 dist 镜像同步。

下表按声明顺序列出全部 86 个函数（含 4 个 `inline` 工具、`setup` 与 `loop`）。"调用方"基于全文件 grep 引用统计；HTTP 路由处理函数额外标注是否在 `setup()`（L1961–1974）的 `server.on(...)` 中注册。

## 函数行号范围索引

| 函数 | 行号范围 |
|---|---:|
| `speedMs` | L232-L236 |
| `hexToRgb565` | L238-L243 |
| `setBacklight` | L245-L248 |
| `initColours` | L250-L258 |
| `drawLogoFilled` | L264-L276 |
| `eyeLX` | L283-L285 |
| `eyeRX` | L286-L286 |
| `eyeY` | L287-L287 |
| `eyeCY` | L288-L288 |
| `drawNormalEyes` | L290-L300 |
| `drawChevron` | L302-L313 |
| `drawSquishEyes` | L315-L329 |
| `drawCompanionEyes` | L331-L369 |
| `setCompanionExpr` | L371-L381 |
| `idleTick` | L383-L403 |
| `drawCodeView` | L405-L415 |
| `progressColor` | L417-L426 |
| `isCodexLayerState` | L428-L431 |
| `isProgressState` | L433-L435 |
| `isProgressSource` | L437-L439 |
| `setAgentMode` | L441-L447 |
| `progressStage` | L449-L456 |
| `progressDefaultMessage` | L458-L467 |
| `cleanAscii` | L469-L477 |
| `drawProgressBars` | L479-L485 |
| `drawCodexCore` | L487-L495 |
| `drawCodexScanLine` | L497-L500 |
| `drawClaudeCodeStyleLayer` | L502-L521 |
| `drawCodexProgressView` | L523-L553 |
| `drawClaudeProgressView` | L555-L582 |
| `drawDefaultClawdView` | L584-L588 |
| `drawProgressView` | L590-L602 |
| `drawCodexIdleView` | L604-L606 |
| `setProgress` | L608-L621 |
| `progressTick` | L623-L638 |
| `checkCodexOfflineTimeout` | L640-L645 |
| `termClear` | L651-L654 |
| `termDrawHeader` | L656-L661 |
| `termDrawPrefix` | L664-L668 |
| `termDrawLine` | L672-L684 |
| `termDrawLastChar` | L686-L698 |
| `termDrawBackspace` | L700-L711 |
| `termFullRedraw` | L713-L717 |
| `termScroll` | L719-L724 |
| `termAddChar` | L726-L752 |
| `animNormalEyes` | L758-L767 |
| `animSquishEyes` | L769-L777 |
| `animLogoReveal` | L779-L794 |
| `routeRoot` | L1501-L1505 |
| `connectSavedWifi` | L1507-L1510 |
| `startSetupAp` | L1512-L1516 |
| `restartSetupAp` | L1518-L1521 |
| `routeNetworkPage` | L1523-L1526 |
| `routeWifiScan` | L1528-L1539 |
| `routeWifiConnect` | L1541-L1554 |
| `routeWifiClear` | L1556-L1564 |
| `drawOtaStatus` | L1566-L1575 |
| `routeOtaPage` | L1577-L1580 |
| `routeOtaResult` | L1582-L1592 |
| `handleOtaUpload` | L1594-L1617 |
| `markAction` | L1619-L1621 |
| `showNormal` | L1623-L1628 |
| `showSquish` | L1630-L1635 |
| `showCodeTerminal` | L1637-L1644 |
| `runNamedCommand` | L1646-L1651 |
| `routeCmd` | L1653-L1662 |
| `routeChar` | L1664-L1669 |
| `routeSpeed` | L1671-L1675 |
| `routeRedraw` | L1678-L1692 |
| `routeCanvas` | L1694-L1699 |
| `routeDrawClear` | L1701-L1709 |
| `routeDrawStroke` | L1711-L1743 |
| `routeBacklight` | L1745-L1749 |
| `routeProgress` | L1751-L1775 |
| `routeAgentMode` | L1777-L1785 |
| `routeExpr` | L1787-L1795 |
| `routeIdle` | L1797-L1801 |
| `rgb565ToHex` | L1804-L1811 |
| `jsonEscape` | L1813-L1817 |
| `stateJson` | L1819-L1835 |
| `routeState` | L1837-L1840 |
| `handleSerialCommand` | L1842-L1882 |
| `handleSerial` | L1884-L1899 |
| `routeNotFound` | L1901-L1901 |
| `setup` | L1907-L1981 |
| `loop` | L1987-L1992 |

## 工具与颜色

| 行号 | 名称(参数) | 返回 | 一句话职责 | 调用方 |
|---|---|---|---|---|
| 232 | `speedMs(int ms)` | int | 根据 `animSpeed` 把动画延时拉长/压缩 | `animLogoReveal` L789、`animNormalEyes` L761–764、`animSquishEyes` L772–773、`idleTick` L392/396/400 |
| 238 | `hexToRgb565(String hex)` | uint16_t | `#RRGGBB` 文本转 RGB565 | `routeRedraw` L1681、`routeDrawClear` L1704、`routeDrawStroke` L1716 |
| 245 | `setBacklight(bool on)` | void | 控制背光 GPIO 并更新 `backlightOn` | `setup` L1911、`routeBacklight` L1747、`handleSerialCommand` L1861 |
| 250 | `initColours()` | void | 初始化 `C_*` 与 `animBgColor`/`drawBgColor` | `setup` L1917 |

## Logo

| 行号 | 名称(参数) | 返回 | 一句话职责 | 调用方 |
|---|---|---|---|---|
| 264 | `drawLogoFilled(uint16_t bg, uint16_t fg)` | void | 用 `LOGO_TRIS` 把 logo 实心绘制 | `animLogoReveal` L791 |

## 眼睛几何与绘制

| 行号 | 名称(参数) | 返回 | 一句话职责 | 调用方 |
|---|---|---|---|---|
| 283 | `inline eyeLX(int16_t ox)` | int16_t | 左眼 X | `drawNormalEyes` L292、`drawSquishEyes` L317、`drawCompanionEyes` L333 |
| 286 | `inline eyeRX(int16_t ox)` | int16_t | 右眼 X | 同上三处 |
| 287 | `inline eyeY()` | int16_t | 眼框 Y | 同上 + `eyeCY` L288 |
| 288 | `inline eyeCY()` | int16_t | 眼框中心 Y | `drawSquishEyes` L317、`drawCompanionEyes` L334 |
| 290 | `drawNormalEyes(int16_t ox=0, bool blink=false)` | void | 绘制正常方眼（含眨眼） | `idleTick` L395/397、`animNormalEyes` L761–765、`drawDefaultClawdView` L587、`routeRedraw` L1685 |
| 302 | `drawChevron(cx,cy,arm,reach,thk,rightFacing,col)` | void | 绘制 `>` `<` 形眯眼线条 | `drawSquishEyes` L323–324、`drawCompanionEyes` L338–339 |
| 315 | `drawSquishEyes(bool closed=false)` | void | 绘制眯眯眼 | `idleTick` L399–401、`animSquishEyes` L772–775、`routeRedraw` L1686 |
| 331 | `drawCompanionEyes(uint8_t expr,int16_t ox=0,bool idle=false)` | void | 按表情枚举绘制陪伴眼 | `setCompanionExpr` L379、`idleTick` L391–393 |
| 371 | `setCompanionExpr(String name)` | bool | 文本表情名 → 切换到陪伴视图（仅支持 `focus`/`happy`，其他名字返回 false 即便文案出现） | `routeExpr` L1790 |
| 383 | `idleTick()` | void | 主循环里的闲置眨眼动画 | **无调用方**（见末尾汇总）|

## 视图与进度

| 行号 | 名称(参数) | 返回 | 一句话职责 | 调用方 |
|---|---|---|---|---|
| 405 | `drawCodeView()` | void | 绘制 "Claude Code" 文字屏 | `showCodeTerminal` L1640、`routeRedraw` L1687 |
| 417 | `progressColor(const String& state)` | uint16_t | 状态 → 颜色 | `drawCodexProgressView` L525/540、`drawClaudeProgressView` L557/569、`progressTick` L631 |
| 428 | `isCodexLayerState(const String& state)` | bool | 是否 IDLE/PLAN/CODE/TEST/DONE/BLOCK | `isProgressState` L434、`progressTick` L624、`checkCodexOfflineTimeout` L641 |
| 433 | `isProgressState(const String& state)` | bool | 是否合法进度状态（含 OFFLINE） | `setProgress` L611 |
| 437 | `isProgressSource(const String& source)` | bool | 是否合法 `progressSource` | **无调用方**（`routeProgress` L1759 是内联枚举检查）|
| 441 | `setAgentMode(String mode)` | bool | 大写归一化并校验 AUTO/CODEX/CLAUDE | `routeAgentMode` L1779 |
| 449 | `progressStage(const String& state)` | uint8_t | 进度条阶段 0–4 | `drawCodexProgressView` L526、`drawClaudeProgressView` L558 |
| 458 | `progressDefaultMessage(const String& state)` | String | 默认消息文案 | `drawCodexProgressView` L527、`drawClaudeProgressView` L559、`setProgress` L614 |
| 469 | `cleanAscii(String text, uint8_t maxLen)` | String | 截断为可打印 ASCII | `setProgress` L613 |
| 479 | `drawProgressBars(uint8_t stage, uint16_t col)` | void | 4 段进度条 | `drawCodexProgressView` L540、`drawClaudeProgressView` L569 |
| 487 | `drawCodexCore(uint16_t col, uint8_t pulse)` | void | Codex 中央脉冲圆 | `drawCodexProgressView` L537、`progressTick` L635 |
| 497 | `drawCodexScanLine(uint16_t col, uint8_t pulse)` | void | TEST 状态下的扫描线 | `drawCodexProgressView` L538、`progressTick` L636 |
| 502 | `drawClaudeCodeStyleLayer(uint16_t col, uint8_t pulse)` | void | Claude 风格花瓣层 | `drawClaudeProgressView` L568、`progressTick` L633 |
| 523 | `drawCodexProgressView()` | void | 完整 Codex 进度屏 | `drawProgressView` L601 |
| 555 | `drawClaudeProgressView()` | void | 完整 Claude 进度屏 | `drawProgressView` L598 |
| 584 | `drawDefaultClawdView()` | void | 切回 VIEW_EYES_NORMAL 并绘制 | `drawProgressView` L593（OFFLINE 时）、`setup` L1980 |
| 590 | `drawProgressView()` | void | 进度屏路由（OFFLINE / claude / codex） | `drawCodexIdleView` L605、`setProgress` L619、`routeRedraw` L1689 |
| 604 | `drawCodexIdleView()` | void | `drawProgressView()` 的同义包装 | **无调用方** |
| 608 | `setProgress(String state, String msg)` | bool | 设进度状态并立即重绘 | `routeProgress` L1768、`handleSerialCommand` L1874、`checkCodexOfflineTimeout` L643 |
| 623 | `progressTick()` | void | 每 500 ms 推动脉冲 | `loop` L1991 |
| 640 | `checkCodexOfflineTimeout()` | void | 120 s 无心跳 → 强制 OFFLINE | `loop` L1990 |

## 终端

| 行号 | 名称(参数) | 返回 | 一句话职责 | 调用方 |
|---|---|---|---|---|
| 651 | `termClear()` | void | 清空文本缓冲 | `showCodeTerminal` L1642 |
| 656 | `termDrawHeader()` | void | 画顶栏 | `termFullRedraw` L715 |
| 664 | `termDrawPrefix(int16_t yy)` | void | 行首 `clawd:~$` | `termDrawLine` L676、`termAddChar` L747 |
| 672 | `termDrawLine(uint8_t r)` | void | 整行重绘 | `termFullRedraw` L716、`termAddChar` L734 |
| 686 | `termDrawLastChar()` | void | 增量绘制新输入字符 | `termAddChar` L750 |
| 700 | `termDrawBackspace()` | void | 退格擦除 | `termAddChar` L739 |
| 713 | `termFullRedraw()` | void | 整屏重画终端 | `termScroll` L723、`showCodeTerminal` L1643 |
| 719 | `termScroll()` | void | 上滚一行 | `termAddChar` L733/744 |
| 726 | `termAddChar(char c)` | void | 输入分发 | `routeChar` L1667（**该路由未注册，见 [http-routes.md](http-routes.md)**）|

## 动画

| 行号 | 名称(参数) | 返回 | 一句话职责 | 调用方 |
|---|---|---|---|---|
| 758 | `animNormalEyes()` | void | 正常眼摆动+眨眼 | `showNormal` L1627 |
| 769 | `animSquishEyes()` | void | 眯眯眼睁/闭三次 | `showSquish` L1634（**`showSquish` 无人调用**）|
| 779 | `animLogoReveal()` | void | 开机 logo 逐段揭示 | `setup` L1927 |

## Web 路由与网络

| 行号 | 名称(参数) | 返回 | 一句话职责 | 注册路径 |
|---|---|---|---|---|
| 1501 | `routeRoot()` | void | 返回 `INDEX_HTML_LITE` | ✓ `GET /` |
| 1507 | `connectSavedWifi()` | void | 用保存的 SSID/PWD 连 STA | 调用方 `setup` L1934、`routeWifiConnect` L1552 |
| 1512 | `startSetupAp()` | void | 启 AP+STA 模式 | `setup` L1930、`restartSetupAp` L1520 |
| 1518 | `restartSetupAp()` | void | 重启 AP | `routeWifiClear` L1563 |
| 1523 | `routeNetworkPage()` | void | 返回 `NETWORK_HTML` | ✓ `GET /network` |
| 1528 | `routeWifiScan()` | void | 扫 AP 列表返回 JSON | ✓ `GET /wifi/scan` |
| 1541 | `routeWifiConnect()` | void | 保存并连接 WiFi | ✓ `POST /wifi/connect` |
| 1556 | `routeWifiClear()` | void | 清空 WiFi 配置 | ✓ `POST /wifi/clear` |
| 1566 | `drawOtaStatus(line1,line2,col)` | void | OTA 进度文案屏 | `handleOtaUpload` L1597–1614 |
| 1577 | `routeOtaPage()` | void | 返回 `OTA_HTML` | ✓ `GET /ota` |
| 1582 | `routeOtaResult()` | void | OTA 上传完成后响应并重启 | ✓ `POST /ota`（与 `handleOtaUpload` 配对）|
| 1594 | `handleOtaUpload()` | void | OTA 文件流分块写入 | ✓ `POST /ota` 上传回调 |
| 1619 | `markAction()` | void | 更新 `lastActionMs` | `showNormal/Squish/CodeTerminal` 与所有 `route*` 入口处 |
| 1623 | `showNormal()` | void | 切回正常眼并播动画 | `runNamedCommand` L1649 |
| 1630 | `showSquish()` | void | 切到眯眯眼并播动画 | **无调用方**（见末尾汇总）|
| 1637 | `showCodeTerminal()` | void | 切到代码视图+终端模式 | **无调用方** |
| 1646 | `runNamedCommand(String name)` | bool | 命令分发；仅匹配 `normal`/`w` | `routeCmd` L1657、`handleSerialCommand` L1853 |
| 1653 | `routeCmd()` | void | `GET /cmd?k=...` | ✓ `GET /cmd` |
| 1664 | `routeChar()` | void | 终端单字符输入 | ✗ **未注册** |
| 1671 | `routeSpeed()` | void | 设动画速度 1–3 | ✗ **未注册** |
| 1678 | `routeRedraw()` | void | 改背景色并重绘当前视图 | ✗ **未注册** |
| 1694 | `routeCanvas()` | void | 进入画板视图 | ✗ **未注册** |
| 1701 | `routeDrawClear()` | void | 清空画板 | ✗ **未注册** |
| 1711 | `routeDrawStroke()` | void | 画一笔轨迹 | ✗ **未注册** |
| 1745 | `routeBacklight()` | void | 背光开关 | ✓ `GET /backlight` |
| 1751 | `routeProgress()` | void | 设进度状态/消息/source | ✓ `GET /progress` |
| 1777 | `routeAgentMode()` | void | 设 AUTO/CODEX/CLAUDE | ✓ `GET /agent-mode` |
| 1787 | `routeExpr()` | void | 设陪伴表情 | ✓ `GET /expr` |
| 1797 | `routeIdle()` | void | 设 `idleEnabled` | ✗ **未注册** |
| 1804 | `rgb565ToHex(uint16_t c)` | String | RGB565 → `#rrggbb` | **无调用方** |
| 1813 | `jsonEscape(String s)` | String | JSON 字符串转义 | `routeWifiScan` L1533、`stateJson` L1823–1830 |
| 1819 | `stateJson()` | String | 设备状态聚合 JSON | `routeState` L1838、`handleSerialCommand` L1848 |
| 1837 | `routeState()` | void | 返回 `stateJson()` | ✓ `GET /state` |
| 1842 | `handleSerialCommand(String line)` | String | 串口行命令分发 | `handleSerial` L1889 |
| 1884 | `handleSerial()` | void | 串口字节装行后分发 | `loop` L1989 |
| 1901 | `routeNotFound()` | void | 404 文本 | ✓ `onNotFound` |
| 1907 | `setup()` | void | 启动序列 + 路由注册 | Arduino 入口 |
| 1987 | `loop()` | void | 主循环 | Arduino 入口 |

## 汇总：未被调用的函数（候选删除）

证据均为：在 `clawd_mochi/clawd_mochi.ino` 全文 grep 该函数名仅命中**自身定义行**，且不是 `server.on(...)` 注册的路由处理器、不是 Arduino 框架入口（`setup`/`loop`）、不是 OTA 上传回调。

| 函数 | 定义行 | 性质 |
|---|---|---|
| `idleTick()` | 383 | 闲置眨眼动画函数，全文件无人调用（`loop()` L1987–1991 未调用，造成 `idleEnabled`/`lastIdleMs` 全局变量随之失效 — 详见 [globals-inventory.md](globals-inventory.md)）|
| `isProgressSource(const String&)` | 437 | 校验函数；`routeProgress` L1759 用内联三选一替代了它 |
| `drawCodexIdleView()` | 604 | 对 `drawProgressView()` 的等价包装，无人调用 |
| `showSquish()` | 1630 | `runNamedCommand` L1649 只匹配 `"normal"/"w"`，`s` 命令路径未连通，因此 `showSquish`→`animSquishEyes`→`drawSquishEyes` 在动画方向上整条链都不会经由命令触发（注：`drawSquishEyes` 仍被 `idleTick`/`routeRedraw` 引用，但前者已死、后者也未注册）|
| `showCodeTerminal()` | 1637 | 同上，`d` 命令未在 `runNamedCommand` 中匹配 |
| `routeChar()` | 1664 | 未注册到 `server.on(...)`；其唯一下游 `termAddChar` 因此也无可达入口 |
| `routeSpeed()` | 1671 | 未注册到 `server.on(...)` |
| `routeRedraw()` | 1678 | 未注册到 `server.on(...)` |
| `routeCanvas()` | 1694 | 未注册到 `server.on(...)`；`VIEW_DRAW` 视图无可达入口 |
| `routeDrawClear()` | 1701 | 未注册到 `server.on(...)` |
| `routeDrawStroke()` | 1711 | 未注册到 `server.on(...)` |
| `routeIdle()` | 1797 | 未注册到 `server.on(...)`，与 `idleEnabled`/`idleTick` 死链一致 |
| `rgb565ToHex(uint16_t)` | 1804 | 全文无人调用；`stateJson()` 输出里也没有它对应的字段 |

二级影响（"主调用方已死" 的函数；自身可能仍有"活的"间接调用方，不直接列为删除候选，但与上表是同一死链）：
- `animSquishEyes` L769 → 仅 `showSquish` 调用
- `termClear` L651、`termFullRedraw` L713（部分）、`termDrawLine`/`termDrawPrefix`/`termDrawLastChar`/`termDrawBackspace`/`termScroll`/`termAddChar` 整条终端绘制链条 → 入口 `showCodeTerminal` 与 `routeChar` 双双未连通
- `drawSquishEyes` L315 在 `routeRedraw`（未注册）与 `idleTick`（不调用）之外只在 `animSquishEyes` 这条死链上出现
- `VIEW_DRAW`/`VIEW_CODE`/`VIEW_EYES_SQUISH`/`VIEW_COMPANION` 几种视图也只能通过上述未注册路由或表情接口（`VIEW_COMPANION` 仍通过 `routeExpr` 可达）被切换到

按 issue 要求**仅盘点不删除**，最终是否裁剪由后续整理 issue 决定。
