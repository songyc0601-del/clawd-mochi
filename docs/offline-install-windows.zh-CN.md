# Windows 离线安装说明

适用于 Arduino IDE 下载 `esp32` 平台或依赖库很慢、经常失败的情况。

## 需要准备

- Arduino IDE 2.x
- ESP32 平台安装包
- Adafruit 相关库

Arduino IDE 的用户数据目录通常是：

```text
C:\Users\你的用户名\AppData\Local\Arduino15
```

你的电脑当前目录示例：

```text
C:\Users\songy\AppData\Local\Arduino15
```

## 放置下载缓存

Arduino IDE 下载中的压缩包会放在：

```text
C:\Users\你的用户名\AppData\Local\Arduino15\staging\packages
```

如果你已经手动下载了失败日志里提示的 zip 文件，把它放到这个目录后，再回到 Arduino IDE 重新安装对应平台。

示例失败文件：

```text
xtensa-esp-elf-14.2.0_20260121-x86_64-w64-mingw32.zip
```

放置位置示例：

```text
C:\Users\songy\AppData\Local\Arduino15\staging\packages\
```

## 推荐版本

本项目已经验证过以下组合可以编译上传：

| 项目 | 版本 |
| --- | --- |
| esp32 by Espressif Systems | 2.0.17 |
| Adafruit GFX Library | 1.12.6 |
| Adafruit ST7735 and ST7789 Library | 1.11.0 |

如果最新版 `esp32:esp32` 下载失败，可以先安装 `2.0.17`。对本项目来说功能够用，而且依赖更容易下载成功。

## Arduino CLI 可选命令

如果你使用的是当前电脑上的 Arduino IDE，CLI 路径可能是：

```text
D:\software\ArduinoIDE\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe
```

安装 ESP32 2.0.17：

```powershell
& 'D:\software\ArduinoIDE\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' core install esp32:esp32@2.0.17
```

安装库：

```powershell
& 'D:\software\ArduinoIDE\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' lib install 'Adafruit GFX Library'
& 'D:\software\ArduinoIDE\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' lib install 'Adafruit ST7735 and ST7789 Library'
```

编译验证：

```powershell
& 'D:\software\ArduinoIDE\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc,CPUFreq=160,UploadSpeed=115200 'D:\codespace\clawd-mochi\clawd_mochi'
```

上传：

```powershell
& 'D:\software\ArduinoIDE\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' upload -p COM7 --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc,CPUFreq=160,UploadSpeed=115200 'D:\codespace\clawd-mochi\clawd_mochi'
```

如果你的端口不是 `COM7`，请在 Arduino IDE 的 `Tools -> Port` 或 Windows 设备管理器里确认实际端口。
