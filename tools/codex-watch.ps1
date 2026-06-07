param(
  [string]$SessionsDir = $(Join-Path $env:USERPROFILE ".codex\sessions"),
  [string]$DeviceUrl = "http://192.168.4.1",
  [string]$StageScript = $(Join-Path $PSScriptRoot "codex-stage.ps1"),
  [int]$IntervalSeconds = 2,
  [int]$DoneAfterSeconds = 20,
  [int]$IdleAfterSeconds = 300,
  [int]$HeartbeatSeconds = 60,
  [switch]$Once,
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
    Push-State -State OFFLINE -Message "codex-offline"
  }
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
      Push-State -State OFFLINE -Message "codex-offline"
    }
    return
  }

  $now = [DateTime]::UtcNow
  $age = ($now - $latest.LastWriteTimeUtc).TotalSeconds

  if ($latest.FullName -ne $script:lastFile) {
    $script:lastFile = $latest.FullName
    $script:lastWriteUtc = $latest.LastWriteTimeUtc
    $script:lastActivityUtc = $latest.LastWriteTimeUtc
    $script:doneSent = $false
    $script:idleSent = $false

    if ($age -le $DoneAfterSeconds) {
      Push-State -State PLAN -Message "codex-session"
    }
    elseif ($age -lt $IdleAfterSeconds) {
      Push-State -State DONE -Message "turn-complete"
      $script:doneSent = $true
    }
    else {
      Push-State -State OFFLINE -Message "codex-offline"
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

    if ($script:currentState -ne "CODE") {
      Push-State -State CODE -Message "active"
    }
    return
  }

  $inactiveSeconds = ($now - $script:lastActivityUtc).TotalSeconds
  if ($script:currentState -ne "BLOCK" -and $inactiveSeconds -ge $IdleAfterSeconds -and -not $script:idleSent) {
    Push-State -State IDLE -Message "codex-ready"
    $script:idleSent = $true
    return
  }

  if ($script:currentState -ne "BLOCK" -and $inactiveSeconds -ge $DoneAfterSeconds -and -not $script:doneSent) {
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
