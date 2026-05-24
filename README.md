<a id="top"></a>

# ESP32 Smart Room

**ESP32 Smart Room** is a standalone IoT room monitoring panel built with a **Freenove ESP32 Wrover**, TFT touchscreen, BME280 environmental sensor, DS3231 RTC module, RGB status LED, buzzer alerts and a local WiFi web dashboard.

The device reads room conditions, displays live data on the TFT screen and exposes a browser-based dashboard directly from the ESP32 access point. It is designed as a compact embedded prototype that combines sensor communication, touch UI, local alerts, persistent settings and a self-hosted web interface.

<p align="center">
  <img src="docs/images/final_build.jpg" alt="ESP32 Smart Room final build" width="700">
</p>

---

## Table of Contents

<details>
  <summary><strong>Show contents</strong></summary>

- [Features](#features)
- [Project Overview](#project-overview)
- [Demo / Photos](#demo--photos)
- [Hardware](#hardware)
- [How It Works](#how-it-works)
- [User Interfaces](#user-interfaces)
  - [TFT Touchscreen](#tft-touchscreen)
  - [Web Dashboard](#web-dashboard)
- [Wiring](#wiring)
  - [I2C Bus](#i2c-bus)
  - [TFT Display and Touch Controller](#tft-display-and-touch-controller--freenove-esp32-wrover)
  - [BME280](#bme280--freenove-esp32-wrover)
  - [DS3231](#ds3231--freenove-esp32-wrover)
  - [RGB LED](#rgb-led-common-anode--freenove-esp32-wrover)
  - [Passive Buzzer](#passive-buzzer--freenove-esp32-wrover)
- [Software](#software)
- [TFT_eSPI Configuration](#tft_espi-configuration)
- [Setup](#setup)
- [Implementation Highlights](#implementation-highlights)
- [Limitations](#limitations)
- [Possible Improvements](#possible-improvements)
- [Authors](#authors)
- [License](#license)

</details>

---

## Features

- Live temperature, humidity and pressure monitoring
- Real-time clock using a DS3231 RTC module
- Local TFT touchscreen interface with multiple screens
- Built-in ESP32 WiFi access point
- Web dashboard hosted directly on the ESP32
- Configurable temperature thresholds
- RGB LED status indicator for temperature ranges
- Passive buzzer alerts for temperature state changes
- Persistent settings stored in ESP32 non-volatile memory
- Min/max temperature tracking since startup
- I2C and SPI communication in one embedded project
- Reproducible `TFT_eSPI` configuration included in the repository

<p align="right">(<a href="#top">back to top</a>)</p>

---

## Project Overview

> [!NOTE]
> This project is a functional embedded prototype focused on local monitoring, direct interaction and standalone operation. It does not require an external server, cloud service or mobile application.

The ESP32 communicates with the BME280 sensor over I2C to read room temperature, humidity and pressure. The DS3231 RTC module provides current time independently from the internet. Data is shown locally on a TFT touchscreen and remotely through a web dashboard served by the ESP32.

The project also includes a simple local alert system. Temperature thresholds can be changed from the TFT interface or the web dashboard. Depending on the current temperature range, the RGB LED changes color and the buzzer can play a short sound alert.

The whole firmware is currently kept in a single Arduino sketch to make flashing and reviewing the prototype straightforward.

<p align="right">(<a href="#top">back to top</a>)</p>

---

## Demo / Photos

Hardware photos, TFT screenshots, web dashboard preview and Fritzing wiring diagram are available below.

<details open>
  <summary><strong>Final build</strong></summary>

![Final build](docs/images/final_build.jpg)

</details>

<details>
  <summary><strong>TFT screens</strong></summary>

The TFT interface contains four screens.

#### MAIN

![TFT MAIN](docs/images/tft_main.jpg)

#### SET

![TFT SET](docs/images/tft_set.jpg)

#### STAT

![TFT STAT](docs/images/tft_stat.jpg)

#### INFO

![TFT INFO](docs/images/tft_info.jpg)

</details>

<details>
  <summary><strong>Web dashboard</strong></summary>

![Web dashboard 1](docs/images/web1.png)

![Web dashboard 2](docs/images/web2.png)

</details>

<details>
  <summary><strong>Wiring diagram</strong></summary>

![Fritzing wiring diagram](docs/images/fritzing_wiring.png)

</details>

<p align="right">(<a href="#top">back to top</a>)</p>

---

## Hardware

> [!IMPORTANT]
> All modules in this setup are powered from **3.3V**. Do not connect the BME280, TFT logic or RTC data lines to 5V logic.

| Component | Purpose |
|---|---|
| Freenove ESP32 Wrover | Main microcontroller |
| TFT display with XPT2046 touch controller | Local graphical user interface |
| BME280 | Temperature, humidity and pressure sensor |
| DS3231 | Real-time clock module |
| RGB LED, common anode | Temperature status indicator |
| Passive buzzer | Sound alerts |
| Resistors | Current limiting for LED and buzzer |
| Jumper wires / breadboard | Prototype wiring |

<p align="right">(<a href="#top">back to top</a>)</p>

---

## How It Works

The ESP32 reads environmental data from the BME280 sensor and time data from the DS3231 RTC module. The current room state is displayed on the TFT touchscreen and can also be checked from the web dashboard.

The RGB LED changes color depending on configurable temperature thresholds:

| Temperature range | RGB color | Meaning |
|---|---|---|
| Below blue threshold | Blue | Cold |
| Between blue and green threshold | Green | Normal |
| Between green and yellow threshold | Yellow | Warm |
| Above yellow threshold | Red | Hot |

When the temperature enters a different range, the RGB LED state is updated. If the buzzer is enabled, the device also plays a short alert sound.

Settings such as temperature thresholds and buzzer state are stored using ESP32 `Preferences`, so they are preserved after restart or power loss.

<p align="right">(<a href="#top">back to top</a>)</p>

---

## User Interfaces

> [!TIP]
> The project provides both a local TFT interface and a browser-based dashboard, so the device can be used without any external server.

### TFT Touchscreen

The TFT interface contains four screens:

| Screen | Description |
|---|---|
| `MAIN` | Temperature, humidity, pressure, current time and quick buzzer mute |
| `SET` | Temperature thresholds and buzzer settings |
| `STAT` | WiFi status, sensor status, current temperature state and min/max temperature |
| `INFO` | Project and author information |

Touch input is handled through the XPT2046 touch controller.

### Web Dashboard

The ESP32 creates its own WiFi access point and serves a local web dashboard.

| Parameter | Value |
|---|---|
| SSID | `SmartRoom-ESP32` |
| Password | `12345678` |
| Dashboard URL | `http://192.168.4.1` |

> [!WARNING]
> The ESP32 runs in access point mode, so the dashboard is available only after connecting directly to the device WiFi network.

The web dashboard allows you to:

- view live BME280 sensor readings,
- check the current RTC time,
- view module status,
- inspect current thresholds,
- change temperature thresholds,
- enable or disable the buzzer.

The dashboard separates live values from editable form fields, so automatic refresh does not interrupt changing settings.

<p align="right">(<a href="#top">back to top</a>)</p>

---

## Wiring

### I2C Bus

The BME280 and DS3231 modules share the same I2C bus.

| Signal | ESP32 GPIO |
|---|---|
| SDA | GPIO21 |
| SCL | GPIO22 |

> [!NOTE]
> BME280 modules may use either `0x76` or `0x77`, depending on the board variant.

Typical I2C addresses:

| Module | Address |
|---|---|
| BME280 | `0x76` or `0x77` |
| DS3231 | `0x68` |

<p align="right">(<a href="#top">back to top</a>)</p>

---

### TFT Display and Touch Controller → Freenove ESP32 Wrover

| TFT / Touch pin | ESP32 pin | Notes |
|---|---|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| LED | 3.3V | Backlight |
| CS | GPIO5 | TFT chip select |
| RESET | GPIO33 | TFT reset |
| DC | GPIO32 | TFT data/command |
| SDI / MOSI | GPIO23 | SPI MOSI |
| SCK | GPIO18 | SPI clock |
| SDO / MISO | GPIO19 | SPI MISO |
| T_CLK | GPIO18 | Shared with SCK |
| T_CS | GPIO14 | Touch chip select |
| T_DIN | GPIO23 | Shared with MOSI |
| T_DO | GPIO19 | Shared with MISO |
| T_IRQ | GPIO34 | Touch interrupt |

TFT pins are configured in the `TFT_eSPI` library inside `User_Setup.h`.

---

### BME280 → Freenove ESP32 Wrover

| BME280 pin | ESP32 pin | Notes |
|---|---|---|
| VIN / VCC | 3.3V | Power |
| GND | GND | Ground |
| SDA | GPIO21 | I2C SDA |
| SCL | GPIO22 | I2C SCL |
| CSB | Not connected | I2C mode |
| SDO | Not connected | Default module address |

---

### DS3231 → Freenove ESP32 Wrover

| DS3231 pin | ESP32 pin | Notes |
|---|---|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| SDA | GPIO21 | Shared I2C SDA |
| SCL | GPIO22 | Shared I2C SCL |

---

### RGB LED, Common Anode → Freenove ESP32 Wrover

| RGB LED pin | ESP32 pin | Notes |
|---|---|---|
| Common anode | 3.3V | Shared LED anode |
| R | GPIO27 | Through resistor |
| G | GPIO26 | Through resistor |
| B | GPIO25 | Through resistor |

The RGB LED is a **common anode** LED, so the project uses inverted control logic.

---

### Passive Buzzer → Freenove ESP32 Wrover

| Buzzer pin | ESP32 pin | Notes |
|---|---|---|
| `+` | GPIO13 | Through 220Ω resistor |
| `-` | GND | Ground |

The passive buzzer is controlled with PWM, which allows it to play simple tones and short sound signals.

<p align="right">(<a href="#top">back to top</a>)</p>

---

## Software

The project targets the Arduino ecosystem on ESP32 and uses the following libraries:

- `SPI.h`
- `Wire.h`
- `WiFi.h`
- `WebServer.h`
- `Preferences.h`
- `TFT_eSPI.h`
- `XPT2046_Touchscreen.h`
- `Adafruit_BME280.h`
- `RTClib.h`

Additional dependencies may be required for the BME280 sensor:

- `Adafruit Unified Sensor`
- `Adafruit BusIO`

<p align="right">(<a href="#top">back to top</a>)</p>

---

## TFT_eSPI Configuration

The TFT display uses the `TFT_eSPI` library with a custom configuration prepared for:

```text
ESP32 WROVER + ILI9341 SPI TFT + XPT2046 touch controller
```

This repository includes the exact configuration used in this project:

```text
docs/tft/User_Setup.h
```

To use the same display setup, copy this file into your local `TFT_eSPI` library directory and replace the default `User_Setup.h`.

> [!WARNING]
> Back up your existing `TFT_eSPI/User_Setup.h` before replacing it, especially if you use the same Arduino installation for other TFT projects.

### Display driver

The project uses the ILI9341 driver variant:

```cpp
#define ILI9341_2_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 320
```

The screen is used in landscape mode by the application code:

```cpp
#define DISPLAY_ROTATION 1
```

### Color configuration

The display uses BGR color order and inverted colors:

```cpp
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_ON
```

### TFT SPI pins

```cpp
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18

#define TFT_CS   5
#define TFT_DC   32
#define TFT_RST  33
```

### Touch controller

The XPT2046 touch controller uses a separate chip select pin:

```cpp
#define TOUCH_CS 14
```

The touch interrupt pin is defined in the project code:

```cpp
#define TOUCH_IRQ 34
```

### Loaded fonts

The configuration enables the standard TFT_eSPI fonts and smooth fonts:

```cpp
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT
```

### SPI frequencies

```cpp
#define SPI_FREQUENCY       10000000
#define SPI_READ_FREQUENCY  10000000
#define SPI_TOUCH_FREQUENCY 2500000
```

<p align="right">(<a href="#top">back to top</a>)</p>

---

## Setup

### 1. Clone the repository

```bash
git clone https://github.com/PoProstuWitold/esp32-smart-room.git
cd esp32-smart-room
```

### 2. Install required libraries

Install the required Arduino libraries manually in the Arduino IDE.

> [!TIP]
> If the BME280 library does not compile, also install `Adafruit Unified Sensor` and `Adafruit BusIO`.

### 3. Configure `TFT_eSPI`

Copy the provided TFT configuration file:

```text
docs/tft/User_Setup.h
```

into your local `TFT_eSPI` library directory and replace the default `User_Setup.h`.

On Linux with Arduino IDE, the path is usually:

```text
~/Arduino/libraries/TFT_eSPI/User_Setup.h
```

This project uses an `ILI9341_2_DRIVER` setup with BGR color order, color inversion enabled, SPI at 10 MHz and touch SPI at 2.5 MHz.

### 4. Upload the code

Connect the Freenove ESP32 Wrover board and upload the project.

### 5. Connect to the ESP32 WiFi

After startup, connect to:

```text
SSID: SmartRoom-ESP32
Password: 12345678
```

Then open:

```text
http://192.168.4.1
```

> [!CAUTION]
> The default access point password is intended only for a local prototype/demo setup. Change it before using the project outside a controlled environment.

<p align="right">(<a href="#top">back to top</a>)</p>

---

## Implementation Highlights

### Non-blocking buzzer playback

The buzzer logic uses `millis()` instead of long blocking `delay()` calls. This keeps the system responsive while sounds are playing.

During buzzer playback, the ESP32 can still:

- handle web server requests,
- process touch input,
- update the TFT screen,
- read sensor data,
- update RTC time.

### Shared settings between interfaces

Temperature thresholds and buzzer state are shared between the TFT interface and the web dashboard. Changing settings in one interface immediately affects the whole device.

### Persistent configuration

The project uses ESP32 `Preferences` to store:

- blue temperature threshold,
- green temperature threshold,
- yellow temperature threshold,
- buzzer enabled/disabled state.

This makes the configuration survive resets and power loss.

### Standalone operation

The ESP32 serves its own dashboard and creates its own WiFi access point. This makes the prototype independent from external servers, routers or cloud platforms.

<p align="right">(<a href="#top">back to top</a>)</p>

---

## Limitations

This project is a working prototype, not a production-ready smart home device.

Known limitations:

- the dashboard works in ESP32 access point mode only,
- there is no user authentication,
- there is no TLS/HTTPS support,
- sensor history is not stored long-term,
- the current build does not include OTA updates,
- the firmware is kept in one Arduino sketch,
- the hardware is still a breadboard/prototype setup.

<p align="right">(<a href="#top">back to top</a>)</p>

---

## Possible Improvements

Possible next steps for this project:

- split the firmware into smaller modules,
- add PlatformIO configuration for reproducible builds,
- add long-term sensor history and charts,
- add WiFi station mode in addition to access point mode,
- add simple dashboard authentication,
- add OTA firmware updates,
- design a PCB or enclosure,
- integrate the device with Home Assistant or MQTT.

<p align="right">(<a href="#top">back to top</a>)</p>

---

## Authors

- **Witold Zawada** ([GitHub](https://github.com/PoProstuWitold))
- **Wiktor Wypyszyński** ([GitHub](https://github.com/Netr0n07))

<p align="right">(<a href="#top">back to top</a>)</p>

---

## License

This project is released under the MIT License.

<p align="right">(<a href="#top">back to top</a>)</p>
