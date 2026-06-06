param(
  [Parameter(ValueFromRemainingArguments = $true)]
  [string[]]$EventArgs
)

$projectRoot = Split-Path -Parent $PSScriptRoot
$progressScript = Join-Path $projectRoot "tools\codex-stage.ps1"

try {
  & $progressScript -State DONE -Message turn-complete | Out-Null
}
catch {
  # Codex notifications must not fail when the board is disconnected.
}

$notifier = Get-ChildItem "$env:USERPROFILE\.codex\plugins\cache\openai-bundled\computer-use\*\node_modules\@oai\sky\bin\windows\codex-computer-use.exe" -ErrorAction SilentlyContinue |
  Sort-Object LastWriteTime -Descending |
  Select-Object -First 1

if ($notifier) {
  & $notifier.FullName "turn-ended" @EventArgs
}
