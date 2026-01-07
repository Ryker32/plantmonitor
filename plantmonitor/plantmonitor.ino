#include <Arduino.h>
#include "epd_2inch13.h"
#include "epd_gui.h"
#include "fonts.h"

// ====== SETTINGS ======
const int MOISTURE_PIN = 34;
const uint32_t UPDATE_MS = 30UL * 60UL * 1000UL;  // 30 minutes

// Calibrate:
//  - sensor in air  -> DRY_RAW
//  - sensor in water/soaked soil -> WET_RAW
const int DRY_RAW = 3200;   // <-- change
const int WET_RAW = 1400;   // <-- change

// Mood thresholds (%)
const int THRESH_DRY = 35;
const int THRESH_WET = 70;

// IMPORTANT: "No rotation"
static const int SCREEN_ROTATE = ROTATE_0;

// With ROTATE_0, GUI coordinates are:
static const int CANVAS_W = EPD_WIDTH;   // 122
static const int CANVAS_H = EPD_HEIGHT;  // 250

// ====== helpers ======
static int clampi(int v, int lo, int hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

static int readMoistureRawAveraged() {
  const int N = 16;
  long sum = 0;
  for (int i = 0; i < N; i++) {
    sum += analogRead(MOISTURE_PIN);
    delay(5);
  }
  return (int)(sum / N);
}

static int moisturePercent(int raw) {
  long denom = (long)(DRY_RAW - WET_RAW);
  if (denom == 0) return 0;
  long pct = (long)(DRY_RAW - raw) * 100L / denom;  // assumes DRY_RAW > WET_RAW (common)
  return clampi((int)pct, 0, 100);
}

static const char* statusFromPct(int pct) {
  if (pct < THRESH_DRY) return "DRY";
  if (pct >= THRESH_WET) return "WET";
  return "OK";
}

static void epdPinsInit() {
  pinMode(BUSY_Pin, INPUT_PULLUP);
  pinMode(RES_Pin, OUTPUT);
  pinMode(DC_Pin, OUTPUT);
  pinMode(CS_Pin, OUTPUT);
  pinMode(SCK_Pin, OUTPUT);
  pinMode(SDI_Pin, OUTPUT);
}

// draw a clean percent symbol (fonts often mess it up)
static void drawPercentSymbol(int x, int y) {
  Gui_Draw_Circle(x + 2,  y + 2,  2, BLACK, FULL,  PIXEL_1X1);
  Gui_Draw_Circle(x + 12, y + 14, 2, BLACK, FULL,  PIXEL_1X1);
  Gui_Draw_Line  (x + 12, y + 2,  x + 2,  y + 14, BLACK, PIXEL_1X1, SOLID);
}

static void drawFaceAt(int cx, int cy, int r, int pct) {
  Gui_Draw_Circle(cx, cy, r, BLACK, EMPTY, PIXEL_2X2);

  int ex = r / 3;
  int ey = r / 4;
  int er = 3;
  Gui_Draw_Circle(cx - ex, cy - ey, er, BLACK, FULL, PIXEL_1X1);
  Gui_Draw_Circle(cx + ex, cy - ey, er, BLACK, FULL, PIXEL_1X1);

  if (pct >= THRESH_WET) {
    // smile
    Gui_Draw_Line(cx - r/2, cy + r/6, cx - r/4, cy + r/3, BLACK, PIXEL_2X2, SOLID);
    Gui_Draw_Line(cx - r/4, cy + r/3, cx + r/4, cy + r/3, BLACK, PIXEL_2X2, SOLID);
    Gui_Draw_Line(cx + r/4, cy + r/3, cx + r/2, cy + r/6, BLACK, PIXEL_2X2, SOLID);
  } else if (pct < THRESH_DRY) {
    // sad
    Gui_Draw_Line(cx - r/2, cy + r/2, cx - r/4, cy + r/3, BLACK, PIXEL_2X2, SOLID);
    Gui_Draw_Line(cx - r/4, cy + r/3, cx + r/4, cy + r/3, BLACK, PIXEL_2X2, SOLID);
    Gui_Draw_Line(cx + r/4, cy + r/3, cx + r/2, cy + r/2, BLACK, PIXEL_2X2, SOLID);
  } else {
    // neutral
    Gui_Draw_Line(cx - r/2 + 6, cy + r/3, cx + r/2 - 6, cy + r/3, BLACK, PIXEL_2X2, SOLID);
  }
}

static void renderScreen(int pct) {
  Gui_SelectImage(BWimage);
  Gui_Clear(WHITE);

  // Header
  Gui_Draw_Str(0, 0, "Plant Monitor", &Font24, WHITE, BLACK);
  Gui_Draw_Line(0, 28, CANVAS_W - 1, 28, BLACK, PIXEL_1X1, SOLID);

  // Left column (text)
  const int leftX = 4;
  const int y0 = 48;

  Gui_Draw_Str(leftX, y0, "Moisture", &Font16, WHITE, BLACK);

  char num[8];
  snprintf(num, sizeof(num), "%d", pct);
  Gui_Draw_Str(leftX, y0 + 20, num, &Font24, WHITE, BLACK);

  int xPct = leftX + (pct < 10 ? 18 : (pct < 100 ? 32 : 46));
  drawPercentSymbol(xPct, y0 + 28);

  char st[24];
  snprintf(st, sizeof(st), "Status:%s", statusFromPct(pct));
  Gui_Draw_Str(leftX, y0 + 58, st, &Font16, WHITE, BLACK);

  // Bar at bottom
  int barX = leftX;
  int barY = CANVAS_H - 18;
  int barW = 66;
  int barH = 10;
  Gui_Draw_Rectangle(barX, barY, barX + barW, barY + barH, BLACK, EMPTY, PIXEL_1X1);
  int fillW = (barW - 2) * pct / 100;
  Gui_Draw_Rectangle(barX + 1, barY + 1, barX + 1 + fillW, barY + barH - 1, BLACK, FULL, PIXEL_1X1);

  // Right column (face)
  int faceR  = 26;
  int faceCx = 88;          // right side of 122px-wide screen
  int faceCy = 120;         // center-ish in 250px height
  drawFaceAt(faceCx, faceCy, faceR, pct);

  // RW unused
  Gui_SelectImage(RWimage);
  Gui_Clear(WHITE);
}

static void updateDisplay(int pct) {
  renderScreen(pct);
  EPD_Display(BWimage, RWimage);
}

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  epdPinsInit();

  // IMPORTANT: use the normal init (not GUI init), and clear once
  EPD_HW_Init();
  EPD_WhiteScreen_White();

  // Set GUI rotation explicitly
  Image_Init(BWimage, EPD_WIDTH, EPD_HEIGHT, SCREEN_ROTATE, WHITE);
  Image_Init(RWimage, EPD_WIDTH, EPD_HEIGHT, SCREEN_ROTATE, WHITE);

  int raw = readMoistureRawAveraged();
  int pct = moisturePercent(raw);
  Serial.printf("BOOT raw=%d pct=%d\n", raw, pct);

  updateDisplay(pct);
}

void loop() {
  static uint32_t lastUpdate = 0;
  uint32_t now = millis();
  if ((uint32_t)(now - lastUpdate) < UPDATE_MS) return;
  lastUpdate = now;

  int raw = readMoistureRawAveraged();
  int pct = moisturePercent(raw);
  Serial.printf("raw=%d pct=%d\n", raw, pct);

  updateDisplay(pct);
}
