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
powershell -ExecutionPolicy Bypass -File .\tools\codex-progress.ps1 -Port COM7 -State OFFLINE -Message codex-offline
```

`OFFLINE` 表示 Codex 离线，设备回到默认 Clawd 动画形象，并在 `/state` 中保留 `progressMsg` 作为原因，例如 `codex-offline` 或 `codex-timeout`。

`IDLE` 表示 Codex 已在线但暂无任务，会显示 Codex 核心低亮待机屏。其他非离线状态会自动切到 Codex 核心脉冲状态屏：

```text
OFFLINE
IDLE
PLAN
CODE
TEST
DONE
BLOCK
```

`PLAN / CODE / TEST / DONE / BLOCK` 使用状态色、英文状态词和 4 段阶段条；`BLOCK` 为红色满格。`OFFLINE` 不会被固件记为用户活动，其他进度状态可以记为活动。

## 自动对接当前 Codex

推荐的自动对接方式是在客户端侧启动 watcher、hook、wrapper 或定时任务，自动观察 Codex 运行状态并推送到设备，而不是依赖模型在工作过程中手动调用脚本。

客户端侧状态映射建议：

- 开始分析或规划：`PLAN`
- 开始修改文件：`CODE`
- 开始编译、烧录或验证：`TEST`
- 完成：`DONE`
- 确实需要用户输入：`BLOCK`
- Codex 未打开、没有 session、watcher 停止或客户端离线：`OFFLINE`

Windows 手动推送或 hook 推送可以使用 `tools/codex-stage.ps1`。默认优先访问设备热点地址 `http://192.168.4.1`，WiFi 不可用时自动回退到 `COM7` USB 串口。电脑和设备连接同一个局域网时，可以用 `-DeviceUrl http://设备局域网IP` 直接推送，无需电脑加入设备热点。

Codex Desktop 的 `notify` 已配置为调用 `tools/codex-notify.ps1`。每个 Codex 工作回合结束时，它会自动推送 `DONE turn-complete`，同时继续调用原有 Computer Use 通知程序。设备未连接时通知脚本会静默跳过，不影响 Codex。

Codex Desktop 目前不公开内部 `PLAN / CODE / TEST` 阶段事件，因此这些状态应由客户端 watcher、wrapper、日志观察或后续官方事件接口推断。不要把模型手动推送作为长期方案。

更完整的客户端方案见 [Codex 客户端状态同步方案](codex-client-status-sync.zh-CN.md)。

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
PROGRESS OFFLINE codex-offline
PROGRESS CODE editing
```

## 和 WiFi 控制的关系

- WiFi 网页控制保留，适合手机操作。
- USB 串口控制新增，适合电脑自动化和 Codex 进度推送。
- 两种方式可以交替使用。
- 如果串口推送了非 `OFFLINE` Codex 进度，屏幕会自动切到 Codex 状态屏；推送 `PROGRESS OFFLINE` 会回到默认 Clawd 动画形象。手机网页仍然可以切回正常、专注或开心表情。
- 已删除串口表情、自动待机、速度、终端和画板命令；这些命令会返回 `ERR unknown-command`。
