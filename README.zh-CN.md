# Clawd Mochi 中文说明

Clawd Mochi 是一个基于 ESP32-C3 Super Mini 和 ST7789 1.54 寸屏幕的桌面小摆件。它支持手机网页控制、USB 串口控制、Codex 工作进度显示和 Web OTA 在线升级。

## 首次网络配置

1. 给 ESP32-C3 通电。
2. 手机连接 WiFi：`ClaWD-Mochi`。
3. 输入密码：`clawd1234`。
4. 浏览器打开：`http://192.168.4.1`。
5. 点击“网络设置”，选择现有的 2.4 GHz WiFi，填写密码并保存。
6. 连接成功后，屏幕和网络设置页会显示设备的局域网 IP。

ESP32-C3 只支持 2.4 GHz WiFi。设备热点会始终保留，配置失败时仍可通过 `http://192.168.4.1/network` 重新设置。

## 日常使用

电脑继续连接原有 WiFi，不需要加入 `ClaWD-Mochi` 热点。浏览器打开屏幕显示的局域网 IP，即可控制设备和使用 OTA。

网页首页按 3 个区域组织：只读工作状态、屏幕显示、设备管理。
工作状态由已接入的 App 自动同步，网页不支持手动修改。

## 日常陪伴

适合平时放在桌面上使用。

- 屏幕开关：打开或关闭背光。
- 基础表情：正常眼睛。
- 陪伴表情：
  - `focus`：专注
  - `happy`：开心

## 工作状态（Codex）

适合把设备当作桌面工作状态牌。Codex 是当前已接入的状态来源，后续可以继续接入其他 App。

- 启动后短暂显示 WiFi 信息；Codex 未打开、watcher 未运行或超时离线时显示默认 Clawd 动画形象。
- 只有客户端 watcher 明确推送 Codex 在线状态后，才进入 Codex 核心脉冲状态屏。
- Codex 阶段状态：
  - `OFFLINE`：Codex 离线，回到默认 Clawd 动画形象
  - `IDLE`：Codex 已在线但暂无任务
  - `PLAN`：规划中，阶段条 1/4
  - `CODE`：工作中，阶段条 2/4
  - `TEST`：验证中，阶段条 3/4
  - `DONE`：已完成，阶段条 4/4
  - `BLOCK`：需要输入或被阻塞，红色满格
- `IDLE / PLAN / CODE / TEST / DONE / BLOCK` 使用 Codex 核心脉冲形象、状态颜色、英文状态词和 4 段阶段条。
- `DONE` 默认保持到 watcher 300 秒无活动后转为 `IDLE codex-ready`；`BLOCK` 不被普通完成/待机计时器自动降级。
- 设备端 120 秒未收到非 `OFFLINE` 状态会自动回到 `OFFLINE codex-timeout`。
- 电脑也可以通过 USB 串口脚本推送状态。
- 推荐由客户端 watcher、hook、wrapper 或定时任务自动观察 Codex 状态并推送到设备，而不是依赖模型在工作中手动调用脚本。
- 当前 Windows 侧可通过 `tools/codex-notify.ps1` 对接回合结束事件，推送 `DONE`，并保留原有 Computer Use 通知。
- `tools/codex-stage.ps1` 优先通过 WiFi HTTP 快速推送，WiFi 不可用时自动回退到 `COM7` USB 串口。
- Linux/WSL 侧可使用 `tools/codex-stage.sh` 作为 HTTP 推送原语。
- 自动 watcher：
  - WSL/Linux/macOS：`./tools/codex-watch.sh`
  - Windows：`powershell -ExecutionPolicy Bypass -File .\tools\codex-watch.ps1`

示例：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\codex-progress.ps1 -Port COM7 -State CODE -Message editing
powershell -ExecutionPolicy Bypass -File .\tools\codex-stage.ps1 -State TEST -Message verifying
```

WSL 示例：

```bash
./tools/codex-watch.sh --device-url http://设备局域网IP
./tools/codex-watch.sh --once --verbose
./tools/codex-watch.sh --device-url http://设备局域网IP --heartbeat 60
```

## 设备维护

- 状态刷新：同步设备当前状态。
- 网络设置：加入现有 2.4 GHz WiFi，或清除已保存的网络配置。
- OTA 在线升级：通过设备局域网 IP 的 `/ota` 上传 `.bin` 固件；热点地址 `http://192.168.4.1/ota` 也始终可用。
- USB 串口控制：适合电脑自动化或排错。

OTA 说明见 [OTA 在线升级说明](docs/ota-update.zh-CN.md)。
USB/Codex 说明见 [USB 串口与 Codex 进度说明](docs/usb-serial-codex.zh-CN.md)。
Codex 客户端状态同步方案见 [Codex 客户端状态同步方案](docs/codex-client-status-sync.zh-CN.md)。
WSL Ubuntu 开发环境见 [WSL Ubuntu 开发环境说明](docs/wsl-ubuntu-setup.zh-CN.md)。
离线安装说明见 [Windows 离线安装说明](docs/offline-install-windows.zh-CN.md)。

## 硬件接线

注意：屏幕 VCC 只能接 `3V3`，不要接 `5V`。

| 屏幕引脚 | ESP32-C3 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 10 |
| SCL | GPIO 8 |
| RES/RST | GPIO 2 |
| DC | GPIO 1 |
| CS | GPIO 4 |
| BL | GPIO 3 |

`SDA -> GPIO 10` 里的 GPIO10 同时也是硬件 SPI 的 `MOSI`。不用额外找一个叫 MOSI 的针脚，按 GPIO10 接就可以。

## Arduino IDE 设置

1. 安装 Arduino IDE 2.x。
2. 打开 `File -> Preferences`。
3. 在 `Additional boards manager URLs` 填入：

```text
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

4. 打开 `Tools -> Board -> Boards Manager`，搜索 `esp32`，安装 `esp32 by Espressif Systems`。
5. 打开 `Tools -> Library Manager`，安装：
   - `Adafruit GFX Library`
   - `Adafruit ST7735 and ST7789 Library`

## 板卡参数

| 设置项 | 推荐值 |
| --- | --- |
| Board | ESP32C3 Dev Module |
| USB CDC On Boot | Enabled |
| CPU Frequency | 160 MHz |
| Upload Speed | 115200 |
| Port | 你的 ESP32-C3 COM 口 |

如果上传失败，先把 `Upload Speed` 改成 `115200`。

## 烧录

1. 打开 `clawd_mochi/clawd_mochi.ino`。
2. 用 USB-C 数据线连接 ESP32-C3。
3. 选择正确的 `Tools -> Port`。
4. 点击 Arduino IDE 左上角上传按钮。
5. 看到 `Hard resetting via RTS pin...` 通常表示上传成功。

## USB 串口命令

固件支持 115200 波特率的 ASCII 串口命令：

```text
STATE
CMD normal
BL 0
BL 1
PROGRESS PLAN planning
PROGRESS CODE editing
PROGRESS TEST verifying
PROGRESS DONE complete
PROGRESS BLOCK need-input
PROGRESS IDLE
PROGRESS OFFLINE codex-offline
```

`STATE` 会返回一行 JSON 状态。

手机网页仍可切换 `focus` 和 `happy`；USB 串口只保留基础界面、背光、Codex 进度和状态查询命令。

## 常见问题

### 找不到 COM 口

换一根支持数据传输的 USB-C 线。有些线只能充电。也可以在 Windows 设备管理器里查看是否出现新的串口。

### 连上 WiFi 但打不开网页

首次配置时确认手机连接的是 `ClaWD-Mochi`，浏览器地址是 `http://192.168.4.1`。日常使用时，电脑和设备必须连接同一个局域网，并使用屏幕显示的局域网 IP。

### 串口脚本提示端口被占用

关闭 Arduino IDE 的串口监视器，或关闭其他正在占用 COM 口的软件，然后重试。
