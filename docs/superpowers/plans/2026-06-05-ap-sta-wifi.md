# AP + STA WiFi Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Allow Clawd Mochi to join an existing 2.4 GHz WiFi while keeping its setup hotspot available.

**Architecture:** Run ESP32 WiFi in `WIFI_AP_STA` mode and store STA credentials in `Preferences`. Add a small network configuration page and local HTTP endpoints, while retaining all existing control, OTA, USB, and Codex behavior.

**Tech Stack:** Arduino ESP32, `WiFi.h`, `WebServer.h`, `Preferences.h`, PowerShell contract tests, Arduino CLI

---

### Task 1: Define the network feature contract

**Files:**
- Create: `tools/test-network-page.ps1`
- Test: `clawd_mochi/clawd_mochi.ino`

- [ ] **Step 1: Write the failing static contract test**

Create a PowerShell test that reads the firmware source and requires:

```powershell
$required = @(
  '#include <Preferences.h>',
  'WiFi.mode(WIFI_AP_STA)',
  'server.on("/network"',
  'server.on("/wifi/scan"',
  'server.on("/wifi/connect"',
  'server.on("/wifi/clear"',
  '"wifiConnected"',
  '"wifiIp"',
  'href="/network"'
)
```

It must also reject any state JSON construction that includes the stored WiFi password.

- [ ] **Step 2: Run the test and verify it fails**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\test-network-page.ps1
```

Expected: failure stating that `Preferences.h` or `/network` is missing.

- [ ] **Step 3: Commit the failing test**

```powershell
git add tools/test-network-page.ps1
git commit -m "test: define AP and STA network contract"
```

### Task 2: Implement AP + STA networking and persistence

**Files:**
- Modify: `clawd_mochi/clawd_mochi.ino`
- Test: `tools/test-network-page.ps1`

- [ ] **Step 1: Add minimal networking state**

Add `Preferences`, saved SSID/password variables, connection state helpers, and a bounded startup connection attempt. Start networking with:

```cpp
WiFi.mode(WIFI_AP_STA);
WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
WiFi.softAP(AP_SSID, AP_PASS);
```

Load credentials from a `clawd-wifi` Preferences namespace. Keep AP running whether STA succeeds or fails.

- [ ] **Step 2: Add network page and HTTP handlers**

Add:

```text
GET  /network
GET  /wifi/scan
POST /wifi/connect
POST /wifi/clear
```

`/wifi/connect` accepts `ssid` and `password`, rejects an empty SSID, saves credentials, and starts STA connection. `/wifi/clear` erases credentials and disconnects STA without stopping AP.

- [ ] **Step 3: Extend state and homepage**

Add `wifiMode`, `wifiConnected`, `wifiSsid`, `wifiIp`, and `apIp` to `/state`. Add a single “网络设置” link to the homepage device controls. Never return the saved password.

- [ ] **Step 4: Run contract tests**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\test-network-page.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\test-web-page.ps1
```

Expected: both tests pass.

### Task 3: Update documentation and distributable firmware

**Files:**
- Modify: `README.zh-CN.md`
- Modify: `docs/ota-update.zh-CN.md`
- Modify: `dist/clawd_mochi/clawd_mochi.ino`

- [ ] **Step 1: Document first-time and daily use**

Explain that first-time setup uses `ClaWD-Mochi`, the device only supports 2.4 GHz WiFi, and daily use uses the displayed LAN IP without disconnecting the computer from its normal WiFi.

- [ ] **Step 2: Document recovery and OTA**

Explain that the AP always remains available, credentials can be cleared from `/network`, and OTA works from either the AP address or LAN IP.

- [ ] **Step 3: Copy the verified main firmware to dist**

```powershell
Copy-Item .\clawd_mochi\clawd_mochi.ino .\dist\clawd_mochi\clawd_mochi.ino -Force
```

### Task 4: Compile, upload, and verify hardware behavior

**Files:**
- Verify: `clawd_mochi/clawd_mochi.ino`
- Verify: `dist/clawd_mochi/clawd_mochi.ino`

- [ ] **Step 1: Compile both firmware copies**

Compile with:

```text
esp32:esp32:esp32c3:CDCOnBoot=cdc,CPUFreq=160,UploadSpeed=115200
```

Expected: both compile successfully.

- [ ] **Step 2: Confirm firmware copies match**

Run `Get-FileHash` for both `.ino` files.

Expected: identical SHA256 hashes.

- [ ] **Step 3: Upload to COM7**

Upload the main sketch with Arduino CLI.

Expected: flash verification and hard reset succeed.

- [ ] **Step 4: Verify hotspot fallback**

Request:

```text
http://192.168.4.1/
http://192.168.4.1/network
http://192.168.4.1/state
http://192.168.4.1/wifi/scan
```

Expected: all endpoints respond; `/state` includes the new fields and contains no password.

- [ ] **Step 5: Verify STA connection flow**

Use the network page to save a known 2.4 GHz WiFi. Confirm `/state` reports `wifiConnected: true` and a LAN IP, then access `/`, `/ota`, and `/state` through that LAN IP. Clear credentials and confirm hotspot access remains available.

