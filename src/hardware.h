#pragma once

#include <Arduino.h>

#if defined(BUDDY_M5STICKS3)
  #include <M5Unified.h>
  using BuddyDisplay = LovyanGFX;
  using BuddySprite = M5Canvas;
#elif defined(BUDDY_M5STICKC_PLUS)
  #include <M5StickCPlus.h>
  using BuddyDisplay = TFT_eSPI;
  using BuddySprite = TFT_eSprite;
#else
  #error "Select a supported board: BUDDY_M5STICKC_PLUS or BUDDY_M5STICKS3"
#endif

static constexpr int BUDDY_SCREEN_W = 135;
static constexpr int BUDDY_SCREEN_H = 240;
static constexpr int BUDDY_LANDSCAPE_W = 240;
static constexpr int BUDDY_LANDSCAPE_H = 135;

struct BuddyRtcTime {
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
};

struct BuddyRtcDate {
  uint16_t year;
  uint8_t month;
  uint8_t date;
  uint8_t weekday;
};

void buddyHardwareBegin();
void buddyHardwareUpdate();
BuddyDisplay& buddyDisplay();

void buddySetRotation(uint8_t rotation);
void buddyPushSprite(BuddySprite& sprite, uint16_t bg);
void buddySetBrightness(uint8_t level);
void buddyScreenOn(uint8_t level);
void buddyScreenOff();
void buddyPowerOff();
uint8_t buddyPowerButtonPress();

void buddyGetAccel(float* ax, float* ay, float* az);
bool buddyIsSideways(float ax, float ay, float az, bool entering);
uint8_t buddyLandscapeRotation(float ax, float ay, float az);
void buddyBeep(uint16_t freq, uint16_t dur);
void buddyBeepUpdate();
void buddySetLed(bool on);

int buddyBatteryVoltageMv();
int buddyBatteryCurrentMa();
int buddyVbusVoltageMv();
int buddyTemperatureC();

void buddyRtcGet(BuddyRtcTime* time, BuddyRtcDate* date);
void buddyRtcSet(const BuddyRtcTime& time, const BuddyRtcDate& date);
