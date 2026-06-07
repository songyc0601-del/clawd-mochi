param(
  [string]$SessionsDir = $(Join-Path $env:USERPROFILE ".codex\sessions"),
  [string]$DeviceUrl = "http://192.168.4.1",
  [string]$StageScript = $(Join-Path $PSScriptRoot "codex-stage.ps1"),
  [string]$ClientSlug = "codex",
  [int]$IntervalSeconds = 2,
  [int]$DoneAfterSeconds = 20,
  [int]$IdleAfterSeconds = 300,
  [int]$HeartbeatSeconds = 60,
  [switch]$Once,
  [switch]$Background,
  [switch]$Stop,
  [switch]$Status,
  [string]$PidFile = $(Join-Path $env:TEMP "clawd-mochi-codex-watch.pid"),
  [string]$LogFile = $(Join-Path $env:LOCALAPPDATA "clawd-mochi\codex-watch.log"),
  [switch]$VerboseLog
)

$lastFile = $null
$lastWriteUtc = $null
$lastActivityUtc = $null
$currentState = ""
$currentMessage = ""
$lastHeartbeatUtc = [DateTime]::MinValue
$doneSent = $false
$idleSent = $false

function Write-WatcherLog {
  param([string]$Message)
  if ($VerboseLog) {
    Write-Host "[codex-watch] $Message"
  }
}

function Get-LatestSessionFile {
  if (-not (Test-Path -LiteralPath $SessionsDir)) {
    return $null
  }

  Get-ChildItem -LiteralPath $SessionsDir -Recurse -Filter "*.jsonl" -File -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First 1
}

function Test-SessionHasLifecycleEvents {
  param([System.IO.FileInfo]$File)
  try {
    Select-String -LiteralPath $File.FullName -Pattern '"type":"(task_started|task_complete|user_message)"' -Quiet -ErrorAction Stop
  }
  catch {
    return $false
  }
}

function Test-SessionComplete {
  param([System.IO.FileInfo]$File)
  $startedLine = 0
  $completeLine = 0
  $lineNumber = 0

  try {
    foreach ($line in [System.IO.File]::ReadLines($File.FullName)) {
      $lineNumber++
      if ($line -match '"type":"task_started"' -or $line -match '"type":"user_message"') {
        $startedLine = $lineNumber
      }
      if ($line -match '"type":"task_complete"') {
        $completeLine = $lineNumber
      }
    }
  }
  catch {
    return $false
  }

  return $completeLine -gt $startedLine
}

function Push-State {
  param(
    [ValidateSet("OFFLINE", "IDLE", "PLAN", "CODE", "TEST", "DONE", "BLOCK")]
    [string]$State,
    [string]$Message
  )

  Write-WatcherLog "push $State $Message"
  try {
    & $StageScript -DeviceUrl $DeviceUrl -State $State -Message $Message | Out-Null
  }
  catch {
    # Device sync is best-effort and must not break Codex.
  }
  $script:currentState = $State
  $script:currentMessage = $Message
  $script:lastHeartbeatUtc = [DateTime]::UtcNow
}

function Get-OnlineMessage {
  param([ValidateSet("session", "ready", "offline")][string]$Kind)
  switch ($Kind) {
    "session" { return "$ClientSlug-session" }
    "ready" { return "$ClientSlug-ready" }
    "offline" { return "$ClientSlug-offline" }
  }
}

function Send-Heartbeat {
  param([DateTime]$Now)
  if ([string]::IsNullOrEmpty($script:currentState)) {
    return
  }
  if (($Now - $script:lastHeartbeatUtc).TotalSeconds -lt $HeartbeatSeconds) {
    return
  }
  Push-State -State $script:currentState -Message $script:currentMessage
}

function Invoke-Cleanup {
  if (-not $Once) {
    Push-State -State OFFLINE -Message (Get-OnlineMessage -Kind offline)
  }

  if (Test-Path -LiteralPath $PidFile) {
    $pidText = Get-Content -LiteralPath $PidFile -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($pidText -eq "$PID") {
      Remove-Item -LiteralPath $PidFile -Force -ErrorAction SilentlyContinue
    }
  }
}

function Test-WatcherPidRunning {
  param([string]$PidText)
  if ([string]::IsNullOrWhiteSpace($PidText)) {
    return $false
  }
  try {
    Get-Process -Id ([int]$PidText) -ErrorAction Stop | Out-Null
    return $true
  }
  catch {
    return $false
  }
}

function Get-WatcherPid {
  if (-not (Test-Path -LiteralPath $PidFile)) {
    return $null
  }
  Get-Content -LiteralPath $PidFile -ErrorAction SilentlyContinue | Select-Object -First 1
}

function Start-BackgroundWatcher {
  $existingPid = Get-WatcherPid
  if (Test-WatcherPidRunning -PidText $existingPid) {
    Write-Host "codex-watch already running: pid $existingPid"
    return
  }

  if ($Once) {
    throw "-Background cannot be used with -Once"
  }

  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $PidFile) | Out-Null
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $LogFile) | Out-Null

  $arguments = @(
    "-NoProfile",
    "-ExecutionPolicy", "Bypass",
    "-File", $PSCommandPath,
    "-SessionsDir", $SessionsDir,
    "-DeviceUrl", $DeviceUrl,
    "-StageScript", $StageScript,
    "-ClientSlug", $ClientSlug,
    "-IntervalSeconds", "$IntervalSeconds",
    "-DoneAfterSeconds", "$DoneAfterSeconds",
    "-IdleAfterSeconds", "$IdleAfterSeconds",
    "-HeartbeatSeconds", "$HeartbeatSeconds",
    "-PidFile", $PidFile,
    "-LogFile", $LogFile
  )
  if ($VerboseLog) {
    $arguments += "-VerboseLog"
  }

  $process = Start-Process -FilePath "powershell" -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput $LogFile -RedirectStandardError "$LogFile.err"
  Set-Content -LiteralPath $PidFile -Value $process.Id
  Write-Host "codex-watch started: pid $($process.Id), log $LogFile"
}

function Stop-BackgroundWatcher {
  $watcherPid = Get-WatcherPid
  if (-not (Test-WatcherPidRunning -PidText $watcherPid)) {
    Remove-Item -LiteralPath $PidFile -Force -ErrorAction SilentlyContinue
    Write-Host "codex-watch is not running"
    return
  }

  try {
    & $StageScript -DeviceUrl $DeviceUrl -State OFFLINE -Message (Get-OnlineMessage -Kind offline) | Out-Null
  }
  catch {
    # Device sync is best-effort and must not break watcher shutdown.
  }
  Stop-Process -Id ([int]$watcherPid) -ErrorAction SilentlyContinue
  Remove-Item -LiteralPath $PidFile -Force -ErrorAction SilentlyContinue
  Write-Host "codex-watch stopping"
}

function Show-BackgroundStatus {
  $watcherPid = Get-WatcherPid
  if (Test-WatcherPidRunning -PidText $watcherPid) {
    Write-Host "codex-watch running: pid $watcherPid"
    return
  }
  Write-Host "codex-watch not running"
  exit 1
}

if ($Stop) {
  Stop-BackgroundWatcher
  exit 0
}

if ($Status) {
  Show-BackgroundStatus
  exit 0
}

if ($Background) {
  Start-BackgroundWatcher
  exit 0
}

function Invoke-PollOnce {
  $latest = Get-LatestSessionFile

  if ($null -eq $latest) {
    $script:lastFile = $null
    $script:lastWriteUtc = $null
    $script:lastActivityUtc = $null
    $script:doneSent = $false
    $script:idleSent = $false
    if ($script:currentState -ne "OFFLINE") {
      Push-State -State OFFLINE -Message (Get-OnlineMessage -Kind offline)
    }
    return
  }

  $now = [DateTime]::UtcNow
  $age = ($now - $latest.LastWriteTimeUtc).TotalSeconds
  $hasLifecycle = Test-SessionHasLifecycleEvents -File $latest
  $isComplete = $false
  if ($hasLifecycle) {
    $isComplete = Test-SessionComplete -File $latest
  }

  if ($latest.FullName -ne $script:lastFile) {
    $script:lastFile = $latest.FullName
    $script:lastWriteUtc = $latest.LastWriteTimeUtc
    $script:lastActivityUtc = $latest.LastWriteTimeUtc
    $script:doneSent = $false
    $script:idleSent = $false

    if ($hasLifecycle -and $isComplete -and $age -lt $IdleAfterSeconds) {
      Push-State -State DONE -Message "turn-complete"
      $script:doneSent = $true
    }
    elseif ($age -le $DoneAfterSeconds) {
      Push-State -State PLAN -Message (Get-OnlineMessage -Kind session)
    }
    elseif (-not $hasLifecycle -and $age -lt $IdleAfterSeconds) {
      Push-State -State DONE -Message "turn-complete"
      $script:doneSent = $true
    }
    elseif ($age -lt $IdleAfterSeconds) {
      Push-State -State CODE -Message "active"
    }
    else {
      Push-State -State OFFLINE -Message (Get-OnlineMessage -Kind offline)
      $script:doneSent = $true
      $script:idleSent = $true
    }
    return
  }

  if ($latest.LastWriteTimeUtc -ne $script:lastWriteUtc) {
    $script:lastWriteUtc = $latest.LastWriteTimeUtc
    $script:lastActivityUtc = $latest.LastWriteTimeUtc
    $script:doneSent = $false
    $script:idleSent = $false

    if ($hasLifecycle -and $isComplete) {
      Push-State -State DONE -Message "turn-complete"
      $script:doneSent = $true
      return
    }

    if ($script:currentState -ne "CODE") {
      Push-State -State CODE -Message "active"
    }
    return
  }

  $inactiveSeconds = ($now - $script:lastActivityUtc).TotalSeconds
  if ($script:currentState -ne "BLOCK" -and $inactiveSeconds -ge $IdleAfterSeconds -and -not $script:idleSent) {
    Push-State -State IDLE -Message (Get-OnlineMessage -Kind ready)
    $script:idleSent = $true
    return
  }

  if ($script:currentState -ne "BLOCK" -and $hasLifecycle -and $isComplete -and -not $script:doneSent) {
    Push-State -State DONE -Message "turn-complete"
    $script:doneSent = $true
    return
  }

  if ($script:currentState -ne "BLOCK" -and -not $hasLifecycle -and $inactiveSeconds -ge $DoneAfterSeconds -and -not $script:doneSent) {
    Push-State -State DONE -Message "turn-complete"
    $script:doneSent = $true
    return
  }

  Send-Heartbeat -Now $now
}

try {
  while ($true) {
    Invoke-PollOnce
    if ($Once) {
      break
    }
    Start-Sleep -Seconds $IntervalSeconds
  }
}
finally {
  Invoke-Cleanup
}
