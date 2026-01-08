#include <Arduino.h>
#include <string.h>
#include "epd_2inch13.h"
#include "epd_gui.h"
#include "fonts.h"

// ====== USER SETTINGS ======
const int MOISTURE_PIN = 34;                      // ESP32 ADC pin (GPIO34 ok)
const uint32_t UPDATE_MS = 30UL * 60UL * 1000UL;  // 30 minutes

// Calibrate:
//  - sensor in air  -> DRY_RAW
//  - sensor in water (or soaked soil) -> WET_RAW
const int DRY_RAW = 3200;  // <-- change
const int WET_RAW = 1400;  // <-- change

// Mood thresholds (percent)
const int THRESH_DRY = 35;   // < 35% => sad
const int THRESH_WET = 70;   // >= 70% => happy

// Landscape canvas dimensions when using ROTATE_90/270
#define SCREEN_W  EPD_HEIGHT   // 250
#define SCREEN_H  EPD_WIDTH    // 122

#define HEADER_H  28
#define BOX_X     0
#define BOX_Y     (HEADER_H + 4)
#define BOX_W     ((SCREEN_W / 8) * 8)   // keep multiple of 8
#define BOX_H     (SCREEN_H - BOX_Y)

// ====== INTERNAL ======
#define SCR_ROW_BYTES ((EPD_WIDTH % 8 == 0) ? (EPD_WIDTH / 8) : (EPD_WIDTH / 8 + 1))
#define BOX_ROW_BYTES ((BOX_W   % 8 == 0) ? (BOX_W   / 8) : (BOX_W   / 8 + 1))

static uint8_t boxBuf[BOX_ROW_BYTES * BOX_H];

static int clampi(int v, int lo, int hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

static int moisturePercent(int raw)
{
  long pct = (long)(DRY_RAW - raw) * 100L / (long)(DRY_RAW - WET_RAW);
  return clampi((int)pct, 0, 100);
}

static int readMoistureRaw()
{
  const int N = 16;
  long sum = 0;
  for (int i = 0; i < N; i++)
  {
    sum += analogRead(MOISTURE_PIN);
    delay(5);
  }
  return (int)(sum / N);
}

static const char* statusFromPct(int pct)
{
  if (pct < THRESH_DRY) return "DRY";
  if (pct >= THRESH_WET) return "WET";
  return "OK";
}

static void epdPinsInit()
{
  pinMode(BUSY_Pin, INPUT_PULLUP);
  pinMode(RES_Pin, OUTPUT);
  pinMode(DC_Pin, OUTPUT);
  pinMode(CS_Pin, OUTPUT);
  pinMode(SCK_Pin, OUTPUT);
  pinMode(SDI_Pin, OUTPUT);
}

// Copy BOX region from BWimage into boxBuf (only used if you switch back to partial update)
static void copyBoxFromBWimage()
{
  int srcByteX = BOX_X / 8;
  for (int y = 0; y < BOX_H; y++)
  {
    const uint8_t* src = BWimage + (BOX_Y + y) * SCR_ROW_BYTES + srcByteX;
    uint8_t* dst = boxBuf + y * BOX_ROW_BYTES;
    memcpy(dst, src, BOX_ROW_BYTES);
  }
}

static void partialUpdateBox()
{
  copyBoxFromBWimage();
  // signature: (x, y, data, PART_COLUMN=height, PART_LINE=width)
  EPD_Dis_Part(BOX_X, BOX_Y, boxBuf, BOX_H, BOX_W);
}

// Fonts in this repo often render '%' weird, so draw a clean % ourselves
static void drawPercentSymbol(int x, int y)
{
  Gui_Draw_Circle(x + 2,  y + 2,  2, BLACK, FULL, PIXEL_1X1);
  Gui_Draw_Circle(x + 12, y + 14, 2, BLACK, FULL, PIXEL_1X1);
  Gui_Draw_Line  (x + 12, y + 2,  x + 2,  y + 14, BLACK, PIXEL_1X1, SOLID);
}

static void drawFaceAt(int cx, int cy, int r, int pct)
{
  // head
  Gui_Draw_Circle(cx, cy, r, BLACK, EMPTY, PIXEL_2X2);

  // eyes
  int ex = r / 3;
  int ey = r / 4;
  int er = 3;
  Gui_Draw_Circle(cx - ex, cy - ey, er, BLACK, FULL, PIXEL_1X1);
  Gui_Draw_Circle(cx + ex, cy - ey, er, BLACK, FULL, PIXEL_1X1);

  // mouth
  if (pct >= THRESH_WET)
  {
    Gui_Draw_Line(cx - r/2, cy + r/6, cx - r/4, cy + r/3, BLACK, PIXEL_2X2, SOLID);
    Gui_Draw_Line(cx - r/4, cy + r/3, cx + r/4, cy + r/3, BLACK, PIXEL_2X2, SOLID);
    Gui_Draw_Line(cx + r/4, cy + r/3, cx + r/2, cy + r/6, BLACK, PIXEL_2X2, SOLID);
  }
  else if (pct < THRESH_DRY)
  {
    Gui_Draw_Line(cx - r/2, cy + r/2, cx - r/4, cy + r/3, BLACK, PIXEL_2X2, SOLID);
    Gui_Draw_Line(cx - r/4, cy + r/3, cx + r/4, cy + r/3, BLACK, PIXEL_2X2, SOLID);
    Gui_Draw_Line(cx + r/4, cy + r/3, cx + r/2, cy + r/2, BLACK, PIXEL_2X2, SOLID);
  }
  else
  {
    Gui_Draw_Line(cx - r/2 + 6, cy + r/3, cx + r/2 - 6, cy + r/3, BLACK, PIXEL_2X2, SOLID);
  }
}

static void drawStaticLayout()
{
  Gui_SelectImage(BWimage);
  Gui_Clear(WHITE);

  Gui_Draw_Str(0, 0, "Plant Monitor", &Font24, WHITE, BLACK);
  Gui_Draw_Line(0, HEADER_H, BOX_W - 1, HEADER_H, BLACK, PIXEL_1X1, SOLID);

  // Outline for dynamic region
  Gui_Draw_Rectangle(BOX_X, BOX_Y, BOX_X + BOX_W - 1, BOX_Y + BOX_H - 1, BLACK, EMPTY, PIXEL_1X1);

  // Make sure RW buffer is "white"/empty
  Gui_SelectImage(RWimage);
  Gui_Clear(WHITE);
}

static void drawDynamic(int pct)
{
  Gui_SelectImage(BWimage);

  // Clear inside the box (leave outline)
  Gui_Draw_Rectangle(BOX_X + 1, BOX_Y + 1, BOX_X + BOX_W - 2, BOX_Y + BOX_H - 2,
                     WHITE, FULL, PIXEL_1X1);

  const int leftX = 6;

  Gui_Draw_Str(leftX, BOX_Y + 10, "Moisture", &Font16, WHITE, BLACK);

  char pctStr[8];
  snprintf(pctStr, sizeof(pctStr), "%d", pct);
  Gui_Draw_Str(leftX, BOX_Y + 34, pctStr, &Font24, WHITE, BLACK);

  // manual %
  int xPct = leftX + (pct < 10 ? 18 : (pct < 100 ? 32 : 46));
  drawPercentSymbol(xPct, BOX_Y + 40);

  char stLine[24];
  snprintf(stLine, sizeof(stLine), "Status: %s", statusFromPct(pct));
  Gui_Draw_Str(leftX, BOX_Y + 72, stLine, &Font16, WHITE, BLACK);

  // bar
  int barX = leftX;
  int barY = BOX_Y + BOX_H - 16;
  int barW = 56;
  int barH = 8;
  Gui_Draw_Rectangle(barX, barY, barX + barW, barY + barH, BLACK, EMPTY, PIXEL_1X1);
  int fillW = (barW - 2) * pct / 100;
  Gui_Draw_Rectangle(barX + 1, barY + 1, barX + 1 + fillW, barY + barH - 1, BLACK, FULL, PIXEL_1X1);

  // Face in right side of the box
  const int faceR  = 18;
  const int faceCx = BOX_X + (BOX_W * 3) / 4;
  const int faceCy = BOX_Y + (BOX_H / 2);

  drawFaceAt(faceCx, faceCy, faceR, pct);
}

void setup()
{
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  epdPinsInit();

  // IMPORTANT: GUI init matches GUI buffer + rotation use
  EPD_HW_Init_GUI();
  EPD_WhiteScreen_White();

  // FIX: use the opposite landscape rotation
  Image_Init(BWimage, EPD_WIDTH, EPD_HEIGHT, ROTATE_90, WHITE);
  Image_Init(RWimage, EPD_WIDTH, EPD_HEIGHT, ROTATE_90, WHITE);
  Gui_SetMirror(MIRROR_NONE);

  drawStaticLayout();

  int raw = readMoistureRaw();
  int pct = moisturePercent(raw);
  Serial.printf("BOOT raw=%d pct=%d\n", raw, pct);

  drawDynamic(pct);
  EPD_Display(BWimage, RWimage);
}

void loop()
{
  static uint32_t lastUpdate = 0;
  uint32_t now = millis();
  if ((uint32_t)(now - lastUpdate) < UPDATE_MS) return;
  lastUpdate = now;

  int raw = readMoistureRaw();
  int pct = moisturePercent(raw);
  Serial.printf("raw=%d pct=%d\n", raw, pct);

  drawDynamic(pct);

  // Full refresh each update (your current loop behavior)
  EPD_Display(BWimage, RWimage);

  // If you want to go back to partial update later, use:
  // partialUpdateBox();
}
