param(
  [ValidateSet("IDLE", "PLAN", "CODE", "TEST", "DONE", "BLOCK", "STATE")]
  [string]$State = "STATE",
  [string]$Message = "",
  [string]$DeviceUrl = "http://192.168.4.1",
  [string]$Port = "COM7"
)

$cleanMessage = ($Message -replace '[^\x20-\x7E]', '').Trim()

try {
  if ($State -eq "STATE") {
    $response = Invoke-RestMethod -Uri "$DeviceUrl/state" -TimeoutSec 2
  } else {
    $query = "state=$([Uri]::EscapeDataString($State))&msg=$([Uri]::EscapeDataString($cleanMessage))"
    $response = Invoke-RestMethod -Uri "$DeviceUrl/progress?$query" -TimeoutSec 2
  }
  $response
  exit 0
}
catch {
  $progressScript = Join-Path $PSScriptRoot "codex-progress.ps1"
  & $progressScript -Port $Port -State $State -Message $cleanMessage
}
