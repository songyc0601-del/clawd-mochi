# USB 串口与 Codex 进度说明

本项目除了 WiFi 网页控制，也支持通过 USB 串口从电脑发送命令。这个方式适合 Codex 工作进度对接：电脑负责知道当前工作阶段，ESP32 负责显示。

## 连接步骤

1. 用 USB-C 数据线连接 ESP32-C3。
2. 确认 Arduino IDE 里能看到端口，例如 `COM7`。
3. 确保串口监视器已关闭，避免占用端口。
4. 在项目根目录运行 PowerShell 脚本。

## 推送 Codex 进度

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\codex-progress.ps1 -Port COM7 -State IDLE
powershell -ExecutionPolicy Bypass -File .\tools\codex-progress.ps1 -Port COM7 -State PLAN -Message planning
powershell -ExecutionPolicy Bypass -File .\tools\codex-progress.ps1 -Port COM7 -State CODE -Message editing
powershell -ExecutionPolicy Bypass -File .\tools\codex-progress.ps1 -Port COM7 -State TEST -Message verifying
powershell -ExecutionPolicy Bypass -File .\tools\codex-progress.ps1 -Port COM7 -State DONE -Message complete
powershell -ExecutionPolicy Bypass -File .\tools\codex-progress.ps1 -Port COM7 -State BLOCK -Message need-input
```

`IDLE` 返回 `CODEX` + Clawd 眼睛待机页。其他状态会自动切到任务页：

```text
IDLE
PLAN
CODE
TEST
DONE
BLOCK
```

`PLAN / CODE / TEST` 为闪烁黄灯，并显示 1/4、2/4、3/4 阶段条；`DONE` 为绿灯和 4/4；`BLOCK` 为红灯。

## 自动对接当前 Codex

项目根目录的 `AGENTS.md` 要求 Codex 在工作过程中主动推送阶段状态：

- 开始分析或规划：`PLAN`
- 开始修改文件：`CODE`
- 开始编译、烧录或验证：`TEST`
- 完成：`DONE`
- 确实需要用户输入：`BLOCK`

阶段推送使用 `tools/codex-stage.ps1`。默认优先访问设备热点地址 `http://192.168.4.1`，WiFi 不可用时自动回退到 `COM7` USB 串口。电脑和设备连接同一个局域网时，可以用 `-DeviceUrl http://设备局域网IP` 直接推送，无需电脑加入设备热点。

Codex Desktop 的 `notify` 已配置为调用 `tools/codex-notify.ps1`。每个 Codex 工作回合结束时，它会自动推送 `DONE turn-complete`，同时继续调用原有 Computer Use 通知程序。设备未连接时通知脚本会静默跳过，不影响 Codex。

Codex Desktop 目前不公开内部 `PLAN / CODE / TEST` 阶段事件，因此这些阶段由 Codex 在执行过程中或用户通过 `codex-progress.ps1` 主动推送，避免显示错误阶段。

首次配置通知后建议重启一次 Codex Desktop，确保新的 `notify` 配置被当前应用进程加载。

项目级 `AGENTS.md` 通常在新任务开始时加载。已经运行中的旧任务不会自动回放之前错过的阶段。

## 查询状态

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\codex-progress.ps1 -Port COM7 -State STATE
```

返回示例：

```json
{"view":4,"busy":false,"term":false,"bl":true,"speed":1,"progress":"CODE","progressMsg":"editing"}
```

## 手动串口命令

也可以用 Arduino IDE 串口监视器发送命令，波特率设置为 `115200`，行尾选择“新行”。

```text
STATE
CMD normal
BL 0
BL 1
PROGRESS IDLE
PROGRESS CODE editing
```

## 和 WiFi 控制的关系

- WiFi 网页控制保留，适合手机操作。
- USB 串口控制新增，适合电脑自动化和 Codex 进度推送。
- 两种方式可以交替使用。
- 如果串口推送了 Codex 进度，屏幕会自动切到进度页；手机网页仍然可以切回正常、专注或开心表情。
- 已删除串口表情、自动待机、速度、终端和画板命令；这些命令会返回 `ERR unknown-command`。
