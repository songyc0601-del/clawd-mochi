param(
  [string]$SessionsDir = $(Join-Path $env:USERPROFILE ".claude\projects"),
  [string]$DeviceUrl = "http://192.168.4.1",
  [string]$StageScript = $(Join-Path $PSScriptRoot "codex-stage.ps1"),
  [int]$IntervalSeconds = 2,
  [int]$DoneAfterSeconds = 20,
  [int]$IdleAfterSeconds = 300,
  [int]$HeartbeatSeconds = 60,
  [switch]$Once,
  [switch]$Background,
  [switch]$Stop,
  [switch]$Status,
  [string]$PidFile = $(Join-Path $env:TEMP "clawd-mochi-claude-watch.pid"),
  [string]$LogFile = $(Join-Path $env:LOCALAPPDATA "clawd-mochi\claude-watch.log"),
  [switch]$VerboseLog
)

$watchScript = Join-Path $PSScriptRoot "codex-watch.ps1"

$arguments = @{
  SessionsDir = $SessionsDir
  DeviceUrl = $DeviceUrl
  StageScript = $StageScript
  ClientSlug = "claude"
  IntervalSeconds = $IntervalSeconds
  DoneAfterSeconds = $DoneAfterSeconds
  IdleAfterSeconds = $IdleAfterSeconds
  HeartbeatSeconds = $HeartbeatSeconds
  PidFile = $PidFile
  LogFile = $LogFile
}

if ($Once) { $arguments.Once = $true }
if ($Background) { $arguments.Background = $true }
if ($Stop) { $arguments.Stop = $true }
if ($Status) { $arguments.Status = $true }
if ($VerboseLog) { $arguments.VerboseLog = $true }

& $watchScript @arguments
