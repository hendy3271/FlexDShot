# FlexDShot

An Arduino library for **Teensy 4.0 / 4.1 / MicroMod** (NXP i.MX RT1062) that sends **DShot** throttle frames to ESCs using the chip's **FlexPWM** hardware and **DMA**.

Each frame is produced in hardware: the library fills a small buffer with one PWM duty value per bit and a DMA channel writes those values into the FlexPWM compare register on every PWM period. The CPU only builds the 16-bit frame; it does not bit-bang the output.

> Send-only. This library transmits DShot frames. It does not read ESC telemetry (no bidirectional DShot / eRPM, no telemetry UART).

## Features

- DShot150, DShot300, DShot600 and DShot1200 (one speed for all pins, chosen before the first send)
- 11-bit throttle value (0–2047) plus the telemetry-request bit
- Standard 4-bit DShot checksum computed for you
- One DMA channel per pin, so several motors can be driven independently
- Works on FlexPWM channels A, B and X (X outputs are handled with inverted compare values)
- Pins are set up automatically the first time you send to them

## Supported boards

| Board | Status |
|---|---|
| Teensy 4.0 | Supported |
| Teensy 4.1 | Supported |
| Teensy MicroMod | Builds; FlexPWM pins 34–39 match the Teensy 4.0 column below (not hardware-tested) |
| Anything else | Not supported (`#error` at compile time) |

## Which pins work

Only pins wired to a **FlexPWM** submodule work. QuadTimer PWM pins (10, 11, 12, 13, 14, 15, 18, 19) return `DShotError_NotFlexPWM`.

Only **one pin per FlexPWM submodule** can be used at a time, because each pin's timing runs off its submodule's counter. Pins that share a submodule are grouped below; pick at most one from each row.

| FlexPWM submodule | Teensy 4.0 pins | Teensy 4.1 pins |
|---|---|---|
| FlexPWM1_0 | 1, 36, 37 | 1, 44, 45 |
| FlexPWM1_1 | 0, 34, 35 | 0, 42, 43 |
| FlexPWM1_2 | 24, 38, 39 | 24, 46, 47 |
| FlexPWM1_3 | 7, 8, 25 | 7, 8, 25 |
| FlexPWM2_0 | 4, 33 | 4, 33 |
| FlexPWM2_1 | 5 | 5 |
| FlexPWM2_2 | 6, 9 | 6, 9 |
| FlexPWM2_3 | — | 36, 37 |
| FlexPWM3_0 | — | 54 |
| FlexPWM3_1 | 28, 29 | 28, 29 |
| FlexPWM3_3 | — | 51 |
| FlexPWM4_0 | 22 | 22 |
| FlexPWM4_1 | 23 | 23 |
| FlexPWM4_2 | 2, 3 | 2, 3 |

That gives up to 12 independent DShot outputs on a Teensy 4.0 and 14 on a Teensy 4.1.
A good four-motor set is pins **2, 4, 5, 6** or **3, 4, 5, 6** (used by the example).

Because DShot takes over a pin's FlexPWM submodule, don't use `analogWrite()` on any other pin that shares that submodule.

## Installation

### Arduino IDE (with Teensyduino)

1. Install the Teensy board package (Teensyduino) if you haven't already.
2. Download this repository as a ZIP (**Code → Download ZIP**).
3. In the Arduino IDE choose **Sketch → Include Library → Add .ZIP Library…** and select the ZIP.

Or clone it straight into your libraries folder:

```sh
cd ~/Arduino/libraries
git clone https://github.com/hendy3271/FlexDShot.git
```

### PlatformIO

```ini
[env:teensy40]
platform = teensy
board = teensy40
framework = arduino
lib_deps = https://github.com/hendy3271/FlexDShot.git
```

## Quick start

```cpp
#include "dshot.h"

const uint8_t MOTOR_PIN = 4;

void setup() {
  dshotFrequency(DShot300);              // choose speed BEFORE the first sendDSHOT()

  int err = sendDSHOT(MOTOR_PIN, 0);     // first call sets the pin up
  if (err != DShotError_Success) {
    // handle the error (see the error table below)
  }

  // Arm: most ESCs want a steady stream of zero-throttle frames first
  for (int i = 0; i < 200; i++) {
    sendDSHOT(MOTOR_PIN, 0);
    delay(5);
  }
}

void loop() {
  sendDSHOT(MOTOR_PIN, 100);             // low throttle (48 = minimum, 2047 = maximum)
  delay(2);                              // keep frames flowing; ESCs disarm if they stop
}
```

The full example in [`examples/example_dshot`](examples/example_dshot/example_dshot.ino) drives four motors on pins 3, 4, 5 and 6 at DShot300, arms them, then ramps all four towards a throttle you type into the Serial Monitor.

**Safety:** test with propellers removed.

## API

All functions are declared in `dshot.h`.

### `float dshotFrequency(uint8_t dshot_speed)`

Sets the DShot speed for every pin. Pass `DShot150`, `DShot300`, `DShot600` or `DShot1200`.
Must be called before the first `sendDSHOT()`; once a pin has been set up the speed is locked.

Returns the new bit rate in bits/s, `-1.0` for an unknown speed, or `-2.0` if a pin is already set up.

### `float dshotFrequency()`

Returns the current bit rate in bits/s. Default is DShot150 (150000).

### `int sendDSHOT(uint8_t pin, uint16_t throttle = 0, bool telemetry = false)`

Sends one DShot frame on `pin`. The first call for a pin configures its FlexPWM submodule and a DMA channel. If the previous frame on that pin is still going out, the call waits for it to finish (up to 1 ms) before starting the new one.

- `throttle`: 0–2047, clamped to `DSHOT_THROTTLE_MAX` (2047). By the DShot convention, 0 is disarmed/stop, 1–47 are ESC commands, and 48–2047 is throttle. The library sends the value as-is; it doesn't treat commands specially.
- `telemetry`: sets the telemetry-request bit in the frame. The library doesn't read the reply.

Returns a `DShotError` value:

| Value | Meaning |
|---|---|
| `DShotError_Success` | Frame started |
| `DShotError_NotAPin` | Pin number out of range |
| `DShotError_NotFlexPWM` | Pin isn't a FlexPWM pin (e.g. a QuadTimer pin) |
| `DShotError_SubmoduleUsed` | Another pin already uses this FlexPWM submodule |
| `DShotError_NoSource` | No DMA request source for this submodule |
| `DShotError_NoDMAAvailable` | Couldn't get a DMA channel (nothing is claimed, so you can retry later) |

### Constants

- `DSHOT_THROTTLE_MAX` = 2047
- `DShotType`: `DShot150`, `DShot300`, `DShot600`, `DShot1200`

## How it works

- The FlexPWM submodule period is set to one DShot bit time (`F_BUS / bit rate`; at 150 MHz that's 1000 counts for DShot150 and 125 for DShot1200).
- Each bit is one PWM period: a `1` is high for 75% of the period, a `0` for 37.5%.
- The frame is `throttle(11) | telemetry(1) | crc(4)`, with `crc = (v ^ v>>4 ^ v>>8) & 0xF` over the 12-bit value, sent most significant bit first.
- 20 values are sent per frame: a short low lead-in, the 16 data bits, and low trailing slots. A DMA-completion interrupt stops the submodule after the frame.

## Limitations

- Send-only: no bidirectional DShot, eRPM or telemetry reading.
- One DShot speed for all pins, fixed after the first send. There's no way to release a pin.
- One pin per FlexPWM submodule; QuadTimer pins aren't supported.
- No helper for DShot commands (beeps, spin direction, save settings, …). You can send the raw command value, but you must repeat it yourself and set the telemetry bit where the ESC firmware needs it.
- `sendDSHOT()` blocks briefly if the previous frame on the same pin hasn't finished (a frame takes about 133 µs at DShot150 and 33 µs at DShot600).

## License

MIT. See [LICENSE](LICENSE).
