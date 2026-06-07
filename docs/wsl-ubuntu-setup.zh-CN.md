# WSL Ubuntu 开发环境说明

本文件说明如何在 WSL Ubuntu 中编译、上传和维护 Clawd Mochi 项目。

当前项目已从 Windows 迁移到 WSL Ubuntu。Windows PowerShell 脚本仍然保留，用于 Windows 环境或回退排错；WSL 下优先使用 `arduino-cli`。`tools/codex-stage.sh` 是 Linux 侧状态推送原语，可被客户端 watcher、hook、wrapper 或定时任务调用。

## 目标环境

- WSL Ubuntu 22.04/24.04 或相近版本。
- 项目目录：`/home/songyc/code/clawd-mochi`。
- 固件目录：`clawd_mochi`。
- 默认板卡 FQBN：`esp32:esp32:esp32c3:CDCOnBoot=cdc,CPUFreq=160,UploadSpeed=115200`。

以下命令默认在项目根目录运行。

## 安装 Arduino CLI

如果仓库里的 `bin/arduino-cli` 可用，可以先临时加入 `PATH`：

```bash
export PATH="$PWD/bin:$PATH"
arduino-cli version
```

如果需要安装到当前用户目录：

```bash
mkdir -p "$HOME/.local/bin"
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="$HOME/.local/bin" sh
export PATH="$HOME/.local/bin:$PATH"
arduino-cli version
```

建议把 `export PATH="$HOME/.local/bin:$PATH"` 写入 `~/.bashrc`，下次打开 WSL 后自动生效。

## 安装 ESP32 Core

初始化配置并加入 Espressif 的 Boards Manager 地址：

```bash
arduino-cli config init
arduino-cli config set board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli core update-index
```

安装 ESP32 core：

```bash
arduino-cli core install esp32:esp32
```

如果最新版下载失败，可以先安装 Windows 离线说明中验证过的版本：

```bash
arduino-cli core install esp32:esp32@2.0.17
```

ESP32 core 还在下载时不要急着编译；等 `arduino-cli core list` 能看到 `esp32:esp32` 后再继续。

## 安装依赖库

项目使用 Adafruit GFX 和 ST7789 屏幕库。`Adafruit BusIO` 通常会作为依赖自动安装，也可以显式安装：

```bash
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit ST7735 and ST7789 Library"
arduino-cli lib install "Adafruit BusIO"
```

检查已安装内容：

```bash
arduino-cli core list
arduino-cli lib list | grep -E "Adafruit GFX|Adafruit ST7735|Adafruit BusIO"
```

## 编译

默认 Linux 编译命令：

```bash
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc,CPUFreq=160,UploadSpeed=115200 clawd_mochi
```

如需生成可用于 OTA 的二进制文件，可以指定输出目录：

```bash
mkdir -p build
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc,CPUFreq=160,UploadSpeed=115200 --output-dir build clawd_mochi
```

当前环境已验证可使用 Arduino CLI 和 ESP32 core 编译。修改固件后优先运行上面的默认编译命令做本地验证；上传或 OTA 只在明确需要部署到设备时执行。

## OTA 上传

首次烧录仍然需要 USB。完成首次烧录并连上网络后，后续可以通过浏览器 OTA：

1. 编译生成 `.bin` 文件。
2. 电脑与 Clawd Mochi 连接同一个局域网。
3. 打开设备局域网地址：

```text
http://设备局域网IP/ota
```

也可以连接设备热点 `ClaWD-Mochi` 后打开兜底地址：

```text
http://192.168.4.1/ota
```

选择 `build/clawd_mochi.ino.bin` 或 Arduino CLI 构建缓存里的 ESP32-C3 `.bin` 文件上传。不要上传 `.ino`、`.zip` 或其他文件。

## USB 上传

WSL 需要能看到 ESP32-C3 的串口设备。Windows 11 推荐使用 `usbipd-win` 把 USB 设备附加到 WSL。

在 Windows PowerShell 管理员窗口中查看设备：

```powershell
usbipd list
```

把对应 BUSID 绑定并附加到 WSL：

```powershell
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```

回到 WSL 后确认串口：

```bash
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
```

上传命令示例：

```bash
arduino-cli upload -p /dev/ttyACM0 --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc,CPUFreq=160,UploadSpeed=115200 clawd_mochi
```

如果权限不足：

```bash
sudo usermod -aG dialout "$USER"
```

然后重启 WSL 会话。也可以临时用 `sudo arduino-cli upload ...` 排查权限问题。

## WSL 下推送 Clawd Mochi 状态

Linux 版状态脚本通过 HTTP 调用设备的 `/state` 和 `/progress`。它只负责“把明确状态发给设备”，不负责判断 Codex 当前处于什么阶段：

```bash
./tools/codex-stage.sh -State STATE
./tools/codex-stage.sh -State PLAN -Message planning
./tools/codex-stage.sh -State CODE -Message editing
./tools/codex-stage.sh -State TEST -Message verifying
./tools/codex-stage.sh -State DONE -Message complete
./tools/codex-stage.sh -State BLOCK -Message need-input
./tools/codex-stage.sh -State OFFLINE -Message codex-offline
```

默认设备地址是 `http://192.168.4.1`。如果设备已经连入局域网：

```bash
./tools/codex-stage.sh -DeviceUrl http://设备局域网IP -State CODE -Message editing
```

自动同步应由客户端侧 watcher、hook、wrapper 或定时任务实现，详见 [Codex 客户端状态同步方案](codex-client-status-sync.zh-CN.md)。Windows PowerShell 版 `tools/codex-stage.ps1` 仍保留，并继续支持 WiFi 失败后回退到 USB 串口。

WSL 下可直接启动已实现的 watcher：

```bash
./tools/codex-watch.sh
```

如果设备已经连入局域网：

```bash
./tools/codex-watch.sh --device-url http://设备局域网IP
```

调试时只运行一轮：

```bash
./tools/codex-watch.sh --once --verbose
```

## 常见问题

- `arduino-cli: command not found`：确认 `bin`、`~/.local/bin` 或 Arduino CLI 安装目录已经加入 `PATH`。
- `platform esp32:esp32 is not installed`：ESP32 core 还没装好，继续等待或重试 `arduino-cli core install esp32:esp32`。
- 找不到 `/dev/ttyACM0`：确认 USB 数据线可传数据，并用 `usbipd` 附加到 WSL。
- 上传权限不足：把当前用户加入 `dialout` 后重启 WSL，或临时用 `sudo` 验证。
- OTA 页面打不开：确认电脑和设备在同一局域网，或连接 `ClaWD-Mochi` 后使用 `http://192.168.4.1/ota`。
