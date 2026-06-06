# OTA 在线升级说明

Clawd Mochi 支持通过浏览器上传 `.bin` 固件进行在线升级。这个功能适合已经完成首次 USB 烧录后的后续升级。

## 重要边界

- 第一次安装仍然需要 USB 烧录。
- OTA 可以通过设备局域网 IP 或设备自己的 WiFi 热点使用，不需要互联网。
- 升级过程中不要断电。
- 上传文件必须是 ESP32-C3 对应的 `.bin` 固件，不要上传 `.ino`、`.zip` 或其他文件。

## 生成固件 bin

在项目根目录运行：

```powershell
& 'D:\software\ArduinoIDE\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc,CPUFreq=160,UploadSpeed=115200 'D:\codespace\clawd-mochi\clawd_mochi'
```

Arduino CLI 会在构建缓存中生成 `.bin`。如果你用 Arduino IDE，也可以使用导出编译二进制文件功能。

## 上传升级

1. 电脑保持连接原有 WiFi，确认它与 Clawd Mochi 在同一个局域网。
2. 使用屏幕或网络设置页显示的局域网 IP 打开：

```text
http://设备局域网IP/ota
```

也可以用手机连接 `ClaWD-Mochi`，打开兜底地址：

```text
http://192.168.4.1/ota
```

3. 选择生成的 `.bin` 文件。
4. 点击上传并升级。
5. 等待设备自动重启。

升级时屏幕会显示：

```text
OTA UPDATING
OTA REBOOTING
```

如果失败，会显示：

```text
OTA FAILED
```

## 排错

- 页面打不开：确认电脑和设备连接同一个局域网，或连接 `ClaWD-Mochi` 后使用 `http://192.168.4.1/ota`。
- 上传失败：确认文件是为 ESP32-C3 编译出的 `.bin`。
- 上传中断：重新给设备上电，然后再次打开 OTA 页面上传。
- Windows 电脑无需连接设备热点，可直接使用设备的局域网 IP。
