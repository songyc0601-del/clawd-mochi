/*
 * ╔══════════════════════════════════════════════════════════════╗
 *   CLAWD MOCHI — ESP32-C3 Super Mini + ST7789 1.54" 240×240
 *
 *   Wiring:
 *     SDA → GPIO 10  (hardware SPI MOSI)
 *     SCL → GPIO 8   (hardware SPI SCK)
 *     RST → GPIO 2
 *     DC  → GPIO 1
 *     CS  → GPIO 4
 *     BL  → GPIO 3
 *     VCC → 3V3
 *     GND → GND
 *
 *   WiFi: "ClaWD-Mochi"  pw: clawd1234  → http://192.168.4.1
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <Preferences.h>

// ── Pins ──────────────────────────────────────────────────────
#define TFT_CS  4
#define TFT_DC  1
#define TFT_RST 2
#define TFT_BLK 3

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ── WiFi ──────────────────────────────────────────────────────
const char* AP_SSID = "ClaWD-Mochi";
const char* AP_PASS = "clawd1234";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);
WebServer server(80);
Preferences wifiPrefs;
String savedWifiSsid = "";
String savedWifiPassword = "";

// ── Display ───────────────────────────────────────────────────
#define DISP_W 240
#define DISP_H 240

// ── Eye constants (shared by both eye views) ──────────────────
#define EYE_W   30
#define EYE_H   60
#define EYE_GAP 120
#define EYE_OX  0     // horizontal offset
#define EYE_OY  40    // vertical offset upward (subtracted from centre)

// ── Colours ───────────────────────────────────────────────────
uint16_t C_ORANGE, C_DARKBG, C_MUTED, C_GREEN;
#define C_WHITE ST77XX_WHITE
#define C_BLACK ST77XX_BLACK

// ── State ─────────────────────────────────────────────────────
#define VIEW_EYES_NORMAL 0
#define VIEW_EYES_SQUISH 1
#define VIEW_CODE        2
#define VIEW_DRAW        3
#define VIEW_PROGRESS    4
#define VIEW_COMPANION   5

#define EXPR_FOCUS       0
#define EXPR_HAPPY       1
#define EXPR_SLEEPY      2
#define EXPR_STARE       3
#define EXPR_BREAK       4

uint8_t  currentView  = VIEW_EYES_NORMAL;
bool     busy         = false;
bool     backlightOn  = true;
uint8_t  animSpeed    = 1;   // 1=slow(default) 2=normal 3=fast
bool     idleEnabled  = true;
uint8_t  companionExpr = EXPR_FOCUS;
uint32_t lastActionMs = 0;
uint32_t lastIdleMs   = 0;
uint32_t lastProgressBlinkMs = 0;
bool     progressBlinkOn = true;

uint16_t animBgColor  = 0;   // background for eye/logo animations
uint16_t drawBgColor  = 0;   // background for canvas
const String PROGRESS_OFFLINE = "OFFLINE";
const String PROGRESS_IDLE    = "IDLE";
const String PROGRESS_PLAN    = "PLAN";
const String PROGRESS_CODE    = "CODE";
const String PROGRESS_TEST    = "TEST";
const String PROGRESS_DONE    = "DONE";
const String PROGRESS_BLOCK   = "BLOCK";

String   progressState = PROGRESS_OFFLINE;
String   progressMsg   = "";
uint8_t  progressPulsePhase = 0;
uint32_t lastCodexProgressMs = 0;

const uint32_t CODEX_OFFLINE_TIMEOUT_MS = 120000UL;
String   serialLine    = "";

// ── Terminal ──────────────────────────────────────────────────
#define TERM_COLS      15
#define TERM_ROWS       8
#define TERM_CHAR_W    12
#define TERM_CHAR_H    20
#define TERM_PAD_X      8
#define TERM_PAD_Y     18

bool    termMode    = false;
String  termLines[TERM_ROWS];
uint8_t termRow     = 0;
uint8_t termCol     = 0;

// ── Logo data ─────────────────────────────────────────────────
#define LOGO_CX 120
#define LOGO_CY 105

#define LOGO_TRI_COUNT 162
static const int16_t LOGO_TRIS[][6] PROGMEM = {
  {120,105,65,134,100,114},{120,105,100,114,101,113},{120,105,101,113,100,112},
  {120,105,100,112,99,112},{120,105,99,112,93,111},{120,105,93,111,73,111},
  {120,105,73,111,55,110},{120,105,55,110,38,109},{120,105,38,109,34,108},
  {120,105,34,108,30,103},{120,105,30,103,30,100},{120,105,30,100,34,98},
  {120,105,34,98,39,98},{120,105,39,98,50,99},{120,105,50,99,67,100},
  {120,105,67,100,80,101},{120,105,80,101,98,103},{120,105,98,103,101,103},
  {120,105,101,103,101,102},{120,105,101,102,100,101},{120,105,100,101,100,100},
  {120,105,100,100,82,88},{120,105,82,88,63,76},{120,105,63,76,53,69},
  {120,105,53,69,48,65},{120,105,48,65,45,61},{120,105,45,61,44,54},
  {120,105,44,54,49,49},{120,105,49,49,55,49},{120,105,55,49,57,49},
  {120,105,57,49,64,55},{120,105,64,55,78,66},{120,105,78,66,96,79},
  {120,105,96,79,99,81},{120,105,99,81,100,81},{120,105,100,81,100,80},
  {120,105,100,80,99,78},{120,105,99,78,89,60},{120,105,89,60,78,41},
  {120,105,78,41,73,34},{120,105,73,34,72,29},{120,105,72,29,72,28},
  {120,105,72,28,72,27},{120,105,72,27,71,26},{120,105,71,26,71,25},
  {120,105,71,25,71,24},{120,105,71,24,77,16},{120,105,77,16,80,15},
  {120,105,80,15,87,16},{120,105,87,16,91,19},{120,105,91,19,95,29},
  {120,105,95,29,103,46},{120,105,103,46,114,68},{120,105,114,68,118,75},
  {120,105,118,75,119,81},{120,105,119,81,120,83},{120,105,120,83,121,83},
  {120,105,121,83,121,82},{120,105,121,82,122,69},{120,105,122,69,124,54},
  {120,105,124,54,126,34},{120,105,126,34,126,28},{120,105,126,28,129,21},
  {120,105,129,21,135,18},{120,105,135,18,139,20},{120,105,139,20,143,25},
  {120,105,143,25,142,28},{120,105,142,28,140,42},{120,105,140,42,136,64},
  {120,105,136,64,133,78},{120,105,133,78,135,78},{120,105,135,78,136,76},
  {120,105,136,76,144,67},{120,105,144,67,156,51},{120,105,156,51,162,45},
  {120,105,162,45,168,38},{120,105,168,38,172,35},{120,105,172,35,180,35},
  {120,105,180,35,185,43},{120,105,185,43,183,52},{120,105,183,52,175,62},
  {120,105,175,62,168,71},{120,105,168,71,159,83},{120,105,159,83,153,94},
  {120,105,153,94,154,94},{120,105,154,94,155,94},{120,105,155,94,176,90},
  {120,105,176,90,188,88},{120,105,188,88,201,85},{120,105,201,85,208,88},
  {120,105,208,88,208,91},{120,105,208,91,206,97},{120,105,206,97,191,101},
  {120,105,191,101,174,104},{120,105,174,104,148,110},{120,105,148,110,148,111},
  {120,105,148,111,148,111},{120,105,148,111,160,112},{120,105,160,112,165,112},
  {120,105,165,112,177,112},{120,105,177,112,200,114},{120,105,200,114,205,118},
  {120,105,205,118,209,123},{120,105,209,123,208,126},{120,105,208,126,199,131},
  {120,105,199,131,187,128},{120,105,187,128,159,121},{120,105,159,121,149,119},
  {120,105,149,119,147,119},{120,105,147,119,147,120},{120,105,147,120,156,128},
  {120,105,156,128,170,141},{120,105,170,141,189,158},{120,105,189,158,190,163},
  {120,105,190,163,188,166},{120,105,188,166,185,166},{120,105,185,166,169,153},
  {120,105,169,153,162,148},{120,105,162,148,148,136},{120,105,148,136,147,136},
  {120,105,147,136,147,137},{120,105,147,137,150,142},{120,105,150,142,168,168},
  {120,105,168,168,169,176},{120,105,169,176,168,179},{120,105,168,179,163,180},
  {120,105,163,180,158,179},{120,105,158,179,148,165},{120,105,148,165,137,149},
  {120,105,137,149,129,134},{120,105,129,134,128,135},{120,105,128,135,123,189},
  {120,105,123,189,120,192},{120,105,120,192,115,194},{120,105,115,194,110,191},
  {120,105,110,191,108,185},{120,105,108,185,110,174},{120,105,110,174,113,160},
  {120,105,113,160,116,148},{120,105,116,148,118,134},{120,105,118,134,119,129},
  {120,105,119,129,119,129},{120,105,119,129,118,129},{120,105,118,129,107,144},
  {120,105,107,144,91,166},{120,105,91,166,78,180},{120,105,78,180,75,181},
  {120,105,75,181,70,178},{120,105,70,178,70,173},{120,105,70,173,73,169},
  {120,105,73,169,91,146},{120,105,91,146,102,132},{120,105,102,132,109,124},
  {120,105,109,124,109,123},{120,105,109,123,108,123},{120,105,108,123,61,153},
  {120,105,61,153,52,155},{120,105,52,155,49,151},{120,105,49,151,49,146},
  {120,105,49,146,51,144},{120,105,51,144,65,134},{120,105,65,134,65,134},
};

#define LOGO_SEG_COUNT 162
static const int16_t LOGO_SEGS[][4] PROGMEM = {
  {65,134,100,114},{100,114,101,113},{101,113,100,112},{100,112,99,112},
  {99,112,93,111},{93,111,73,111},{73,111,55,110},{55,110,38,109},
  {38,109,34,108},{34,108,30,103},{30,103,30,100},{30,100,34,98},
  {34,98,39,98},{39,98,50,99},{50,99,67,100},{67,100,80,101},
  {80,101,98,103},{98,103,101,103},{101,103,101,102},{101,102,100,101},
  {100,101,100,100},{100,100,82,88},{82,88,63,76},{63,76,53,69},
  {53,69,48,65},{48,65,45,61},{45,61,44,54},{44,54,49,49},
  {49,49,55,49},{55,49,57,49},{57,49,64,55},{64,55,78,66},
  {78,66,96,79},{96,79,99,81},{99,81,100,81},{100,81,100,80},
  {100,80,99,78},{99,78,89,60},{89,60,78,41},{78,41,73,34},
  {73,34,72,29},{72,29,72,28},{72,28,72,27},{72,27,71,26},
  {71,26,71,25},{71,25,71,24},{71,24,77,16},{77,16,80,15},
  {80,15,87,16},{87,16,91,19},{91,19,95,29},{95,29,103,46},
  {103,46,114,68},{114,68,118,75},{118,75,119,81},{119,81,120,83},
  {120,83,121,83},{121,83,121,82},{121,82,122,69},{122,69,124,54},
  {124,54,126,34},{126,34,126,28},{126,28,129,21},{129,21,135,18},
  {135,18,139,20},{139,20,143,25},{143,25,142,28},{142,28,140,42},
  {140,42,136,64},{136,64,133,78},{133,78,135,78},{135,78,136,76},
  {136,76,144,67},{144,67,156,51},{156,51,162,45},{162,45,168,38},
  {168,38,172,35},{172,35,180,35},{180,35,185,43},{185,43,183,52},
  {183,52,175,62},{175,62,168,71},{168,71,159,83},{159,83,153,94},
  {153,94,154,94},{154,94,155,94},{155,94,176,90},{176,90,188,88},
  {188,88,201,85},{201,85,208,88},{208,88,208,91},{208,91,206,97},
  {206,97,191,101},{191,101,174,104},{174,104,148,110},{148,110,148,111},
  {148,111,148,111},{148,111,160,112},{160,112,165,112},{165,112,177,112},
  {177,112,200,114},{200,114,205,118},{205,118,209,123},{209,123,208,126},
  {208,126,199,131},{199,131,187,128},{187,128,159,121},{159,121,149,119},
  {149,119,147,119},{147,119,147,120},{147,120,156,128},{156,128,170,141},
  {170,141,189,158},{189,158,190,163},{190,163,188,166},{188,166,185,166},
  {185,166,169,153},{169,153,162,148},{162,148,148,136},{148,136,147,136},
  {147,136,147,137},{147,137,150,142},{150,142,168,168},{168,168,169,176},
  {169,176,168,179},{168,179,163,180},{163,180,158,179},{158,179,148,165},
  {148,165,137,149},{137,149,129,134},{129,134,128,135},{128,135,123,189},
  {123,189,120,192},{120,192,115,194},{115,194,110,191},{110,191,108,185},
  {108,185,110,174},{110,174,113,160},{113,160,116,148},{116,148,118,134},
  {118,134,119,129},{119,129,119,129},{119,129,118,129},{118,129,107,144},
  {107,144,91,166},{91,166,78,180},{78,180,75,181},{75,181,70,178},
  {70,178,70,173},{70,173,73,169},{73,169,91,146},{91,146,102,132},
  {102,132,109,124},{109,124,109,123},{109,123,108,123},{108,123,61,153},
  {61,153,52,155},{52,155,49,151},{49,151,49,146},{49,146,51,144},
  {51,144,65,134},{65,134,65,134},
};

// ═════════════════════════════════════════════════════════════
//  HELPERS
// ═════════════════════════════════════════════════════════════

int speedMs(int ms) {
  if (animSpeed == 3) return ms / 2;
  if (animSpeed == 1) return ms * 2;
  return ms;
}

uint16_t hexToRgb565(String hex) {
  hex.replace("#", "");
  if (hex.length() != 6) return C_WHITE;
  long v = strtol(hex.c_str(), nullptr, 16);
  return tft.color565((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
}

void setBacklight(bool on) {
  backlightOn = on;
  digitalWrite(TFT_BLK, on ? HIGH : LOW);
}

void initColours() {
  // C_ORANGE = tft.color565(170, 72, 28);
  C_ORANGE = tft.color565(218, 17, 0);
  C_DARKBG = tft.color565(10,  12,  16);
  C_MUTED  = tft.color565(90,  88,  86);
  C_GREEN  = tft.color565(80, 220, 130);
  animBgColor = C_ORANGE;
  drawBgColor = C_ORANGE;
}

// ═════════════════════════════════════════════════════════════
//  LOGO
// ═════════════════════════════════════════════════════════════

void drawLogoFilled(uint16_t bg, uint16_t fg) {
  tft.fillScreen(bg);
  for (uint16_t i = 0; i < LOGO_TRI_COUNT; i++) {
    tft.fillTriangle(
      pgm_read_word(&LOGO_TRIS[i][0]), pgm_read_word(&LOGO_TRIS[i][1]),
      pgm_read_word(&LOGO_TRIS[i][2]), pgm_read_word(&LOGO_TRIS[i][3]),
      pgm_read_word(&LOGO_TRIS[i][4]), pgm_read_word(&LOGO_TRIS[i][5]),
      fg);
  }
  tft.setTextColor(fg); tft.setTextSize(2);
  tft.setCursor(LOGO_CX - 54, 210); tft.print("Anthropic");
  tft.setCursor(LOGO_CX - 53, 210); tft.print("Anthropic");
}

// ═════════════════════════════════════════════════════════════
//  VIEWS
// ═════════════════════════════════════════════════════════════

// Eye helpers — shared constants via #define EYE_*
inline int16_t eyeLX(int16_t ox) {
  return (DISP_W - (EYE_W * 2 + EYE_GAP)) / 2 + EYE_OX + ox;
}
inline int16_t eyeRX(int16_t ox) { return eyeLX(ox) + EYE_W + EYE_GAP; }
inline int16_t eyeY()            { return (DISP_H - EYE_H) / 2 - EYE_OY; }
inline int16_t eyeCY()           { return eyeY() + EYE_H / 2; }

void drawNormalEyes(int16_t ox = 0, bool blink = false) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), ey = eyeY();
  if (!blink) {
    tft.fillRect(lx, ey, EYE_W, EYE_H, C_BLACK);
    tft.fillRect(rx, ey, EYE_W, EYE_H, C_BLACK);
  } else {
    tft.fillRect(lx, ey + EYE_H / 2 - 3, EYE_W, 6, C_BLACK);
    tft.fillRect(rx, ey + EYE_H / 2 - 3, EYE_W, 6, C_BLACK);
  }
}

void drawChevron(int16_t cx, int16_t cy, int16_t arm, int16_t reach,
                 uint8_t thk, bool rightFacing, uint16_t col) {
  for (int8_t t = -(int8_t)thk; t <= (int8_t)thk; t++) {
    if (rightFacing) {
      tft.drawLine(cx - reach/2, cy - arm + t, cx + reach/2, cy + t,      col);
      tft.drawLine(cx + reach/2, cy + t,       cx - reach/2, cy + arm + t, col);
    } else {
      tft.drawLine(cx + reach/2, cy - arm + t, cx - reach/2, cy + t,      col);
      tft.drawLine(cx - reach/2, cy + t,       cx + reach/2, cy + arm + t, col);
    }
  }
}

void drawSquishEyes(bool closed = false) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(0), rx = eyeRX(0), cy = eyeCY();
  const int16_t arm   = EYE_H / 2;
  const int16_t reach = EYE_W / 2;
  const int16_t lcx   = lx + EYE_W / 2;
  const int16_t rcx   = rx + EYE_W / 2;
  if (!closed) {
    drawChevron(lcx, cy, arm, reach, 10, true,  C_BLACK);
    drawChevron(rcx, cy, arm, reach, 10, false, C_BLACK);
  } else {
    tft.fillRect(lx, cy - 5, EYE_W, 10, C_BLACK);
    tft.fillRect(rx, cy - 5, EYE_W, 10, C_BLACK);
  }
}

void drawCompanionEyes(uint8_t expr, int16_t ox = 0, bool idle = false) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), ey = eyeY();
  const int16_t cy = eyeCY();

  switch (expr) {
    case EXPR_HAPPY:
      drawChevron(lx + EYE_W / 2, cy, EYE_H / 2, EYE_W / 2, 8, true, C_BLACK);
      drawChevron(rx + EYE_W / 2, cy, EYE_H / 2, EYE_W / 2, 8, false, C_BLACK);
      break;
    case EXPR_SLEEPY:
      tft.fillRect(lx, cy - 2, EYE_W, 5, C_BLACK);
      tft.fillRect(rx, cy - 2, EYE_W, 5, C_BLACK);
      tft.setTextColor(C_BLACK); tft.setTextSize(2);
      tft.setCursor(176, 48); tft.print("Z");
      if (idle) { tft.setTextSize(1); tft.setCursor(202, 32); tft.print("z"); }
      break;
    case EXPR_STARE:
      tft.fillRect(lx - 4, ey - 4, EYE_W + 8, EYE_H + 8, C_BLACK);
      tft.fillRect(rx - 4, ey - 4, EYE_W + 8, EYE_H + 8, C_BLACK);
      tft.fillRect(lx + 10, ey + 18, 8, 18, C_WHITE);
      tft.fillRect(rx + 10, ey + 18, 8, 18, C_WHITE);
      break;
    case EXPR_BREAK:
      tft.fillRect(lx, cy - 4, EYE_W, 8, C_BLACK);
      tft.fillRect(rx, cy - 4, EYE_W, 8, C_BLACK);
      tft.drawFastHLine(76, 152, 88, C_BLACK);
      break;
    case EXPR_FOCUS:
    default:
      tft.fillRect(lx, ey, EYE_W, EYE_H, C_BLACK);
      tft.fillRect(rx, ey, EYE_W, EYE_H, C_BLACK);
      if (idle) {
        tft.fillRect(lx, ey + EYE_H / 2 - 3, EYE_W, 6, animBgColor);
        tft.fillRect(rx, ey + EYE_H / 2 - 3, EYE_W, 6, animBgColor);
      }
      break;
  }
}

bool setCompanionExpr(String name) {
  name.trim();
  name.toLowerCase();
  if (name == "focus") companionExpr = EXPR_FOCUS;
  else if (name == "happy") companionExpr = EXPR_HAPPY;
  else return false;
  termMode = false;
  currentView = VIEW_COMPANION;
  drawCompanionEyes(companionExpr);
  return true;
}

void idleTick() {
  if (!idleEnabled || busy || termMode || currentView == VIEW_DRAW) return;
  if (currentView != VIEW_COMPANION && currentView != VIEW_EYES_NORMAL &&
      currentView != VIEW_EYES_SQUISH) return;
  const uint32_t now = millis();
  if (now - lastActionMs < 8000 || now - lastIdleMs < 8000) return;
  lastIdleMs = now;
  if (currentView == VIEW_COMPANION) {
    drawCompanionEyes(companionExpr, 0, true);
    delay(speedMs(120));
    drawCompanionEyes(companionExpr);
  } else if (currentView == VIEW_EYES_NORMAL) {
    drawNormalEyes(0, true);
    delay(speedMs(120));
    drawNormalEyes();
  } else if (currentView == VIEW_EYES_SQUISH) {
    drawSquishEyes(true);
    delay(speedMs(120));
    drawSquishEyes(false);
  }
}

void drawCodeView() {
  termMode = false;
  tft.fillScreen(C_DARKBG);
  tft.fillRect(0, 0,          DISP_W, 4, C_ORANGE);
  tft.fillRect(0, DISP_H - 4, DISP_W, 4, C_ORANGE);
  tft.setTextColor(C_ORANGE); tft.setTextSize(4);
  tft.setCursor((DISP_W - 144) / 2, DISP_H / 2 - 52); tft.print("Claude");
  tft.setTextColor(C_WHITE);  tft.setTextSize(4);
  tft.setCursor((DISP_W - 96) / 2,  DISP_H / 2 + 8);  tft.print("Code");
  tft.fillRect((DISP_W - 96) / 2, DISP_H / 2 + 52, 96, 3, C_ORANGE);
}

uint16_t progressColor(const String& state) {
  if (state == PROGRESS_OFFLINE) return C_ORANGE;
  if (state == PROGRESS_IDLE)    return tft.color565(110, 116, 124);
  if (state == PROGRESS_PLAN)    return tft.color565(70, 130, 220);
  if (state == PROGRESS_CODE)    return C_ORANGE;
  if (state == PROGRESS_TEST)    return tft.color565(50, 208, 176);
  if (state == PROGRESS_DONE)    return tft.color565(40, 200, 100);
  if (state == PROGRESS_BLOCK)   return tft.color565(230, 60, 40);
  return C_MUTED;
}

bool isCodexLayerState(const String& state) {
  return state == PROGRESS_IDLE || state == PROGRESS_PLAN || state == PROGRESS_CODE ||
         state == PROGRESS_TEST || state == PROGRESS_DONE || state == PROGRESS_BLOCK;
}

bool isProgressState(const String& state) {
  return state == PROGRESS_OFFLINE || isCodexLayerState(state);
}

uint8_t progressStage(const String& state) {
  if (state == PROGRESS_PLAN)  return 1;
  if (state == PROGRESS_CODE)  return 2;
  if (state == PROGRESS_TEST)  return 3;
  if (state == PROGRESS_DONE)  return 4;
  if (state == PROGRESS_BLOCK) return 4;
  return 0;
}

String progressDefaultMessage(const String& state) {
  if (state == PROGRESS_OFFLINE) return "codex-offline";
  if (state == PROGRESS_IDLE)  return "codex-ready";
  if (state == PROGRESS_PLAN)  return "planning";
  if (state == PROGRESS_CODE)  return "active";
  if (state == PROGRESS_TEST)  return "verifying";
  if (state == PROGRESS_DONE)  return "turn-complete";
  if (state == PROGRESS_BLOCK) return "need-input";
  return "";
}

String cleanAscii(String text, uint8_t maxLen) {
  String out = "";
  text.trim();
  for (uint16_t i = 0; i < text.length() && out.length() < maxLen; i++) {
    const char c = text[i];
    if (c >= 32 && c < 127) out += c;
  }
  return out;
}

void drawProgressBars(uint8_t stage, uint16_t col) {
  const uint16_t off = tft.color565(48, 50, 56);
  for (uint8_t i = 0; i < 4; i++) {
    const int16_t x = 34 + i * 45;
    tft.fillRoundRect(x, 148, 38, 9, 3, i < stage ? col : off);
  }
}

void drawCodexCore(uint16_t col, uint8_t pulse) {
  const uint16_t dim = tft.color565(36, 38, 44);
  const uint8_t ring = 28 + (pulse % 3) * 4;
  tft.fillRect(58, 42, 124, 92, C_DARKBG);
  tft.drawCircle(120, 88, ring, col);
  tft.drawCircle(120, 88, ring + 12, dim);
  tft.fillCircle(120, 88, 18, col);
  tft.fillCircle(120, 88, 8, C_WHITE);
}

void drawCodexScanLine(uint16_t col, uint8_t pulse) {
  const int16_t y = 58 + (pulse % 4) * 16;
  tft.drawFastHLine(78, y, 84, col);
}

void drawDefaultClawdView() {
  termMode = false;
  currentView = VIEW_EYES_NORMAL;
  drawNormalEyes();
}

void drawProgressView() {
  termMode = false;
  if (progressState == PROGRESS_OFFLINE) {
    drawDefaultClawdView();
    return;
  }

  currentView = VIEW_PROGRESS;
  const uint16_t col = progressColor(progressState);
  const uint8_t stage = progressStage(progressState);
  String message = progressMsg.length() > 0 ? progressMsg : progressDefaultMessage(progressState);
  tft.fillScreen(C_DARKBG);
  tft.fillRect(0, 0, DISP_W, 6, col);
  tft.fillRect(0, DISP_H - 6, DISP_W, 6, col);

  tft.setTextColor(C_MUTED); tft.setTextSize(1);
  tft.setCursor(12, 18); tft.print("CODEX");
  tft.fillCircle(222, 21, 4, col);

  // Codex core-pulse status layer.
  drawCodexCore(col, progressPulsePhase);
  if (progressState == PROGRESS_TEST) drawCodexScanLine(col, progressPulsePhase);

  drawProgressBars(stage, progressState == PROGRESS_BLOCK ? progressColor(PROGRESS_BLOCK) : col);

  tft.setTextColor(col); tft.setTextSize(4);
  int16_t x = (DISP_W - progressState.length() * 24) / 2;
  if (x < 0) x = 0;
  tft.setCursor(x, 172); tft.print(progressState);

  if (message.length() > 0) {
    tft.setTextColor(C_WHITE); tft.setTextSize(1);
    int16_t mx = (DISP_W - message.length() * 6) / 2;
    if (mx < 0) mx = 0;
    tft.setCursor(mx, 214); tft.print(message);
  }
}

void drawCodexIdleView() {
  drawProgressView();
}

bool setProgress(String state, String msg) {
  state.trim();
  state.toUpperCase();
  if (!isProgressState(state)) return false;
  progressState = state;
  progressMsg = cleanAscii(msg, 24);
  if (progressMsg.length() == 0) progressMsg = progressDefaultMessage(state);
  progressBlinkOn = true;
  progressPulsePhase = 0;
  lastProgressBlinkMs = millis();
  if (state != PROGRESS_OFFLINE) lastCodexProgressMs = millis();
  drawProgressView();
  return true;
}

void progressTick() {
  if (currentView != VIEW_PROGRESS || !isCodexLayerState(progressState)) return;
  const uint32_t now = millis();
  if (now - lastProgressBlinkMs < 500) return;
  lastProgressBlinkMs = now;
  progressBlinkOn = !progressBlinkOn;
  progressPulsePhase = (progressPulsePhase + 1) % 6;
  drawCodexCore(progressColor(progressState), progressPulsePhase);
  if (progressState == PROGRESS_TEST) drawCodexScanLine(progressColor(progressState), progressPulsePhase);
}

void checkCodexOfflineTimeout() {
  if (!isCodexLayerState(progressState) || lastCodexProgressMs == 0) return;
  if (millis() - lastCodexProgressMs >= CODEX_OFFLINE_TIMEOUT_MS) {
    setProgress(PROGRESS_OFFLINE, "codex-timeout");
  }
}

// ═════════════════════════════════════════════════════════════
//  TERMINAL
// ═════════════════════════════════════════════════════════════

void termClear() {
  for (uint8_t i = 0; i < TERM_ROWS; i++) termLines[i] = "";
  termRow = 0; termCol = 0;
}

void termDrawHeader() {
  tft.fillRect(0, 0, DISP_W, TERM_PAD_Y + 1, C_DARKBG);
  tft.setTextColor(C_ORANGE); tft.setTextSize(1);
  tft.setCursor(TERM_PAD_X, 4); tft.print("clawd@mochi terminal");
  tft.drawFastHLine(0, TERM_PAD_Y, DISP_W, C_ORANGE);
}

// Prefix "clawd:~$ " in green, drawn only when the row has content
void termDrawPrefix(int16_t yy) {
  tft.setTextColor(C_GREEN); tft.setTextSize(1);
  tft.setCursor(TERM_PAD_X, yy + 6);
  tft.print("clawd:~$ ");
}

#define PREFIX_PX 54   // 9 chars × 6px = 54px at textSize 1

void termDrawLine(uint8_t r) {
  const int16_t yy = TERM_PAD_Y + 4 + r * TERM_CHAR_H;
  tft.fillRect(0, yy, DISP_W, TERM_CHAR_H, C_DARKBG);
  // show prefix only on the currently active (cursor) line
  if (r == termRow) termDrawPrefix(yy);
  tft.setTextColor(C_WHITE); tft.setTextSize(2);
  tft.setCursor(TERM_PAD_X + PREFIX_PX, yy + 1);
  tft.print(termLines[r]);
  if (r == termRow) {
    const int16_t cx = TERM_PAD_X + PREFIX_PX + termCol * TERM_CHAR_W;
    tft.fillRect(cx, yy + 1, TERM_CHAR_W - 2, TERM_CHAR_H - 2, C_GREEN);
  }
}

void termDrawLastChar() {
  if (termCol == 0) return;
  const int16_t yy    = TERM_PAD_Y + 4 + termRow * TERM_CHAR_H;
  const int16_t baseX = TERM_PAD_X + PREFIX_PX;
  const uint8_t prev  = termCol - 1;
  // erase prev cell (had cursor block)
  tft.fillRect(baseX + prev * TERM_CHAR_W, yy + 1, TERM_CHAR_W, TERM_CHAR_H - 1, C_DARKBG);
  tft.setTextColor(C_WHITE); tft.setTextSize(2);
  tft.setCursor(baseX + prev * TERM_CHAR_W, yy + 1);
  tft.print(termLines[termRow][prev]);
  // new cursor
  tft.fillRect(baseX + termCol * TERM_CHAR_W, yy + 1, TERM_CHAR_W - 2, TERM_CHAR_H - 2, C_GREEN);
}

void termDrawBackspace() {
  const int16_t yy    = TERM_PAD_Y + 4 + termRow * TERM_CHAR_H;
  const int16_t baseX = TERM_PAD_X + PREFIX_PX;
  // erase deleted char + old cursor
  tft.fillRect(baseX + termCol * TERM_CHAR_W, yy + 1, TERM_CHAR_W * 2, TERM_CHAR_H - 1, C_DARKBG);
  // new cursor
  tft.fillRect(baseX + termCol * TERM_CHAR_W, yy + 1, TERM_CHAR_W - 2, TERM_CHAR_H - 2, C_GREEN);
  // if line now empty, erase the prefix too
  if (termLines[termRow].length() == 0) {
    tft.fillRect(0, yy, TERM_PAD_X + PREFIX_PX, TERM_CHAR_H, C_DARKBG);
  }
}

void termFullRedraw() {
  tft.fillScreen(C_DARKBG);
  termDrawHeader();
  for (uint8_t r = 0; r < TERM_ROWS; r++) termDrawLine(r);
}

void termScroll() {
  for (uint8_t i = 0; i < TERM_ROWS - 1; i++) termLines[i] = termLines[i + 1];
  termLines[TERM_ROWS - 1] = "";
  termRow = TERM_ROWS - 1;
  termFullRedraw();
}

void termAddChar(char c) {
  if (c == '\n' || c == '\r') {
    const int16_t yy = TERM_PAD_Y + 4 + termRow * TERM_CHAR_H;
    // erase cursor on current row
    tft.fillRect(TERM_PAD_X + PREFIX_PX + termCol * TERM_CHAR_W,
                 yy + 1, TERM_CHAR_W, TERM_CHAR_H - 1, C_DARKBG);
    termRow++; termCol = 0;
    if (termRow >= TERM_ROWS) { termScroll(); return; }
    termDrawLine(termRow);  // draws prefix on new line
  } else if (c == '\b' || c == 127) {
    if (termCol > 0) {
      termCol--;
      termLines[termRow].remove(termLines[termRow].length() - 1);
      termDrawBackspace();
    }
  } else if (c >= 32 && c < 127) {
    if (termCol >= TERM_COLS) {
      termRow++; termCol = 0;
      if (termRow >= TERM_ROWS) { termScroll(); return; }
    }
    // draw prefix on first char of this line
    if (termCol == 0) termDrawPrefix(TERM_PAD_Y + 4 + termRow * TERM_CHAR_H);
    termLines[termRow] += c;
    termCol++;
    termDrawLastChar();
  }
}

// ═════════════════════════════════════════════════════════════
//  ANIMATIONS
// ═════════════════════════════════════════════════════════════

void animNormalEyes() {
  busy = true;
  const int16_t offs[] = {-16, 16, -16, 16, 0};
  for (uint8_t i = 0; i < 5; i++) { drawNormalEyes(offs[i]); delay(speedMs(80)); }
  drawNormalEyes(0, true);  delay(speedMs(100));
  drawNormalEyes(0, false); delay(speedMs(70));
  drawNormalEyes(0, true);  delay(speedMs(70));
  drawNormalEyes(0, false);
  busy = false;
}

void animSquishEyes() {
  busy = true;
  for (uint8_t i = 0; i < 3; i++) {
    drawSquishEyes(false); delay(speedMs(160));
    drawSquishEyes(true);  delay(speedMs(100));
  }
  drawSquishEyes(false);
  busy = false;
}

void animLogoReveal() {
  busy = true;
  tft.fillScreen(animBgColor);
  for (uint16_t i = 0; i < LOGO_SEG_COUNT; i++) {
    int16_t x1 = pgm_read_word(&LOGO_SEGS[i][0]);
    int16_t y1 = pgm_read_word(&LOGO_SEGS[i][1]);
    int16_t x2 = pgm_read_word(&LOGO_SEGS[i][2]);
    int16_t y2 = pgm_read_word(&LOGO_SEGS[i][3]);
    tft.drawLine(x1, y1, x2, y2, C_WHITE);
    tft.drawLine(x1 + 1, y1, x2 + 1, y2, C_WHITE);
    if (i % 4 == 0) { server.handleClient(); delay(speedMs(8)); }
  }
  drawLogoFilled(animBgColor, C_WHITE);
  delay(1500);
  busy = false;
}

// ═════════════════════════════════════════════════════════════
//  WEB PAGE
// ═════════════════════════════════════════════════════════════
const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Clawd Mochi 控制器</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
body{background:#1c1c20;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI','Microsoft YaHei','PingFang SC','Courier New',monospace;color:#e8e4dc;
  display:flex;flex-direction:column;align-items:center;
  padding:20px 14px 52px;gap:14px;min-height:100vh}

.hdr{text-align:center;padding:2px 0 4px}
.mascot{font-size:15px;color:#c96a3e;line-height:1.3;font-weight:bold;
  font-family:'Courier New',monospace;display:block;letter-spacing:1px}
.sitename{font-size:10px;color:#5a5048;margin-top:8px;letter-spacing:3px}

.sec{width:100%;max-width:390px;font-size:10px;color:#8a8278;
  letter-spacing:2px;font-weight:bold;padding:0 2px}
.status{width:100%;max-width:390px;display:flex;gap:8px;flex-wrap:wrap}
.chip{flex:1;min-width:96px;background:#222028;border:1.5px solid #38343a;
  border-radius:9px;color:#d8d4cc;font-size:10px;font-weight:bold;
  padding:9px 8px;text-align:center}
.chip b{color:#c96a3e}
.help{width:100%;max-width:390px;color:#7a7068;font-size:10px;
  line-height:1.7;border-top:1px solid #2e2a28;padding-top:10px}

/* Busy bar */
.busy{width:100%;max-width:390px;height:2px;background:#2e2a28;
  border-radius:1px;overflow:hidden;opacity:0;transition:opacity .2s}
.busy.show{opacity:1}
.busy-i{height:100%;width:30%;background:#c96a3e;border-radius:1px;
  animation:sl 1s linear infinite}
@keyframes sl{0%{margin-left:-30%}100%{margin-left:100%}}

/* Controls */
.ctrl{display:flex;gap:8px;width:100%;max-width:390px}
.cbtn{flex:1;background:#252428;border:1.5px solid #38343a;border-radius:10px;
  color:#b8b4ac;font-family:'Courier New',monospace;font-size:11px;font-weight:bold;
  padding:12px 4px;cursor:pointer;text-align:center;transition:all .12s}
.cbtn:active:not(:disabled){transform:scale(.94)}
.cbtn:disabled{opacity:.3;cursor:default}
.cbtn.on{border-color:#c96a3e;color:#c96a3e;background:#201408}
.cbtn.dim{border-color:#2e2a28;color:#4a4540}

/* View grid */
.vgrid{display:grid;grid-template-columns:1fr 1fr;gap:8px;width:100%;max-width:390px}
.vbtn{background:#252428;border:1.5px solid #38343a;border-radius:12px;
  color:#d8d4cc;font-family:'Courier New',monospace;
  padding:14px 6px 10px;cursor:pointer;text-align:center;
  transition:all .12s;user-select:none}
.vbtn:active:not(:disabled){transform:scale(.94)}
.vbtn:disabled{opacity:.3;cursor:default}
.vbtn .ic{font-size:20px;display:block;margin-bottom:4px;line-height:1;color:#c96a3e}
.vbtn .nm{font-size:12px;font-weight:bold;color:#e8e4dc}
.vbtn .ht{font-size:9px;color:#8a8278;margin-top:3px}
.vbtn.active{border-color:#c96a3e;background:#201408}
.vbtn[data-v="1"].active{border-color:#c96a3e;background:#201408}
.vbtn[data-v="2"].active{border-color:#4a8acd;background:#0c1628}
.vbtn[data-v="3"].active{border-color:#38343a;background:#201c18}

/* Speed slider */
.speed-row{width:100%;max-width:390px;display:flex;align-items:center;gap:10px}
.sl{font-size:10px;color:#6a6058;white-space:nowrap;min-width:36px}
input[type=range]{flex:1;accent-color:#c96a3e;cursor:pointer;height:20px}
.sv{font-size:11px;color:#c96a3e;min-width:44px;text-align:right;font-weight:bold}

/* Terminal */
.twrap{width:100%;max-width:390px;display:none;flex-direction:column;gap:8px}
.twrap.open{display:flex}
.thdr{display:flex;justify-content:space-between;align-items:center}
.tttl{font-size:11px;color:#28b878;letter-spacing:1px;font-weight:bold}
.tx{background:#0c1e12;border:2px solid #1a4828;border-radius:9px;
  color:#28b878;font-family:'Courier New',monospace;font-size:13px;
  font-weight:bold;padding:10px 18px;cursor:pointer}
.tx:active{background:#081410}
.trow{display:flex;gap:6px}
.tin{flex:1;background:#0c1018;border:1.5px solid #1a2820;border-radius:9px;
  color:#40d880;font-family:'Courier New',monospace;font-size:15px;
  padding:11px;outline:none}
.tin::placeholder{color:#2a3828}
.tgo{background:#1a9060;border:none;border-radius:9px;color:#fff;
  font-family:'Courier New',monospace;font-size:22px;font-weight:bold;
  padding:11px 16px;cursor:pointer;min-width:52px}
.tgo:active{background:#0f6040}

/* Canvas */
.cwrap{width:100%;max-width:390px;background:#222028;border:1.5px solid #38343a;
  border-radius:12px;padding:12px;flex-direction:column;gap:10px;display:none}
.cwrap.open{display:flex}
.crow{display:flex;gap:8px}
.ci{display:flex;flex-direction:column;align-items:center;gap:4px;flex:1}
.cl{font-size:10px;color:#7a7068;letter-spacing:1px;font-weight:bold}
.cs{width:100%;height:38px;border-radius:7px;border:1.5px solid #38343a;cursor:pointer;padding:0}
.dacts{display:flex;gap:7px}
.db{flex:1;background:#1c1820;border:1.5px solid #38343a;border-radius:9px;
  color:#c0bab8;font-family:'Courier New',monospace;font-size:11px;
  font-weight:bold;padding:11px 4px;cursor:pointer;transition:all .12s}
.db:active{transform:scale(.95);background:#281838}
.db.hi{border-color:#c96a3e;color:#c96a3e}
canvas{width:100%;border-radius:8px;border:1.5px solid #38343a;
  touch-action:none;cursor:crosshair;display:block}

/* Toast */
.toast{position:fixed;bottom:18px;left:50%;transform:translateX(-50%);
  background:#252428;border:1.5px solid #38343a;border-radius:9px;
  font-size:12px;color:#d8d4cc;padding:7px 16px;opacity:0;
  transition:opacity .18s;pointer-events:none;white-space:nowrap;z-index:99}
.toast.show{opacity:1}
</style>
</head>
<body>

<div class="hdr">
  <span class="mascot">&#x2590;&#x259B;&#x2588;&#x2588;&#x2588;&#x259C;&#x258C;<br>&#x259C;&#x2588;&#x2588;&#x2588;&#x2588;&#x2588;&#x259B;<br>&#x2598;&#x2598;&nbsp;&#x259D;&#x259D;</span>
  <div class="sitename">CLAWD &middot; MOCHI &middot; 控制器</div>
</div>

<div class="busy" id="busy"><div class="busy-i"></div></div>

<div class="status">
  <div class="chip">模式 <b id="stView">表情</b></div>
  <div class="chip">速度 <b id="stSpeed">慢</b></div>
  <div class="chip">待机 <b id="stIdle">开</b></div>
</div>

<div class="sec">// 控制</div>
<div class="ctrl">
  <button class="cbtn on" id="blBtn" onclick="toggleBL()">&#9728; 屏幕开</button>
  <button class="cbtn on" id="idleBtn" onclick="toggleIdle()">待机开</button>
</div>

<div class="sec">// 模式</div>
<div class="vgrid">
  <button class="vbtn active" data-v="0" onclick="setView(0)">
    <span class="ic">&#9632; &#9632;</span>
    <span class="nm">正常眼睛</span>
    <span class="ht">摆动 + 眨眼</span>
  </button>
  <button class="vbtn" data-v="1" onclick="setView(1)">
    <span class="ic">&gt; &lt;</span>
    <span class="nm">眯眯眼</span>
    <span class="ht">睁开 / 闭合</span>
  </button>
  <button class="vbtn" data-v="2" onclick="setView(2)">
    <span class="ic">{ }</span>
    <span class="nm">代码模式</span>
    <span class="ht">打开终端</span>
  </button>
  <button class="vbtn" data-v="3" onclick="toggleCanvas()">
    <span class="ic">&#11035;</span>
    <span class="nm">画板</span>
    <span class="ht">画到屏幕上</span>
  </button>
</div>

<div class="sec">// 陪伴表情</div>
<div class="vgrid">
  <button class="vbtn" onclick="setExpr('focus')">
    <span class="ic">&#9632; &#9632;</span>
    <span class="nm">专注</span>
    <span class="ht">focus</span>
  </button>
  <button class="vbtn" onclick="setExpr('happy')">
    <span class="ic">&gt; &lt;</span>
    <span class="nm">开心</span>
    <span class="ht">happy</span>
  </button>
  <button class="vbtn" onclick="setExpr('sleepy')">
    <span class="ic">Z</span>
    <span class="nm">困了</span>
    <span class="ht">sleepy</span>
  </button>
  <button class="vbtn" onclick="setExpr('stare')">
    <span class="ic">::</span>
    <span class="nm">盯着你</span>
    <span class="ht">stare</span>
  </button>
  <button class="vbtn" onclick="setExpr('break')">
    <span class="ic">- -</span>
    <span class="nm">休息中</span>
    <span class="ht">break</span>
  </button>
</div>

<div class="sec">// Codex 进度</div>
<div class="vgrid">
  <button class="vbtn" onclick="setProgressView('PLAN')">
    <span class="ic">P</span>
    <span class="nm">规划中</span>
    <span class="ht">PLAN</span>
  </button>
  <button class="vbtn" onclick="setProgressView('CODE')">
    <span class="ic">C</span>
    <span class="nm">编码中</span>
    <span class="ht">CODE</span>
  </button>
  <button class="vbtn" onclick="setProgressView('TEST')">
    <span class="ic">T</span>
    <span class="nm">测试中</span>
    <span class="ht">TEST</span>
  </button>
  <button class="vbtn" onclick="setProgressView('DONE')">
    <span class="ic">&#10003;</span>
    <span class="nm">已完成</span>
    <span class="ht">DONE</span>
  </button>
  <button class="vbtn" onclick="setProgressView('BLOCK')">
    <span class="ic">!</span>
    <span class="nm">被阻塞</span>
    <span class="ht">BLOCK</span>
  </button>
</div>

<div class="sec">// 速度</div>
<div class="speed-row">
  <span class="sl">慢</span>
  <input type="range" id="spd" min="1" max="3" value="1" step="1" oninput="setSpeed(this.value)">
  <span class="sv" id="spdV">慢</span>
</div>

<div class="ctrl">
  <div class="ci" style="flex:1;display:flex;flex-direction:column;gap:4px;align-items:stretch">
    <span class="cl" style="font-size:10px;color:#8a8278;letter-spacing:1px;font-weight:bold;text-align:center">背景色</span>
    <input type="color" class="cs" id="bgCol" value="#aa4818" oninput="onBgChange(this.value)">
  </div>
  <div class="ci" style="flex:1;display:flex;flex-direction:column;gap:4px;align-items:stretch">
    <span class="cl" style="font-size:10px;color:#8a8278;letter-spacing:1px;font-weight:bold;text-align:center">画笔颜色</span>
    <input type="color" class="cs" id="penCol" value="#000000">
  </div>
</div>

<div class="sec">// 终端</div>
<div class="twrap" id="twrap">
  <div class="thdr">
    <span class="tttl">&#9658; clawd:~$</span>
    <button class="tx" onclick="closeTerm()">&#x2715; 退出终端</button>
  </div>
  <div class="trow">
    <input class="tin" id="tin" type="text" placeholder="在这里输入..."
           autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false">
    <button class="tgo" onclick="termEnter()">&#8629;</button>
  </div>
</div>

<div class="cwrap" id="cwrap">
  <div class="dacts">
    <button class="db hi" onclick="clearAll()">&#11035; 清空</button>
    <button class="db" style="border-color:#28b878;color:#28b878" onclick="toggleCanvas()">&#10003; 完成</button>
  </div>
  <canvas id="cvs" width="240" height="240"></canvas>
</div>

<div class="toast" id="toast"></div>

<div class="help">连接不上时：确认 WiFi 是 ClaWD-Mochi，密码 clawd1234，浏览器地址是 http://192.168.4.1。电脑控制可用 USB 串口脚本，不需要连接设备 WiFi。</div>

<script>
let activeView  = 0;
let termOpen    = false;
let canvasOpen  = false;
let blOn        = true;
let isBusy      = false;
let drawing     = false;
let lastX = 0, lastY = 0;
let tt;

const spdLabels = ['','慢','正常','快'];

// ── Toast ──────────────────────────────────────────────────────
function toast(msg, ok=true) {
  const el = document.getElementById('toast');
  el.textContent = msg;
  el.style.borderColor = ok ? '#28b878' : '#c96a3e';
  el.classList.add('show');
  clearTimeout(tt);
  tt = setTimeout(() => el.classList.remove('show'), 1300);
}

// ── Busy ────────────────────────────────────────────────────────
function setBusy(b) {
  isBusy = b;
  document.getElementById('busy').classList.toggle('show', b);
  const locked = b || termOpen;
  document.querySelectorAll('.vbtn').forEach(el => {
    // when canvas open, keep canvas btn (data-v=3) active so user can exit
    el.disabled = canvasOpen ? parseInt(el.dataset.v) !== 3 : locked;
  });
  document.querySelectorAll('.lbtn').forEach(el => el.disabled = locked || canvasOpen);
  document.querySelectorAll('.cbtn').forEach(el => {
    if (el.id !== 'blBtn') el.disabled = locked;
  });
}

// ── HTTP ────────────────────────────────────────────────────────
async function req(path) {
  try { const r = await fetch(path); return r.ok; }
  catch(e) { toast('连接失败', false); return false; }
}

async function waitNotBusy() {
  for (let i = 0; i < 100; i++) {
    try {
      const r = await fetch('/state');
      const j = await r.json();
      if (!j.busy) return;
    } catch(e) {}
    await new Promise(r => setTimeout(r, 150));
  }
}

// ── Background colour ───────────────────────────────────────────
async function onBgChange(hex) {
  if (canvasOpen) {
    await req('/draw/clear?bg=' + encodeURIComponent(hex));
  } else {
    await req('/redraw?bg=' + encodeURIComponent(hex));
  }
  redrawCanvas(hex);
}

// ── Speed ───────────────────────────────────────────────────────
async function setSpeed(v) {
  document.getElementById('spdV').textContent = spdLabels[v];
  await req('/speed?v=' + v);
}

// ── Views ───────────────────────────────────────────────────────
async function setView(v) {
  if (isBusy || termOpen || canvasOpen) return;
  if (v === 3) { toggleCanvas(); return; }  // canvas button in grid
  const keys = ['w','s','d'];
  if (!await req('/cmd?k=' + keys[v])) return;
  activeView = v;
  document.querySelectorAll('.vbtn').forEach(b =>
    b.classList.toggle('active', parseInt(b.dataset.v) === v));
  if (v === 2) {
    termOpen = true;
    document.getElementById('twrap').classList.add('open');
    setBusy(false);   // re-run to apply termOpen lock
    setBusy(false);
    document.querySelectorAll('.vbtn,.lbtn').forEach(b => b.disabled = true);
    const cvb = document.getElementById('cvBtn'); if (cvb) cvb.disabled = true;
    document.getElementById('tin').focus();
    toast('终端已打开');
    return;
  }
  setBusy(true);
  await waitNotBusy();
  setBusy(false);
}

// ── Logo animations (kept for startup, not exposed in UI) ──────

// ── Backlight ───────────────────────────────────────────────────
async function setProgressView(state) {
  if (isBusy || termOpen || canvasOpen) return;
  if (!await req('/progress?state=' + encodeURIComponent(state) + '&msg=manual')) return;
  document.querySelectorAll('.vbtn').forEach(b => b.classList.remove('active'));
  toast('Codex ' + state);
}

async function toggleBL() {
  blOn = !blOn;
  await req('/backlight?on=' + (blOn ? 1 : 0));
  const b = document.getElementById('blBtn');
  b.textContent = blOn ? '\u2600 屏幕开' : '\u25cb 屏幕关';
  b.classList.toggle('on', blOn);
  b.classList.toggle('dim', !blOn);
}

// ── Canvas toggle ───────────────────────────────────────────────
async function toggleCanvas() {
  canvasOpen = !canvasOpen;
  document.getElementById('cwrap').classList.toggle('open', canvasOpen);
  const b = document.getElementById('cvBtn');
  if (b) { b.classList.toggle('on', canvasOpen); b.textContent = canvasOpen ? '\u2b1b 画板开' : '\u2b1b 画板'; }
  // highlight the canvas vbtn (data-v=3) in the grid
  document.querySelectorAll('.vbtn').forEach(btn =>
    btn.classList.toggle('active', canvasOpen && parseInt(btn.dataset.v) === 3));
  await req('/canvas?on=' + (canvasOpen ? 1 : 0));
  if (canvasOpen) {
    const bg = document.getElementById('bgCol').value;
    redrawCanvas(bg);
    await req('/draw/clear?bg=' + encodeURIComponent(bg));
    // lock all other buttons
    document.querySelectorAll('.vbtn,.lbtn').forEach(b => b.disabled = true);
    toast('画板已打开');
  } else {
    setBusy(false);   // re-evaluate locks
    toast('画板已关闭');
  }
}

// ── Terminal ────────────────────────────────────────────────────
const tin = document.getElementById('tin');
let lastVal = '';
tin.addEventListener('input', async () => {
  const cur = tin.value, prev = lastVal;
  if (cur.length > prev.length) {
    await req('/char?c=' + encodeURIComponent(cur[cur.length - 1]));
  } else if (cur.length < prev.length) {
    await req('/char?c=%08');
  }
  lastVal = cur;
});
async function termEnter() {
  await req('/char?c=%0A');
  tin.value = ''; lastVal = ''; tin.focus();
}
tin.addEventListener('keydown', e => {
  if (e.key === 'Enter') { e.preventDefault(); termEnter(); }
});
async function closeTerm() {
  await req('/cmd?k=q');
  termOpen = false;
  document.getElementById('twrap').classList.remove('open');
  setBusy(false);
  toast('终端已关闭');
}

// ── Canvas drawing — send full stroke on finger lift ────────────
const cvs = document.getElementById('cvs');
const ctx = cvs.getContext('2d');
let strokePts = [];

function getPos(e) {
  const r = cvs.getBoundingClientRect();
  const sx = cvs.width / r.width, sy = cvs.height / r.height;
  const s = e.touches ? e.touches[0] : e;
  return { x: (s.clientX - r.left) * sx, y: (s.clientY - r.top) * sy };
}

function redrawCanvas(hex) {
  ctx.fillStyle = hex;
  ctx.fillRect(0, 0, cvs.width, cvs.height);
}

function startDraw(e) {
  e.preventDefault();
  drawing = true;
  strokePts = [];
  const p = getPos(e); lastX = p.x; lastY = p.y;
  strokePts.push({ x: Math.round(p.x), y: Math.round(p.y) });
  // draw dot on canvas preview only — no display send yet
  ctx.beginPath(); ctx.arc(p.x, p.y, 2, 0, Math.PI * 2);
  ctx.fillStyle = document.getElementById('penCol').value; ctx.fill();
}
function moveDraw(e) {
  if (!drawing) return; e.preventDefault();
  const p = getPos(e);
  ctx.beginPath(); ctx.moveTo(lastX, lastY); ctx.lineTo(p.x, p.y);
  ctx.strokeStyle = document.getElementById('penCol').value;
  ctx.lineWidth = 4; ctx.lineCap = 'round'; ctx.stroke();
  strokePts.push({ x: Math.round(p.x), y: Math.round(p.y) });
  lastX = p.x; lastY = p.y;
}
async function endDraw(e) {
  if (!drawing) return; drawing = false;
  if (!canvasOpen || strokePts.length < 1) return;
  const pen = document.getElementById('penCol').value.replace('#', '');
  const pts = strokePts.map(p => p.x + ',' + p.y).join(';');
  await req('/draw/stroke?pen=' + pen + '&pts=' + encodeURIComponent(pts));
  strokePts = [];
}

cvs.addEventListener('mousedown',  startDraw);
cvs.addEventListener('mousemove',  moveDraw);
cvs.addEventListener('mouseup',    endDraw);
cvs.addEventListener('mouseleave', endDraw);
cvs.addEventListener('touchstart', startDraw, {passive:false});
cvs.addEventListener('touchmove',  moveDraw,  {passive:false});
cvs.addEventListener('touchend',   endDraw);

// Clear = clear both web canvas and display
async function clearAll() {
  const bg = document.getElementById('bgCol').value;
  redrawCanvas(bg);
  await req('/draw/clear?bg=' + encodeURIComponent(bg));
  toast('已清空');
}

// Init: sync speed and backlight from ESP32, reset bg to default
(async () => {
  try {
    const r = await fetch('/state');
    const j = await r.json();
    // Sync speed
    const spd = j.speed || 1;
    document.getElementById('spd').value = spd;
    document.getElementById('spdV').textContent = spdLabels[spd];
    // Sync backlight
    if (j.bl === false) {
      blOn = false;
      const b = document.getElementById('blBtn');
      b.textContent = '\u25cb 屏幕关';
      b.classList.remove('on'); b.classList.add('dim');
    }
  } catch(e) {}
  // Always reset bg picker to default orange on page load
  document.getElementById('bgCol').value = '#aa4818';
  redrawCanvas('#aa4818');
})();
</script>
</body>
</html>
)rawhtml";

const char INDEX_HTML_CLEAN[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Clawd Mochi</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
body{background:#1c1c20;color:#e8e4dc;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI","Microsoft YaHei",sans-serif;display:flex;flex-direction:column;align-items:center;gap:12px;min-height:100vh;padding:18px 14px 42px}
.hdr{text-align:center}.mascot{font-family:"Courier New",monospace;color:#c96a3e;font-size:15px;line-height:1.2;font-weight:bold}.site{font-size:11px;color:#8a8278;letter-spacing:2px;margin-top:8px}
.sec{width:100%;max-width:390px;color:#8a8278;font-size:11px;font-weight:bold;letter-spacing:1px}
.area{width:100%;max-width:390px;border-top:1px solid #2e2a28;padding-top:12px;display:flex;flex-direction:column;gap:8px}.status,.ctrl,.grid{width:100%;display:grid;gap:8px}.status{grid-template-columns:repeat(3,1fr)}.ctrl{grid-template-columns:repeat(2,1fr)}.grid{grid-template-columns:repeat(2,1fr)}
.chip,.btn{background:#252428;border:1.5px solid #38343a;border-radius:9px;color:#d8d4cc;text-align:center;font-weight:bold}
.chip{font-size:10px;padding:9px 6px}.chip b{color:#c96a3e}.btn{font-size:12px;min-height:54px;padding:10px 6px;cursor:pointer}.btn:active{transform:scale(.96)}.btn.on{border-color:#c96a3e;color:#c96a3e;background:#201408}.btn .ic{display:block;color:#c96a3e;font-size:18px;line-height:1;margin-bottom:5px}.btn .hint{display:block;color:#8a8278;font-size:9px;margin-top:3px}
.row{width:100%;max-width:390px;display:flex;gap:10px;align-items:center}.row span{font-size:10px;color:#8a8278}input[type=range]{flex:1;accent-color:#c96a3e}.colors{width:100%;max-width:390px;display:grid;grid-template-columns:1fr 1fr;gap:8px}.colorbox{display:flex;flex-direction:column;gap:5px;color:#8a8278;font-size:10px;text-align:center}.colorbox input{height:38px;border-radius:7px;border:1.5px solid #38343a}
.panel{width:100%;max-width:390px;display:none;flex-direction:column;gap:8px}.panel.open{display:flex}.termrow{display:flex;gap:6px}.termrow input{flex:1;background:#0c1018;border:1.5px solid #1a2820;border-radius:8px;color:#40d880;font-size:15px;padding:11px}.canvas{width:100%;border:1.5px solid #38343a;border-radius:8px;touch-action:none}
.toast{position:fixed;bottom:16px;left:50%;transform:translateX(-50%);background:#252428;border:1.5px solid #38343a;border-radius:9px;color:#e8e4dc;font-size:12px;padding:7px 14px;opacity:0;transition:.18s}.toast.show{opacity:1}.help{width:100%;max-width:390px;border-top:1px solid #2e2a28;color:#8a8278;font-size:10px;line-height:1.7;padding-top:10px}
</style>
</head>
<body>
<div class="hdr"><div class="mascot">&#x2590;&#x259B;&#x2588;&#x2588;&#x2588;&#x259C;&#x258C;<br>&#x259C;&#x2588;&#x2588;&#x2588;&#x2588;&#x2588;&#x259B;<br>&#x2598;&#x2598;&nbsp;&#x259D;&#x259D;</div><div class="site">CLAWD MOCHI</div></div>
<div class="status"><div class="chip">&#x6A21;&#x5F0F; <b id="stView">&#x8868;&#x60C5;</b></div><div class="chip">&#x901F;&#x5EA6; <b id="stSpeed">&#x6162;</b></div><div class="chip">&#x5F85;&#x673A; <b id="stIdle">&#x5F00;</b></div></div>

<div class="area">
<div class="sec">// &#x65E5;&#x5E38;&#x966A;&#x4F34;</div>
<div class="ctrl"><button class="btn on" id="blBtn" onclick="toggleBL()">&#9728; &#x5C4F;&#x5E55;&#x5F00;</button><button class="btn on" id="idleBtn" onclick="toggleIdle()">&#x5F85;&#x673A;&#x5F00;</button></div>
<div class="grid">
<button class="btn" onclick="setView(0)"><span class="ic">&#9632; &#9632;</span>&#x6B63;&#x5E38;&#x773C;&#x775B;<span class="hint">normal</span></button>
<button class="btn" onclick="setView(1)"><span class="ic">&gt; &lt;</span>&#x772F;&#x772F;&#x773C;<span class="hint">squish</span></button>
<button class="btn" onclick="setExpr('focus')"><span class="ic">&#9632; &#9632;</span>&#x4E13;&#x6CE8;<span class="hint">focus</span></button>
<button class="btn" onclick="setExpr('happy')"><span class="ic">&gt; &lt;</span>&#x5F00;&#x5FC3;<span class="hint">happy</span></button>
<button class="btn" onclick="setExpr('sleepy')"><span class="ic">Z</span>&#x56F0;&#x4E86;<span class="hint">sleepy</span></button>
<button class="btn" onclick="setExpr('stare')"><span class="ic">::</span>&#x76EF;&#x7740;&#x4F60;<span class="hint">stare</span></button>
<button class="btn" onclick="setExpr('break')"><span class="ic">- -</span>&#x4F11;&#x606F;&#x4E2D;<span class="hint">break</span></button>
</div>
<div class="row"><span>&#x6162;</span><input type="range" id="spd" min="1" max="3" value="1" step="1" oninput="setSpeed(this.value)"><span id="spdV">&#x6162;</span></div>
<div class="colors"><label class="colorbox">&#x80CC;&#x666F;&#x8272;<input type="color" id="bgCol" value="#aa4818" oninput="onBgChange(this.value)"></label><label class="colorbox">&#x753B;&#x7B14;&#x989C;&#x8272;<input type="color" id="penCol" value="#000000"></label></div>
</div>

<div class="area">
<div class="sec">// &#x521B;&#x4F5C;&#x753B;&#x677F;</div>
<div class="grid">
<button class="btn" onclick="toggleCanvas()"><span class="ic">&#11035;</span>&#x8FDB;&#x5165;&#x753B;&#x677F;<span class="hint">canvas</span></button>
<button class="btn" onclick="clearAll()"><span class="ic">&#9003;</span>&#x6E05;&#x7A7A;&#x753B;&#x677F;<span class="hint">clear</span></button>
</div>
</div>

<div class="area">
<div class="sec">// Codex &#x5DE5;&#x4F5C;</div>
<div class="grid">
<button class="btn" onclick="setView(2)"><span class="ic">{ }</span>&#x4EE3;&#x7801;&#x7EC8;&#x7AEF;<span class="hint">terminal</span></button>
<button class="btn" onclick="setProgressView('IDLE')"><span class="ic">_</span>&#x5F85;&#x673A; Logo<span class="hint">IDLE</span></button>
<button class="btn" onclick="setProgressView('PLAN')"><span class="ic">P</span>&#x89C4;&#x5212;&#x4E2D;<span class="hint">PLAN</span></button>
<button class="btn" onclick="setProgressView('CODE')"><span class="ic">C</span>&#x7F16;&#x7801;&#x4E2D;<span class="hint">CODE</span></button>
<button class="btn" onclick="setProgressView('TEST')"><span class="ic">T</span>&#x6D4B;&#x8BD5;&#x4E2D;<span class="hint">TEST</span></button>
<button class="btn" onclick="setProgressView('DONE')"><span class="ic">&#10003;</span>&#x5DF2;&#x5B8C;&#x6210;<span class="hint">DONE</span></button>
<button class="btn" onclick="setProgressView('BLOCK')"><span class="ic">!</span>&#x88AB;&#x963B;&#x585E;<span class="hint">BLOCK</span></button>
</div>
</div>

<div class="area">
<div class="sec">// &#x8BBE;&#x5907;&#x7EF4;&#x62A4;</div>
<div class="ctrl"><button class="btn" onclick="location.href='/ota'">OTA &#x5728;&#x7EBF;&#x5347;&#x7EA7;</button><button class="btn" onclick="refreshState()">&#x5237;&#x65B0;&#x72B6;&#x6001;</button></div>
</div>
<div class="panel" id="term"><div class="termrow"><input id="tin" placeholder="type here..." autocomplete="off"><button class="btn" onclick="termEnter()">&#8629;</button></div><button class="btn" onclick="closeTerm()">&#x9000;&#x51FA;&#x7EC8;&#x7AEF;</button></div>
<div class="panel" id="draw"><div class="ctrl"><button class="btn" onclick="clearAll()">&#x6E05;&#x7A7A;</button><button class="btn" onclick="toggleCanvas()">&#x5B8C;&#x6210;</button></div><canvas class="canvas" id="cvs" width="240" height="240"></canvas></div>
<div class="help">&#x8FDE;&#x63A5;&#x4E0D;&#x4E0A;&#x65F6;&#xFF1A;&#x786E;&#x8BA4; WiFi &#x662F; ClaWD-Mochi&#xFF0C;&#x5BC6;&#x7801; clawd1234&#xFF0C;&#x5730;&#x5740; http://192.168.4.1&#x3002;&#x7535;&#x8111;&#x63A7;&#x5236;&#x53EF;&#x7528; USB &#x4E32;&#x53E3;&#x811A;&#x672C;&#x3002;</div>
<div class="toast" id="toast"></div>
<script>
let termOpen=false,canvasOpen=false,blOn=true,idleOn=true,drawing=false,lastVal='',lastX=0,lastY=0,strokePts=[],tt;
const spdLabels=['','&#x6162;','&#x6B63;&#x5E38;','&#x5FEB;'],viewLabels=['&#x8868;&#x60C5;','&#x772F;&#x772F;&#x773C;','&#x4EE3;&#x7801;','&#x753B;&#x677F;','Codex','&#x966A;&#x4F34;'];
function $(id){return document.getElementById(id)}
function toast(msg,ok=true){const e=$('toast');e.textContent=msg;e.style.borderColor=ok?'#28b878':'#c96a3e';e.classList.add('show');clearTimeout(tt);tt=setTimeout(()=>e.classList.remove('show'),1200)}
async function req(p){try{const r=await fetch(p);return r.ok}catch(e){toast('no connection',false);return false}}
function status(view){if(view!==undefined)$('stView').innerHTML=viewLabels[view]||viewLabels[0];$('stSpeed').innerHTML=$('spdV').innerHTML;$('stIdle').innerHTML=idleOn?'&#x5F00;':'&#x5173;'}
async function setView(v){if(termOpen||canvasOpen)return;const k=['w','s','d'][v];if(await req('/cmd?k='+k)){status(v);if(v===2){termOpen=true;$('term').classList.add('open');$('tin').focus()}}}
async function setExpr(n){if(await req('/expr?name='+encodeURIComponent(n))){status(5);toast('OK')}}
async function setProgressView(s){if(await req('/progress?state='+s+'&msg=manual')){status(4);toast('Codex '+s)}}
async function setSpeed(v){$('spdV').innerHTML=spdLabels[v];await req('/speed?v='+v);status()}
async function refreshState(){try{const r=await fetch('/state'),j=await r.json();idleOn=j.idle!==false;$('spd').value=j.speed||1;$('spdV').innerHTML=spdLabels[j.speed||1];status(j.view)}catch(e){toast('no connection',false)}}
async function toggleBL(){blOn=!blOn;await req('/backlight?on='+(blOn?1:0));$('blBtn').innerHTML=blOn?'&#9728; &#x5C4F;&#x5E55;&#x5F00;':'&#9675; &#x5C4F;&#x5E55;&#x5173;';$('blBtn').classList.toggle('on',blOn)}
async function toggleIdle(){idleOn=!idleOn;await req('/idle?on='+(idleOn?1:0));$('idleBtn').innerHTML=idleOn?'&#x5F85;&#x673A;&#x5F00;':'&#x5F85;&#x673A;&#x5173;';$('idleBtn').classList.toggle('on',idleOn);status()}
async function onBgChange(hex){await req((canvasOpen?'/draw/clear?bg=':'/redraw?bg=')+encodeURIComponent(hex));redrawCanvas(hex)}
async function toggleCanvas(){canvasOpen=!canvasOpen;$('draw').classList.toggle('open',canvasOpen);await req('/canvas?on='+(canvasOpen?1:0));if(canvasOpen){redrawCanvas($('bgCol').value);await req('/draw/clear?bg='+encodeURIComponent($('bgCol').value));status(3)}}
const tin=$('tin');tin.addEventListener('input',async()=>{const c=tin.value,p=lastVal;if(c.length>p.length)await req('/char?c='+encodeURIComponent(c[c.length-1]));else if(c.length<p.length)await req('/char?c=%08');lastVal=c});tin.addEventListener('keydown',e=>{if(e.key==='Enter'){e.preventDefault();termEnter()}})
async function termEnter(){await req('/char?c=%0A');tin.value='';lastVal='';tin.focus()}async function closeTerm(){await req('/cmd?k=q');termOpen=false;$('term').classList.remove('open');status(2)}
const cvs=$('cvs'),ctx=cvs.getContext('2d');function pos(e){const r=cvs.getBoundingClientRect(),s=e.touches?e.touches[0]:e;return{x:(s.clientX-r.left)*cvs.width/r.width,y:(s.clientY-r.top)*cvs.height/r.height}}function redrawCanvas(hex){ctx.fillStyle=hex;ctx.fillRect(0,0,240,240)}
function startDraw(e){e.preventDefault();drawing=true;strokePts=[];const p=pos(e);lastX=p.x;lastY=p.y;strokePts.push({x:Math.round(p.x),y:Math.round(p.y)});ctx.fillStyle=$('penCol').value;ctx.beginPath();ctx.arc(p.x,p.y,2,0,Math.PI*2);ctx.fill()}
function moveDraw(e){if(!drawing)return;e.preventDefault();const p=pos(e);ctx.strokeStyle=$('penCol').value;ctx.lineWidth=4;ctx.lineCap='round';ctx.beginPath();ctx.moveTo(lastX,lastY);ctx.lineTo(p.x,p.y);ctx.stroke();strokePts.push({x:Math.round(p.x),y:Math.round(p.y)});lastX=p.x;lastY=p.y}
async function endDraw(){if(!drawing)return;drawing=false;if(!canvasOpen||!strokePts.length)return;const pts=strokePts.map(p=>p.x+','+p.y).join(';');await req('/draw/stroke?pen='+$('penCol').value.replace('#','')+'&pts='+encodeURIComponent(pts));strokePts=[]}
async function clearAll(){redrawCanvas($('bgCol').value);await req('/draw/clear?bg='+encodeURIComponent($('bgCol').value))}
['mousedown','touchstart'].forEach(e=>cvs.addEventListener(e,startDraw,{passive:false}));['mousemove','touchmove'].forEach(e=>cvs.addEventListener(e,moveDraw,{passive:false}));['mouseup','mouseleave','touchend'].forEach(e=>cvs.addEventListener(e,endDraw));
(async()=>{try{const r=await fetch('/state'),j=await r.json();idleOn=j.idle!==false;$('spd').value=j.speed||1;$('spdV').innerHTML=spdLabels[j.speed||1];if(j.bl===false){blOn=false;$('blBtn').innerHTML='&#9675; &#x5C4F;&#x5E55;&#x5173;';$('blBtn').classList.remove('on')}}catch(e){}redrawCanvas('#aa4818');status()})();
</script>
</body>
</html>
)rawhtml";

const char INDEX_HTML_LITE[] PROGMEM = R"rawhtml(
<!doctype html><html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Clawd Mochi</title><style>
*{box-sizing:border-box}body{margin:0;background:#eef1ed;color:#252925;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI","Microsoft YaHei",sans-serif;padding:18px 14px 40px}
.wrap{max-width:390px;margin:auto}.head{display:flex;align-items:center;justify-content:space-between;padding:3px 2px 14px}.logo{font-size:18px;font-weight:900}.online{color:#24744a;font-size:11px}.online:before{content:"";display:inline-block;width:7px;height:7px;margin-right:5px;border-radius:50%;background:#28a864}.online.off{color:#a33}.online.off:before{background:#c44}
.work{background:#222824;color:#fff;border-radius:8px;padding:16px}.worktop,.workstate{display:flex;align-items:center;justify-content:space-between}.worktop{color:#aeb8b0;font-size:10px}.source{color:#fff}.workstate{margin-top:9px}.workstate b{font-size:25px}.workstate span{color:#efbc42;font:700 11px monospace}.bar{display:grid;grid-template-columns:repeat(4,1fr);gap:5px;margin-top:14px}.bar i{height:5px;border-radius:4px;background:#4e5751}.bar i.on{background:#efbc42}.bar.done i{background:#35a968}.bar.block i{background:#d65343}
.sec{font-size:12px;font-weight:900;margin:18px 2px 8px}.grid,.device{display:grid;gap:7px}.grid{grid-template-columns:repeat(3,1fr)}.device{grid-template-columns:repeat(2,1fr)}.btn{border:1px solid #d4d9d3;background:#fff;color:#252925;border-radius:7px;min-height:54px;padding:11px 4px;font-weight:800}.btn:active{transform:scale(.97)}.btn.on{border-color:#d9472b;color:#c53d26;background:#fff8f5}.device .btn{text-align:left;padding:12px}.toggle{float:right;color:#25814e}.toast{position:fixed;bottom:16px;left:50%;transform:translateX(-50%);background:#222824;color:#fff;border-radius:7px;padding:8px 14px;font-size:11px;opacity:0;transition:.2s}.toast.show{opacity:1}
</style></head><body><main class="wrap">
<div class="head"><div class="logo">Clawd Mochi</div><div class="online" id="online">&#x5728;&#x7EBF;</div></div>
<section class="work"><div class="worktop"><span>&#x5DE5;&#x4F5C;&#x72B6;&#x6001;</span><span class="source" id="source">Codex</span></div>
<div class="workstate"><b id="progressText">&#x5F85;&#x673A;&#x4E2D;</b><span id="progress">IDLE</span></div><div class="bar" id="bar"><i></i><i></i><i></i><i></i></div></section>
<div class="sec">&#x5C4F;&#x5E55;&#x663E;&#x793A;</div><div class="grid">
<button class="btn" onclick="cmd('normal')">&#x6B63;&#x5E38;</button>
<button class="btn" onclick="expr('focus')">&#x4E13;&#x6CE8;</button>
<button class="btn" onclick="expr('happy')">&#x5F00;&#x5FC3;</button>
</div>
<div class="sec">&#x8BBE;&#x5907;</div><div class="device">
<button class="btn on" id="blBtn" onclick="toggleBL()"><span class="toggle" id="bl">&#x5F00;&#x542F;</span>&#x5C4F;&#x5E55;&#x80CC;&#x5149;</button>
<button class="btn" onclick="location.href='/ota'">OTA &#x5347;&#x7EA7;</button>
<button class="btn" onclick="location.href='/network'">&#x7F51;&#x7EDC;&#x8BBE;&#x7F6E;</button>
</div></main><div class="toast" id="toast"></div><script>
let bl=true;const $=id=>document.getElementById(id),labels={OFFLINE:'Codex \u79bb\u7ebf',IDLE:'\u5f85\u673a\u4e2d',PLAN:'\u89c4\u5212\u4e2d',CODE:'\u7f16\u7801\u4e2d',TEST:'\u6d4b\u8bd5\u4e2d',DONE:'\u5df2\u5b8c\u6210',BLOCK:'\u88ab\u963b\u585e'},steps={OFFLINE:0,IDLE:0,PLAN:1,CODE:2,TEST:3,DONE:4,BLOCK:4};
function toast(t){$('toast').textContent=t;$('toast').classList.add('show');setTimeout(()=>$('toast').classList.remove('show'),1000)}
async function req(path){const r=await fetch(path);if(!r.ok)throw new Error();return r}
async function cmd(n){try{await req('/cmd?k='+n);toast('\u5df2\u66f4\u65b0')}catch(e){failed()}}
async function expr(n){try{await req('/expr?name='+n);toast('\u5df2\u66f4\u65b0')}catch(e){failed()}}
async function toggleBL(){bl=!bl;try{await req('/backlight?on='+(bl?1:0));paintBL();toast('\u5df2\u66f4\u65b0')}catch(e){bl=!bl;paintBL();failed()}}
function paintBL(){$('bl').textContent=bl?'\u5f00\u542f':'\u5173\u95ed';$('blBtn').classList.toggle('on',bl)}
function failed(){$('online').textContent='\u79bb\u7ebf';$('online').classList.add('off');toast('\u8fde\u63a5\u5931\u8d25')}
function paintProgress(s){$('progress').textContent=s;$('progressText').textContent=labels[s]||s;const bar=$('bar');bar.className='bar '+(s==='DONE'?'done':s==='BLOCK'?'block':'');[...bar.children].forEach((x,i)=>x.classList.toggle('on',i<(steps[s]||0)))}
async function refresh(){try{const j=await (await req('/state')).json();bl=j.bl!==false;paintBL();paintProgress(j.progress||'OFFLINE');$('online').textContent='\u5728\u7ebf';$('online').classList.remove('off')}catch(e){failed()}}
refresh();setInterval(refresh,5000);
</script></body></html>
)rawhtml";

const char OTA_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Clawd Mochi OTA</title>
<style>
*{box-sizing:border-box}body{background:#eef1ed;color:#252925;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI","Microsoft YaHei",sans-serif;display:flex;justify-content:center;min-height:100vh;margin:0;padding:24px 14px}
.box{width:100%;max-width:390px}.title{font-size:20px;font-weight:900;margin-bottom:18px}.card{border:1px solid #d4d9d3;border-radius:8px;padding:14px;background:#fff}input,button{width:100%;font-size:14px}button{border:0;border-radius:7px;background:#d9472b;color:#fff;font-weight:bold;padding:12px;margin-top:12px}.link{display:block;color:#59635c;margin-top:18px;text-align:center;text-decoration:none;font-size:12px}
</style>
</head>
<body>
<div class="box">
<div class="title">OTA &#x5347;&#x7EA7;</div>
<form class="card" method="POST" action="/ota" enctype="multipart/form-data">
<input type="file" name="firmware" accept=".bin" required>
<button type="submit">&#x4E0A;&#x4F20;&#x56FA;&#x4EF6;</button>
</form>
<a class="link" href="/">&#x8FD4;&#x56DE;</a>
</div>
</body>
</html>
)rawhtml";

// ═════════════════════════════════════════════════════════════
//  WEB ROUTES
// ═════════════════════════════════════════════════════════════

const char NETWORK_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html><html lang="zh-CN"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Clawd Mochi Network</title><style>
*{box-sizing:border-box}body{background:#eef1ed;color:#252925;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI","Microsoft YaHei",sans-serif;display:flex;justify-content:center;min-height:100vh;margin:0;padding:24px 14px}
.box{width:100%;max-width:390px}.title{font-size:20px;font-weight:900;margin-bottom:12px}.card{border:1px solid #d4d9d3;border-radius:8px;padding:14px;background:#fff;margin-bottom:10px}.muted{color:#687169;font-size:12px;line-height:1.7}select,input,button{width:100%;font-size:14px;border-radius:7px;padding:12px}select,input{border:1px solid #d4d9d3;background:#fff;margin-top:8px}button{border:0;background:#d9472b;color:#fff;font-weight:bold;margin-top:10px}.secondary{background:#59635c}.link{display:block;color:#59635c;margin-top:18px;text-align:center;text-decoration:none;font-size:12px}
</style></head><body><main class="box">
<div class="title">&#x7F51;&#x7EDC;&#x8BBE;&#x7F6E;</div>
<section class="card"><div id="status" class="muted">&#x6B63;&#x5728;&#x8BFB;&#x53D6;&#x72B6;&#x6001;...</div></section>
<section class="card"><select id="ssid"><option value="">&#x6B63;&#x5728;&#x626B;&#x63CF; WiFi...</option></select>
<input id="password" type="password" placeholder="WiFi &#x5BC6;&#x7801;">
<button onclick="connectWifi()">&#x4FDD;&#x5B58;&#x5E76;&#x8FDE;&#x63A5;</button>
<button class="secondary" onclick="clearWifi()">&#x6E05;&#x9664;&#x914D;&#x7F6E;</button></section>
<a class="link" href="/">&#x8FD4;&#x56DE;</a></main><script>
const $=id=>document.getElementById(id);
async function refresh(){const j=await (await fetch('/state')).json();$('status').textContent=j.wifiConnected?('\u5df2\u8fde\u63a5 '+j.wifiSsid+' / '+j.wifiIp):('\u672a\u8fde\u63a5\u5c40\u57df\u7f51 / \u70ed\u70b9 '+j.apIp)}
async function scan(){const list=await (await fetch('/wifi/scan')).json();$('ssid').innerHTML='<option value="">\u9009\u62e9 2.4 GHz WiFi</option>'+list.map(x=>'<option value="'+x.ssid.replace(/"/g,'&quot;')+'">'+x.ssid+' ('+x.rssi+' dBm)</option>').join('')}
async function connectWifi(){const body=new URLSearchParams({ssid:$('ssid').value,password:$('password').value});const r=await fetch('/wifi/connect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});alert(r.ok?'\u5df2\u4fdd\u5b58\uff0c\u6b63\u5728\u8fde\u63a5':'\u8bf7\u9009\u62e9 WiFi');setTimeout(refresh,5000)}
async function clearWifi(){await fetch('/wifi/clear',{method:'POST'});$('password').value='';refresh()}
refresh();scan();
</script></body></html>
)rawhtml";

void routeRoot() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.send_P(200, "text/html", INDEX_HTML_LITE);
}

void connectSavedWifi() {
  if (savedWifiSsid.isEmpty()) return;
  WiFi.begin(savedWifiSsid.c_str(), savedWifiPassword.c_str());
}

void startSetupAp() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
}

void restartSetupAp() {
  WiFi.softAPdisconnect(true);
  startSetupAp();
}

void routeNetworkPage() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send_P(200, "text/html", NETWORK_HTML);
}

void routeWifiScan() {
  const int count = WiFi.scanNetworks();
  String j = "[";
  for (int i = 0; i < count; i++) {
    if (i > 0) j += ",";
    j += "{\"ssid\":\""; j += jsonEscape(WiFi.SSID(i)); j += "\",\"rssi\":";
    j += WiFi.RSSI(i); j += "}";
  }
  j += "]";
  WiFi.scanDelete();
  server.send(200, "application/json", j);
}

void routeWifiConnect() {
  const String ssid = server.arg("ssid");
  if (ssid.isEmpty()) {
    server.send(400, "application/json", "{\"e\":\"ssid-required\"}");
    return;
  }
  savedWifiSsid = ssid;
  savedWifiPassword = server.arg("password");
  wifiPrefs.putString("ssid", savedWifiSsid);
  wifiPrefs.putString("password", savedWifiPassword);
  WiFi.disconnect(false, false);
  connectSavedWifi();
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeWifiClear() {
  wifiPrefs.clear();
  savedWifiSsid = "";
  savedWifiPassword = "";
  WiFi.disconnect(false, true);
  server.send(200, "application/json", "{\"ok\":1}");
  delay(200);
  restartSetupAp();
}

void drawOtaStatus(const char* line1, const char* line2, uint16_t col) {
  busy = true;
  termMode = false;
  tft.fillScreen(C_DARKBG);
  tft.fillRect(0, 0, DISP_W, 6, col);
  tft.setTextColor(col); tft.setTextSize(3);
  tft.setCursor(28, 70); tft.print(line1);
  tft.setTextColor(C_WHITE); tft.setTextSize(2);
  tft.setCursor(18, 126); tft.print(line2);
}

void routeOtaPage() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send_P(200, "text/html", OTA_HTML);
}

void routeOtaResult() {
  const bool ok = !Update.hasError();
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", ok ? "OK rebooting" : "OTA failed");
  if (ok) {
    delay(600);
    ESP.restart();
  } else {
    busy = false;
  }
}

void handleOtaUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    drawOtaStatus("OTA", "UPDATING", C_ORANGE);
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      drawOtaStatus("OTA", "FAILED", tft.color565(230, 60, 40));
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      drawOtaStatus("OTA", "WRITE ERR", tft.color565(230, 60, 40));
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      drawOtaStatus("OTA", "REBOOTING", C_GREEN);
    } else {
      drawOtaStatus("OTA", "FAILED", tft.color565(230, 60, 40));
      busy = false;
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.end();
    drawOtaStatus("OTA", "ABORTED", tft.color565(230, 60, 40));
    busy = false;
  }
}

void markAction() {
  lastActionMs = millis();
}

void showNormal() {
  markAction();
  termMode = false;
  currentView = VIEW_EYES_NORMAL;
  animNormalEyes();
}

void showSquish() {
  markAction();
  termMode = false;
  currentView = VIEW_EYES_SQUISH;
  animSquishEyes();
}

void showCodeTerminal() {
  markAction();
  currentView = VIEW_CODE;
  drawCodeView();
  termMode = true;
  termClear();
  termFullRedraw();
}

bool runNamedCommand(String name) {
  name.trim();
  name.toLowerCase();
  if (name == "normal" || name == "w") { showNormal(); return true; }
  return false;
}

void routeCmd() {
  if (!server.hasArg("k") || server.arg("k").isEmpty()) {
    server.send(400, "application/json", "{\"e\":1}"); return;
  }
  if (runNamedCommand(server.arg("k"))) {
    server.send(200, "application/json", "{\"ok\":1}");
  } else {
    server.send(400, "application/json", "{\"e\":1}");
  }
}

void routeChar() {
  if (!termMode) { server.send(200, "application/json", "{\"ok\":1}"); return; }
  const String val = server.arg("c");
  if (val.length() > 0) termAddChar(val[0]);
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeSpeed() {
  markAction();
  if (server.hasArg("v")) animSpeed = constrain(server.arg("v").toInt(), 1, 3);
  server.send(200, "application/json", "{\"ok\":1}");
}

// /redraw?bg=hex — set animBg and immediately redraw current view
void routeRedraw() {
  markAction();
  if (server.hasArg("bg")) {
    animBgColor = hexToRgb565(server.arg("bg"));
    drawBgColor = animBgColor;
  }
  switch (currentView) {
    case VIEW_EYES_NORMAL: drawNormalEyes(); break;
    case VIEW_EYES_SQUISH: drawSquishEyes(); break;
    case VIEW_CODE:        drawCodeView();   break;
    case VIEW_DRAW:        tft.fillScreen(drawBgColor); break;
    case VIEW_PROGRESS:    drawProgressView(); break;
  }
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeCanvas() {
  markAction();
  const bool on = server.hasArg("on") && server.arg("on") == "1";
  if (on) { currentView = VIEW_DRAW; tft.fillScreen(drawBgColor); }
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeDrawClear() {
  markAction();
  const String bg = server.hasArg("bg") ? server.arg("bg") : "#aa4818";
  drawBgColor = hexToRgb565(bg);
  animBgColor = drawBgColor;  // keep in sync
  currentView = VIEW_DRAW; termMode = false;
  tft.fillScreen(drawBgColor);
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeDrawStroke() {
  markAction();
  if (!server.hasArg("pts") || !server.hasArg("pen")) {
    server.send(200, "application/json", "{\"ok\":1}"); return;
  }
  const uint16_t color = hexToRgb565(server.arg("pen"));
  const String   data  = server.arg("pts");
  currentView = VIEW_DRAW;

  struct Pt { int16_t x, y; };
  Pt prev = {-1, -1};
  int start = 0;
  while (start < (int)data.length()) {
    int semi = data.indexOf(';', start);
    if (semi == -1) semi = data.length();
    String entry = data.substring(start, semi);
    const int comma = entry.indexOf(',');
    if (comma > 0) {
      const int16_t x = entry.substring(0, comma).toInt();
      const int16_t y = entry.substring(comma + 1).toInt();
      if (prev.x >= 0) {
        tft.drawLine(prev.x, prev.y, x, y, color);
        tft.drawLine(prev.x + 1, prev.y, x + 1, y, color);
        tft.drawLine(prev.x, prev.y + 1, x, y + 1, color);
      } else {
        tft.fillCircle(x, y, 2, color);
      }
      prev = {x, y};
    }
    start = semi + 1;
  }
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeBacklight() {
  markAction();
  setBacklight(server.hasArg("on") && server.arg("on") == "1");
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeProgress() {
  const String state = server.hasArg("state") ? server.arg("state") : "";
  const String msg = server.hasArg("msg") ? server.arg("msg") : "";
  String normalized = state;
  normalized.trim();
  normalized.toUpperCase();
  if (!setProgress(normalized, msg)) {
    server.send(400, "application/json", "{\"e\":1}");
    return;
  }
  if (normalized != PROGRESS_OFFLINE) markAction();
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeExpr() {
  markAction();
  const String name = server.hasArg("name") ? server.arg("name") : "";
  if (!setCompanionExpr(name)) {
    server.send(400, "application/json", "{\"e\":1}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeIdle() {
  markAction();
  if (server.hasArg("on")) idleEnabled = server.arg("on") == "1";
  server.send(200, "application/json", "{\"ok\":1}");
}

// Convert RGB565 back to #RRGGBB for state endpoint
String rgb565ToHex(uint16_t c) {
  uint8_t r = ((c >> 11) & 0x1F) << 3;
  uint8_t g = ((c >> 5)  & 0x3F) << 2;
  uint8_t b = (c & 0x1F) << 3;
  char buf[8];
  snprintf(buf, sizeof(buf), "#%02x%02x%02x", r, g, b);
  return String(buf);
}

String jsonEscape(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  return s;
}

String stateJson() {
  String j = "{\"view\":"; j += currentView;
  j += ",\"busy\":";   j += busy        ? "true" : "false";
  j += ",\"bl\":";     j += backlightOn ? "true" : "false";
  j += ",\"progress\":\""; j += jsonEscape(progressState); j += "\"";
  j += ",\"progressMsg\":\""; j += jsonEscape(progressMsg); j += "\"";
  j += ",\"expr\":";   j += companionExpr;
  j += ",\"wifiMode\":\"AP_STA\"";
  j += ",\"wifiConnected\":"; j += WiFi.status() == WL_CONNECTED ? "true" : "false";
  j += ",\"wifiSsid\":\""; j += WiFi.status() == WL_CONNECTED ? jsonEscape(WiFi.SSID()) : ""; j += "\"";
  j += ",\"wifiIp\":\""; j += WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : ""; j += "\"";
  j += ",\"apIp\":\""; j += WiFi.softAPIP().toString(); j += "\"";
  j += "}";
  return j;
}

void routeState() {
  String j = stateJson();
  server.send(200, "application/json", j);
}

String handleSerialCommand(String line) {
  line.trim();
  if (line.length() == 0) return "";

  String upper = line;
  upper.toUpperCase();
  if (upper == "STATE") return stateJson();

  if (upper.startsWith("CMD ")) {
    markAction();
    String name = line.substring(4);
    if (runNamedCommand(name)) return "OK";
    return "ERR unknown-command";
  }

  if (upper.startsWith("BL ")) {
    markAction();
    const String v = line.substring(3);
    if (v == "0" || v == "1") {
      setBacklight(v == "1");
      return "OK";
    }
    return "ERR bad-value";
  }

  if (upper.startsWith("PROGRESS ")) {
    const int firstSpace = line.indexOf(' ', 9);
    String state = firstSpace < 0 ? line.substring(9) : line.substring(9, firstSpace);
    String msg = firstSpace < 0 ? "" : line.substring(firstSpace + 1);
    String normalized = state;
    normalized.trim();
    normalized.toUpperCase();
    if (setProgress(normalized, msg)) {
      if (normalized != PROGRESS_OFFLINE) markAction();
      return "OK";
    }
    return "ERR bad-state";
  }

  return "ERR unknown-command";
}

void handleSerial() {
  while (Serial.available() > 0) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      const String response = handleSerialCommand(serialLine);
      serialLine = "";
      if (response.length() > 0) Serial.println(response);
    } else if (serialLine.length() < 120) {
      serialLine += c;
    } else {
      serialLine = "";
      Serial.println("ERR line-too-long");
    }
  }
}

void routeNotFound() { server.send(404, "text/plain", "not found"); }

// ═════════════════════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);

  pinMode(TFT_BLK, OUTPUT);
  setBacklight(true);

  SPI.begin(8, -1, 10, TFT_CS);   // SCK=8, MOSI=10
  tft.init(240, 240);
  tft.setSPISpeed(40000000);
  tft.setRotation(1);
  initColours();

  // ── Boot splash ────────────────────────────────────────────
  tft.fillScreen(animBgColor);
  tft.setTextColor(C_WHITE); tft.setTextSize(3);
  tft.setCursor(DISP_W / 2 - 54, DISP_H / 2 - 22); tft.print("Clawd");
  tft.setCursor(DISP_W / 2 - 54, DISP_H / 2 + 14); tft.print("Mochi");
  delay(1200);

  // ── Logo shown once at startup ─────────────────────────────
  animLogoReveal();

  // ── Start WiFi ─────────────────────────────────────────────
  startSetupAp();
  wifiPrefs.begin("clawd-wifi", false);
  savedWifiSsid = wifiPrefs.getString("ssid", "");
  savedWifiPassword = wifiPrefs.getString("password", "");
  connectSavedWifi();
  const uint32_t wifiConnectStart = millis();
  while (!savedWifiSsid.isEmpty() && WiFi.status() != WL_CONNECTED &&
         millis() - wifiConnectStart < 6000) {
    delay(100);
  }

  // ── WiFi info screen (stays until first web request) ───────
  tft.fillScreen(C_DARKBG);
  tft.fillRect(0, 0, DISP_W, 4, C_ORANGE);
  tft.setTextColor(C_WHITE);  tft.setTextSize(2);
  tft.setCursor(12, 16);  tft.print("WiFi: ClaWD-Mochi");
  tft.setTextColor(C_MUTED);  tft.setTextSize(1);
  tft.setCursor(12, 44);  tft.print("password: clawd1234");
  tft.setTextColor(C_WHITE);  tft.setTextSize(2);
  tft.setCursor(12, 68);  tft.print("Setup:");
  tft.setTextColor(C_ORANGE); tft.setTextSize(2);
  tft.setCursor(12, 94);  tft.print("192.168.4.1");
  tft.setTextColor(C_MUTED);  tft.setTextSize(1);
  tft.setCursor(12, 124);
  if (WiFi.status() == WL_CONNECTED) {
    tft.print("LAN: "); tft.print(WiFi.localIP());
  } else {
    tft.print("Open /network to configure");
  }

  // ── Register routes ────────────────────────────────────────
  server.on("/",            HTTP_GET, routeRoot);
  server.on("/network",     HTTP_GET, routeNetworkPage);
  server.on("/wifi/scan",   HTTP_GET, routeWifiScan);
  server.on("/wifi/connect", HTTP_POST, routeWifiConnect);
  server.on("/wifi/clear",  HTTP_POST, routeWifiClear);
  server.on("/ota",         HTTP_GET, routeOtaPage);
  server.on("/ota",         HTTP_POST, routeOtaResult, handleOtaUpload);
  server.on("/cmd",         HTTP_GET, routeCmd);
  server.on("/backlight",   HTTP_GET, routeBacklight);
  server.on("/progress",    HTTP_GET, routeProgress);
  server.on("/expr",        HTTP_GET, routeExpr);
  server.on("/state",       HTTP_GET, routeState);
  server.onNotFound(routeNotFound);
  server.begin();

  delay(1800);
  progressState = PROGRESS_OFFLINE;
  progressMsg = "codex-offline";
  drawDefaultClawdView();
}

// ═════════════════════════════════════════════════════════════
//  LOOP
// ═════════════════════════════════════════════════════════════

void loop() {
  server.handleClient();
  handleSerial();
  checkCodexOfflineTimeout();
  progressTick();
}
