# HTTP routes

审计对象：`clawd_mochi/clawd_mochi.ino`

当前所有 HTTP 路由都在 `setup()` 的路由注册段集中注册。

## 已注册路由

| 路径 | 方法 | Handler | 用途 |
| --- | --- | --- | --- |
| `/` | `GET` | `routeRoot()` | 返回主控制页 `INDEX_HTML_LITE`，并设置 no-cache header。 |
| `/network` | `GET` | `routeNetworkPage()` | 返回 WiFi 配网页 `NETWORK_HTML`。 |
| `/wifi/scan` | `GET` | `routeWifiScan()` | 扫描附近 WiFi，返回 SSID/RSSI JSON 数组。 |
| `/wifi/connect` | `POST` | `routeWifiConnect()` | 保存提交的 SSID/密码到 `Preferences`，并尝试 STA 连接。 |
| `/wifi/clear` | `POST` | `routeWifiClear()` | 清除保存的 WiFi 配置，断开 STA，并重启 AP。 |
| `/ota` | `GET` | `routeOtaPage()` | 返回 OTA 上传页 `OTA_HTML`。 |
| `/ota` | `POST` | `routeOtaResult()`, `handleOtaUpload()` | 处理 OTA 上传结果和上传流；成功后重启设备。 |
| `/cmd` | `GET` | `routeCmd()` | 执行命令参数 `k`；当前仅接受 `normal` / `w`。 |
| `/backlight` | `GET` | `routeBacklight()` | 根据 `on=1` 控制 TFT 背光。 |
| `/progress` | `GET` | `routeProgress()` | 更新进度状态、消息和可选来源。 |
| `/agent-mode` | `GET` | `routeAgentMode()` | 设置 agent 显示模式：`AUTO`、`CODEX`、`CLAUDE`。 |
| `/expr` | `GET` | `routeExpr()` | 设置陪伴表情；当前支持 `focus`、`happy`。 |
| `/state` | `GET` | `routeState()` | 返回设备视图、背光、进度、网络等 JSON 状态。 |
| `onNotFound` | 任意 | `routeNotFound()` | 对未匹配路径返回 404 文本。 |

## 当前页面调用

- `INDEX_HTML_LITE` 调用 `/cmd`、`/expr`、`/backlight`、`/agent-mode`、`/state`，并提供跳转到 `/ota` 和 `/network` 的入口。
- `NETWORK_HTML` 调用 `/state`、`/wifi/scan`、`/wifi/connect`、`/wifi/clear`。
- `OTA_HTML` 通过表单向 `/ota` 发起固件上传。

## 已移除的旧路径

旧版绘图、终端字符、速度、重绘和闲置控制路径已从源码中移除；当前只以本文件“已注册路由”表为准。
