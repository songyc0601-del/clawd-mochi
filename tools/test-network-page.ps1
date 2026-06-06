$source = Get-Content (Join-Path $PSScriptRoot "..\clawd_mochi\clawd_mochi.ino") -Raw

$required = @(
  '#include <Preferences.h>',
  'WiFi.mode(WIFI_AP_STA)',
  'void startSetupAp()',
  'server.on("/network"',
  'server.on("/wifi/scan"',
  'server.on("/wifi/connect"',
  'server.on("/wifi/clear"',
  '\"wifiConnected\"',
  '\"wifiIp\"',
  "location.href='/network'"
)

foreach ($text in $required) {
  if (-not $source.Contains($text)) {
    throw "Missing required network content: $text"
  }
}

$apStart = $source.IndexOf("void startSetupAp()")
$apEnd = $source.IndexOf("void restartSetupAp()", $apStart)
$apSource = $source.Substring($apStart, $apEnd - $apStart)
if ($apSource.IndexOf("WiFi.softAP(AP_SSID, AP_PASS)") -gt $apSource.IndexOf("WiFi.softAPConfig(")) {
  throw "SoftAP must start before its DHCP configuration is applied"
}

$stateStart = $source.IndexOf("String stateJson()")
$stateEnd = $source.IndexOf("void routeState()", $stateStart)
if ($stateStart -lt 0 -or $stateEnd -lt 0) {
  throw "Unable to find stateJson()"
}

$stateSource = $source.Substring($stateStart, $stateEnd - $stateStart)
if ($stateSource -match '(?i)password|STA_PASS|wifiPassword') {
  throw "State JSON must not expose the WiFi password"
}

$clearStart = $source.IndexOf("void routeWifiClear()")
$clearEnd = $source.IndexOf("void drawOtaStatus", $clearStart)
if ($clearStart -lt 0 -or $clearEnd -lt 0) {
  throw "Unable to find routeWifiClear()"
}

$clearSource = $source.Substring($clearStart, $clearEnd - $clearStart)
foreach ($text in @("WiFi.disconnect(false, true)", "restartSetupAp()")) {
  if (-not $clearSource.Contains($text)) {
    throw "WiFi clear must restore hotspot DHCP: $text"
  }
}

Write-Host "Network page contract passed"
