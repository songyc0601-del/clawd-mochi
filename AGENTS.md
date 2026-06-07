# Codex 项目说明

## Clawd Mochi 状态屏

Clawd Mochi 状态同步应由运行在 Windows、macOS、Linux 或 WSL 上的客户端 watcher、hook、wrapper 或定时任务负责。

不要依赖模型在日常工作中记得主动推送 `PLAN / CODE / TEST / DONE`。只有在明确有用时，模型才可以把下面的脚本作为手动诊断或一次性状态检查入口。

WSL Ubuntu 下的手动状态命令：

```bash
./tools/codex-stage.sh -State PLAN -Message planning
./tools/codex-stage.sh -State CODE -Message editing
./tools/codex-stage.sh -State TEST -Message verifying
./tools/codex-stage.sh -State BLOCK -Message need-input
./tools/codex-stage.sh -State DONE -Message complete
```

Windows 下继续使用现有 PowerShell 脚本进行手动状态推送：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\codex-stage.ps1 -State PLAN -Message planning
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\codex-stage.ps1 -State CODE -Message editing
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\codex-stage.ps1 -State TEST -Message verifying
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\codex-stage.ps1 -State BLOCK -Message need-input
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\codex-stage.ps1 -State DONE -Message complete
```

- 状态同步是 best-effort。设备未连接时继续任务，不要把它当作阻塞。
- 客户端状态同步方案见 `docs/codex-client-status-sync.zh-CN.md`。
- 自动 watcher 脚本为 `tools/codex-watch.sh` 和 `tools/codex-watch.ps1`。

## 默认编译命令

默认在 WSL Ubuntu 中使用 Linux 版 `arduino-cli`：

```bash
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc,CPUFreq=160,UploadSpeed=115200 clawd_mochi
```

ESP32 core 仍在下载或缺失时，不要运行 Arduino 编译。保留 Windows PowerShell 脚本，用于 Windows 专属流程和回退排查。
