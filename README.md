# claude-desktop-buddy (M5StickS3 fork)

This is a fork of [anthropics/claude-desktop-buddy][upstream] that adds
**experimental** support for the **M5StickS3** (ESP32-S3-PICO + 135×240
ST7789). The upstream reference targets the M5StickC Plus only; this
fork keeps that build working and adds a parallel build for the StickS3.

[upstream]: https://github.com/anthropics/claude-desktop-buddy

> **Experimental status.** Builds, BLE pairs, syncs time, renders the
> pet + landscape clock, and approve/deny over BLE all work in casual
> use. Not physically verified: both landscape rotation directions
> (only one has been confirmed), face-down nap on this IMU, long-running
> stability beyond a couple of hours, and the power button short-press
> semantics. Treat any of these as "probably works, file an issue if it
> doesn't" for now.

Everything below is the upstream README with M5StickS3-specific notes
inlined. Skip to **[Building for M5StickS3](#building-for-m5sticks3)** if
that's what you're here for.

---

Claude for macOS and Windows can connect Claude Cowork and Claude Code to
maker devices over BLE, so developers and makers can build hardware that
displays permission prompts, recent messages, and other interactions. We've
been impressed by the creativity of the maker community around Claude -
providing a lightweight, opt-in API is our way of making it easier to build
fun little hardware devices that integrate with Claude.

> **Building your own device?** You don't need any of the code here. See
> **[REFERENCE.md](REFERENCE.md)** for the wire protocol: Nordic UART
> Service UUIDs, JSON schemas, and the folder push transport.

As an example, we built a desk pet on ESP32 that lives off permission
approvals and interaction with Claude. It sleeps when nothing's happening,
wakes when sessions start, gets visibly impatient when an approval prompt is
waiting, and lets you approve or deny right from the device.

<p align="center">
  <img src="docs/device.jpg" alt="M5StickC Plus running the buddy firmware" width="500">
</p>

## Hardware

The firmware targets the Arduino framework on two boards:

- **M5StickC Plus** (ESP32 + 135×240 ST7789) — upstream's original target,
  uses the `M5StickCPlus` library directly.
- **M5StickS3** (ESP32-S3-PICO + 135×240 ST7789) — added in this fork,
  uses `M5Unified` with runtime autodetect.

Both share the same display dimensions, sprite layout, and BLE protocol.
The hardware-specific differences (IMU axis mapping, button input, RTC
backing) live behind `src/hardware.h` / `src/hardware.cpp`.

## Flashing

Install
[PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/),
then pick the env that matches your board:

```bash
# M5StickC Plus (upstream default)
pio run -e m5stickc-plus -t upload

# M5StickS3 (this fork)
pio run -e m5stick-s3 -t upload
```

If you're starting from a previously-flashed device, wipe it first
(swap the env to match your board):

```bash
pio run -e m5stickc-plus -t erase && pio run -e m5stickc-plus -t upload
pio run -e m5stick-s3   -t erase && pio run -e m5stick-s3   -t upload
```

Once running, you can also wipe everything from the device itself: **hold A
→ settings → reset → factory reset → tap twice**.

## Building for M5StickS3

The `m5stick-s3` env uses the generic `esp32-s3-devkitc-1` PIO board
profile because there's no dedicated `m5stick-s3` profile upstream.
M5Unified autodetects the actual hardware at runtime and configures the
panel, IMU, RTC, speaker, and PMU based on what it finds — that's why no
StickS3-specific board file is needed.

A second env, `m5stick-s3-diag`, builds the same firmware with the
`BUDDY_DIAG_ROTATION` flag set. That replaces `setup()` / `loop()` with a
rotation overlay (corner labels, live IMU values, dimensions) that's
useful if you're porting to a third board and need to figure out how its
panel orientation and IMU axes line up. Not for normal use.

### Known caveats

- **First-boot LittleFS corruption**: if you flash this fork onto a device
  that's never had a LittleFS partition written, you'll see
  `Corrupted dir pair` on the serial log and the firmware falls back to
  ASCII pet mode (no GIF support until LittleFS is initialized). The
  on-device factory reset (hold A → settings → reset → factory reset →
  tap twice) formats LittleFS and clears the warning.

- **No external RTC chip**: M5StickS3 doesn't have a PCF8563 or similar,
  so `M5.Rtc` is a no-op. The firmware routes RTC reads/writes through
  the ESP32 system clock (`gettimeofday` / `settimeofday`), which the BLE
  bridge seeds via its time-sync message after pairing. Before the first
  sync, the landscape clock shows whatever the system clock holds (usually
  some date in 1970 / 2000, not the wall clock); after pairing it
  catches up within a second. The clock also doesn't survive a hard
  power-cycle on this device — no battery-backed RTC means each cold
  boot needs a fresh sync from the desktop.

- **BT and Wi-Fi settings are placeholders**: the toggles under
  *settings* are stored preferences only. BLE is always on (the BT
  toggle does not turn it off — tearing down the Arduino BLE stack
  reliably isn't supported), and the Wi-Fi toggle has no Wi-Fi stack
  behind it at all on this fork. They're left in the UI for parity
  with the upstream menu layout.

### What changed vs upstream

The board-portability work was bigger than just adding a build env. The
key change was introducing `src/hardware.h` / `src/hardware.cpp` as a thin
abstraction that hides board-specific APIs (display surface type, IMU
reads, RTC, brightness, power button, beeper, LED, battery telemetry)
behind a uniform interface. Most of `main.cpp` was then rewritten to call
that interface instead of `M5StickCPlus` types directly. The StickS3
support drops in as a new branch inside that abstraction layer.

Files touched relative to upstream:

- `src/hardware.h` / `src/hardware.cpp` — the abstraction itself, plus
  the `BUDDY_M5STICKS3` branch: IMU axis mapping (X = long axis,
  Y = short, Z = screen normal), landscape rotation 1↔3 keyed off `ay`
  sign, system-clock RTC fallback.
- `src/main.cpp` — replaced direct `M5StickCPlus.h` / `TFT_eSPI` /
  `TFT_eSprite` uses with `BuddyDisplay` / `BuddySprite` and the
  `buddy*()` wrappers; added an early-return to the diag overlay when
  `BUDDY_DIAG_ROTATION` is defined. About info page picks the hardware
  string by build flag.
- `src/buddy.cpp` / `src/buddy.h` / `src/character.cpp` / `src/character.h`
  / `src/data.h` / `src/xfer.h` / `src/buddies/*.cpp` — minor: switched
  include + sprite/display types to the abstraction.
- `src/diag_rotation.cpp` — new file, only compiled under
  `BUDDY_DIAG_ROTATION`.
- `platformio.ini` — `m5stick-s3` and `m5stick-s3-diag` envs.

## Pairing

To pair your device with Claude, first enable developer mode (**Help →
Troubleshooting → Enable Developer Mode**). Then, open the Hardware Buddy
window in **Developer → Open Hardware Buddy…**, click **Connect**, and pick
your device from the list. macOS will prompt for Bluetooth permission on
first connect; grant it.

<p align="center">
  <img src="docs/menu.png" alt="Developer → Open Hardware Buddy… menu item" width="420">
  <img src="docs/hardware-buddy-window.png" alt="Hardware Buddy window with Connect button and folder drop target" width="420">
</p>

Once paired, the bridge auto-reconnects whenever both sides are awake.

If discovery isn't finding the stick:

- Make sure it's awake (any button press)
- Check the stick's settings menu → bluetooth is on

## Controls

|                         | Normal               | Pet         | Info        | Approval    |
| ----------------------- | -------------------- | ----------- | ----------- | ----------- |
| **A** (front)           | next screen          | next screen | next screen | **approve** |
| **B** (right)           | scroll transcript    | next page   | next page   | **deny**    |
| **Hold A**              | menu                 | menu        | menu        | menu        |
| **Power** (left, short) | toggle screen off    |             |             |             |
| **Power** (left, ~6s)   | hard power off       |             |             |             |
| **Shake**               | dizzy                |             |             | —           |
| **Face-down**           | nap (energy refills) |             |             |             |

The screen auto-powers-off after 30s of no interaction (kept on while an
approval prompt is up). Any button press wakes it.

## ASCII pets

Eighteen pets, each with seven animations (sleep, idle, busy, attention,
celebrate, dizzy, heart). Menu → "next pet" cycles them with a counter.
Choice persists to NVS.

## GIF pets

If you want a custom GIF character instead of an ASCII buddy, drag a
character pack folder onto the drop target in the Hardware Buddy window. The
app streams it over BLE and the stick switches to GIF mode live. **Settings
→ delete char** reverts to ASCII mode.

A character pack is a folder with `manifest.json` and 96px-wide GIFs:

```json
{
  "name": "bufo",
  "colors": {
    "body": "#6B8E23",
    "bg": "#000000",
    "text": "#FFFFFF",
    "textDim": "#808080",
    "ink": "#000000"
  },
  "states": {
    "sleep": "sleep.gif",
    "idle": ["idle_0.gif", "idle_1.gif", "idle_2.gif"],
    "busy": "busy.gif",
    "attention": "attention.gif",
    "celebrate": "celebrate.gif",
    "dizzy": "dizzy.gif",
    "heart": "heart.gif"
  }
}
```

State values can be a single filename or an array. Arrays rotate: each
loop-end advances to the next GIF, useful for an idle activity carousel so
the home screen doesn't loop one clip forever.

GIFs are 96px wide; height up to ~140px stays on a 135×240 portrait screen.
Crop tight to the character — transparent margins waste screen and shrink
the sprite. `tools/prep_character.py` handles the resize: feed it source
GIFs at any sizes and it produces a 96px-wide set where the character is the
same scale in every state.

The whole folder must fit under 1.8MB —
`gifsicle --lossy=80 -O3 --colors 64` typically cuts 40–60%.

See [upstream's `characters/bufo/`][upstream-chars] for a working example.
The bundled GIF assets are not redistributed in this fork — their
license terms aren't documented in upstream, so grab them from upstream
if you want a ready-made character pack, or build your own.

[upstream-chars]: https://github.com/anthropics/claude-desktop-buddy/tree/main/characters/bufo

If you're iterating on a character and would rather skip the BLE round-trip,
`tools/flash_character.py <character-dir> --env m5stick-s3` stages the
folder into `data/` and runs `pio run -t uploadfs -e m5stick-s3` over
USB. Drop the `--env` flag (or change it) to target the StickC Plus
build.

## The seven states

| State       | Trigger                     | Feel                        |
| ----------- | --------------------------- | --------------------------- |
| `sleep`     | bridge not connected        | eyes closed, slow breathing |
| `idle`      | connected, nothing urgent   | blinking, looking around    |
| `busy`      | sessions actively running   | sweating, working           |
| `attention` | approval pending            | alert, **LED blinks**       |
| `celebrate` | level up (every 50K tokens) | confetti, bouncing          |
| `dizzy`     | you shook the stick         | spiral eyes, wobbling       |
| `heart`     | approved in under 5s        | floating hearts             |

## Project layout

```
src/
  main.cpp           — loop, state machine, UI screens
  buddy.cpp          — ASCII species dispatch + render helpers
  buddies/           — one file per species, seven anim functions each
  ble_bridge.cpp     — Nordic UART service, line-buffered TX/RX
  character.cpp      — GIF decode + render
  data.h             — wire protocol, JSON parse
  xfer.h             — folder push receiver
  stats.h            — NVS-backed stats, settings, owner, species choice
  hardware.h/.cpp    — board abstraction (display, IMU, RTC, power, audio)
  diag_rotation.cpp  — rotation/IMU diagnostic overlay (BUDDY_DIAG_ROTATION)
characters/          — example GIF character packs
tools/               — generators and converters
```

## Availability

The BLE API is only available when the desktop apps are in developer mode
(**Help → Troubleshooting → Enable Developer Mode**). It's intended for
makers and developers and isn't an officially supported product feature.
