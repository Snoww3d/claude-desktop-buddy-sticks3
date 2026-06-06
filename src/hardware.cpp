#include "hardware.h"

#if defined(BUDDY_M5STICKS3)
#include <sys/time.h>
#include <time.h>
#endif

BuddyDisplay& buddyDisplay() {
#if defined(BUDDY_M5STICKS3)
  return M5.Display;
#else
  return M5.Lcd;
#endif
}

#if defined(BUDDY_M5STICKS3)
static uint32_t speakerStopAt = 0;
#endif

void buddyHardwareBegin() {
#if defined(BUDDY_M5STICKS3)
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  cfg.internal_imu = true;
  cfg.internal_rtc = true;
  cfg.internal_spk = true;
  M5.begin(cfg);
  M5.Display.setRotation(0);
  // ES8311 codec + AW8737 amp on the StickS3 are quiet at the fork's default
  // of 64/255 — chirps were barely audible. Near-max digital volume is needed
  // for the alert/button beeps to actually carry across a desk.
  M5.Speaker.setVolume(255);
  M5.Speaker.stop();
#else
  M5.begin();
  M5.Lcd.setRotation(0);
  M5.Imu.Init();
  M5.Beep.begin();
#endif
}

void buddyHardwareUpdate() {
  M5.update();
}

void buddySetRotation(uint8_t rotation) {
  buddyDisplay().setRotation(rotation);
}

void buddyPushSprite(BuddySprite& sprite, uint16_t bg) {
#if defined(BUDDY_M5STICKS3)
  int x = (buddyDisplay().width() - sprite.width()) / 2;
  int y = (buddyDisplay().height() - sprite.height()) / 2;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x > 0) {
    buddyDisplay().fillRect(0, y, x, sprite.height(), bg);
    buddyDisplay().fillRect(x + sprite.width(), y,
                            buddyDisplay().width() - x - sprite.width(),
                            sprite.height(), bg);
  }
  if (y > 0) {
    buddyDisplay().fillRect(0, 0, buddyDisplay().width(), y, bg);
    buddyDisplay().fillRect(0, y + sprite.height(),
                            buddyDisplay().width(),
                            buddyDisplay().height() - y - sprite.height(), bg);
  }
  sprite.pushSprite(x, y);
#else
  sprite.pushSprite(0, 0);
#endif
}

void buddySetBrightness(uint8_t level) {
#if defined(BUDDY_M5STICKS3)
  M5.Display.setBrightness(level);
#else
  M5.Axp.ScreenBreath(level);
#endif
}

void buddyScreenOn(uint8_t level) {
#if defined(BUDDY_M5STICKC_PLUS)
  M5.Axp.SetLDO2(true);
#endif
  buddySetBrightness(level);
}

void buddyScreenOff() {
#if defined(BUDDY_M5STICKS3)
  M5.Display.setBrightness(0);
#else
  M5.Axp.SetLDO2(false);
#endif
}

void buddyPowerOff() {
#if defined(BUDDY_M5STICKS3)
  M5.Power.powerOff();
#else
  M5.Axp.PowerOff();
#endif
}

uint8_t buddyPowerButtonPress() {
#if defined(BUDDY_M5STICKC_PLUS)
  return M5.Axp.GetBtnPress();
#else
  return M5.Power.getKeyState();
#endif
}

void buddyGetAccel(float* ax, float* ay, float* az) {
#if defined(BUDDY_M5STICKS3)
  M5.Imu.getAccel(ax, ay, az);
#else
  M5.Imu.getAccelData(ax, ay, az);
#endif
}

bool buddyIsSideways(float ax, float ay, float az, bool entering) {
#if defined(BUDDY_M5STICKS3)
  // StickS3 IMU frame: X = long axis (upright USB-down gives ax≈-1),
  // Y = short axis (sideways gives |ay|≈1), Z = screen normal.
  // Sideways means gravity along Y, not X.
  return entering
    ? fabsf(ay) > 0.7f && fabsf(ax) < 0.5f && fabsf(az) < 0.5f
    : fabsf(ay) > 0.4f;
#else
  return entering
    ? fabsf(ax) > 0.7f && fabsf(ay) < 0.5f && fabsf(az) < 0.5f
    : fabsf(ax) > 0.4f;
#endif
}

uint8_t buddyLandscapeRotation(float ax, float ay, float az) {
#if defined(BUDDY_M5STICKS3)
  // StickS3: gravity sign along Y picks which landscape direction.
  // Sign may need flipping after physical test.
  return ay > 0 ? 1 : 3;
#else
  return ax > 0 ? 1 : 3;
#endif
}

void buddyBeep(uint16_t freq, uint16_t dur) {
#if defined(BUDDY_M5STICKS3)
  M5.Speaker.stop();
  M5.Speaker.tone(freq, dur, 0, true);
  speakerStopAt = millis() + dur + 20;
#else
  M5.Beep.tone(freq, dur);
#endif
}

void buddyBeepUpdate() {
#if defined(BUDDY_M5STICKC_PLUS)
  M5.Beep.update();
#else
  if (speakerStopAt && (int32_t)(millis() - speakerStopAt) >= 0) {
    M5.Speaker.stop();
    speakerStopAt = 0;
  }
#endif
}

void buddySetLed(bool on) {
#if defined(BUDDY_M5STICKS3)
  M5.Power.setLed(on ? 255 : 0);
#else
  digitalWrite(10, on ? LOW : HIGH);
#endif
}

int buddyBatteryVoltageMv() {
#if defined(BUDDY_M5STICKS3)
  return M5.Power.getBatteryVoltage();
#else
  return (int)(M5.Axp.GetBatVoltage() * 1000);
#endif
}

int buddyBatteryCurrentMa() {
#if defined(BUDDY_M5STICKS3)
  return M5.Power.getBatteryCurrent();
#else
  return (int)M5.Axp.GetBatCurrent();
#endif
}

int buddyVbusVoltageMv() {
#if defined(BUDDY_M5STICKS3)
  return M5.Power.getVBUSVoltage();
#else
  return (int)(M5.Axp.GetVBusVoltage() * 1000);
#endif
}

int buddyTemperatureC() {
#if defined(BUDDY_M5STICKS3)
  return 0;
#else
  return (int)M5.Axp.GetTempInAXP192();
#endif
}

void buddyRtcGet(BuddyRtcTime* time, BuddyRtcDate* date) {
#if defined(BUDDY_M5STICKS3)
  // StickS3 has no external RTC chip — M5.Rtc.getTime() is a no-op and
  // leaves the out struct uninitialized (0xFF junk). Read the ESP32 system
  // clock instead; buddyRtcSet() below seeds it from the bridge time sync.
  // We store local time as if it were UTC (data.h hands us local components
  // already adjusted by tz_offset), so gmtime_r decomposes consistently.
  timeval tv;
  gettimeofday(&tv, nullptr);
  struct tm lt;
  gmtime_r(&tv.tv_sec, &lt);
  if (time) {
    time->hours   = (uint8_t)lt.tm_hour;
    time->minutes = (uint8_t)lt.tm_min;
    time->seconds = (uint8_t)lt.tm_sec;
  }
  if (date) {
    date->year    = (uint16_t)(lt.tm_year + 1900);
    date->month   = (uint8_t)(lt.tm_mon + 1);
    date->date    = (uint8_t)lt.tm_mday;
    date->weekday = (uint8_t)lt.tm_wday;
  }
#else
  RTC_TimeTypeDef tm;
  RTC_DateTypeDef dt;
  M5.Rtc.GetTime(&tm);
  M5.Rtc.GetDate(&dt);
  if (time) *time = { tm.Hours, tm.Minutes, tm.Seconds };
  if (date) *date = { dt.Year, dt.Month, dt.Date, dt.WeekDay };
#endif
}

void buddyRtcSet(const BuddyRtcTime& time, const BuddyRtcDate& date) {
#if defined(BUDDY_M5STICKS3)
  // Mirror of buddyRtcGet: write to ESP32 system clock. M5.Rtc.setTime is
  // also called for boards that happen to have a real RTC chip — it's a
  // no-op when the chip isn't present, harmless when it is.
  struct tm lt = {};
  lt.tm_hour = time.hours;
  lt.tm_min  = time.minutes;
  lt.tm_sec  = time.seconds;
  lt.tm_year = (int)date.year - 1900;
  lt.tm_mon  = (int)date.month - 1;
  lt.tm_mday = date.date;
  // mktime interprets lt in the current TZ; force UTC so the round-trip
  // through gmtime_r in buddyRtcGet yields the same local components.
  setenv("TZ", "GMT0", 1);
  tzset();
  timeval tv = { mktime(&lt), 0 };
  settimeofday(&tv, nullptr);
  m5::rtc_time_t mt { (int8_t)time.hours, (int8_t)time.minutes, (int8_t)time.seconds };
  m5::rtc_date_t md { (int16_t)date.year, (int8_t)date.month, (int8_t)date.date, (int8_t)date.weekday };
  M5.Rtc.setTime(&mt);
  M5.Rtc.setDate(&md);
#else
  RTC_TimeTypeDef tm = { time.hours, time.minutes, time.seconds };
  RTC_DateTypeDef dt = { date.weekday, date.month, date.date, date.year };
  M5.Rtc.SetTime(&tm);
  M5.Rtc.SetDate(&dt);
#endif
}
