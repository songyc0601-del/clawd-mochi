$ErrorActionPreference = "Stop"

$firmware = Join-Path $PSScriptRoot "..\clawd_mochi\clawd_mochi.ino"
$source = Get-Content -Raw -Encoding UTF8 $firmware
$start = $source.IndexOf("const char INDEX_HTML_LITE[]")
$end = $source.IndexOf("const char OTA_HTML[]", $start)

if ($start -lt 0 -or $end -lt 0) {
  throw "Embedded web pages not found"
}

$page = $source.Substring($start, $end - $start)

$required = @(
  'id="progress"',
  'id="source"',
  "cmd('normal')",
  "expr('focus')",
  "expr('happy')",
  'id="blBtn"',
  "location.href='/ota'",
  "setInterval(refresh,5000)"
)

$forbidden = @(
  "onclick=`"progress(",
  "async function progress(",
  "Codex desktop status screen",
  "onclick=`"refresh()"
)

foreach ($text in $required) {
  if (-not $page.Contains($text)) {
    throw "Missing required page content: $text"
  }
}

foreach ($text in $forbidden) {
  if ($page.Contains($text)) {
    throw "Forbidden page content found: $text"
  }
}

Write-Output "Web page contract passed"
