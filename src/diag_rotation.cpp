#include "hardware.h"

#ifdef BUDDY_DIAG_ROTATION

// Rotation diagnostic. Cycles 0..3 on BtnA. Renders direct to the LCD
// (no sprite) so what you see is exactly what setRotation() produced.
//
// On screen:
//   corners labeled TL/TR/BL/BR — these are the four logical corners after
//     setRotation(N). Comparing them to where they physically land tells you
//     how the framework maps (0,0) for that rotation.
//   center: rotation number + width x height as reported by the display.
//   live IMU ax/ay/az so you can tell which physical axis is gravity in
//     each orientation.
//
// Hold the device in the orientation you want to map (upright, USB-up
// landscape, USB-down landscape, etc.), tap A to step through rotations,
// note which rotation puts TL at the physical top-left.

static uint8_t diagRotation = 0;
static uint8_t lastRotApplied = 0xFF;

static void drawStatic(int w, int h) {
  auto& d = buddyDisplay();
  d.fillScreen(0x0000);
  d.setTextColor(0xFFFF, 0x0000);

  d.setTextDatum(TL_DATUM);
  d.setTextSize(2);
  d.drawString("TL", 4, 4);
  d.setTextDatum(TR_DATUM); d.drawString("TR", w - 4, 4);
  d.setTextDatum(BL_DATUM); d.drawString("BL", 4, h - 4);
  d.setTextDatum(BR_DATUM); d.drawString("BR", w - 4, h - 4);

  d.setTextDatum(MC_DATUM);
  d.setTextSize(3);
  char buf[16];
  snprintf(buf, sizeof(buf), "ROT %u", diagRotation);
  d.drawString(buf, w / 2, h / 2 - 50);
  d.setTextSize(2);
  snprintf(buf, sizeof(buf), "%dx%d", w, h);
  d.drawString(buf, w / 2, h / 2 - 20);

  d.setTextDatum(TL_DATUM);
  d.setTextSize(1);
  d.drawString("BtnA: next rotation", 4, h / 2 + 70);
  d.drawString("BtnB: hold-to-freeze IMU", 4, h / 2 + 82);
}

static void drawImu(int w, int h, bool frozen) {
  auto& d = buddyDisplay();
  float ax = 0, ay = 0, az = 0;
  buddyGetAccel(&ax, &ay, &az);

  d.setTextColor(0xFFFF, 0x0000);
  d.setTextDatum(MC_DATUM);
  d.setTextSize(2);
  char buf[24];
  // trailing spaces overwrite the previous value when it shrinks (e.g. -1.00 -> 0.50)
  snprintf(buf, sizeof(buf), "ax %+0.2f ", ax); d.drawString(buf, w / 2, h / 2 + 10);
  snprintf(buf, sizeof(buf), "ay %+0.2f ", ay); d.drawString(buf, w / 2, h / 2 + 30);
  snprintf(buf, sizeof(buf), "az %+0.2f ", az); d.drawString(buf, w / 2, h / 2 + 50);
  if (frozen) {
    d.setTextSize(1);
    d.drawString("[FROZEN]", w / 2, h / 2 + 65);
  }
  d.setTextDatum(TL_DATUM);
}

void diagRotationSetup() {
  buddyHardwareBegin();
  buddySetBrightness(80);
  Serial.println("DIAG: rotation overlay active (BUDDY_DIAG_ROTATION)");
}

void diagRotationLoop() {
  buddyHardwareUpdate();

  if (M5.BtnA.wasPressed()) {
    diagRotation = (diagRotation + 1) % 4;
    Serial.printf("DIAG: rotation -> %u\n", diagRotation);
  }

  auto& d = buddyDisplay();
  if (lastRotApplied != diagRotation) {
    d.setRotation(diagRotation);
    drawStatic(d.width(), d.height());
    lastRotApplied = diagRotation;
  }

  static uint32_t lastDraw = 0;
  uint32_t now = millis();
  if (now - lastDraw >= 100) {
    lastDraw = now;
    drawImu(d.width(), d.height(), M5.BtnB.isPressed());
  }

  delay(10);
}

#endif  // BUDDY_DIAG_ROTATION
