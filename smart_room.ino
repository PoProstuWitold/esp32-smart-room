#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Adafruit_BME280.h>
#include <RTClib.h>

// ======================================================
// display module
// ======================================================

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define DISPLAY_ROTATION 1

#define DISPLAY_BACKGROUND TFT_BLACK
#define TITLE_COLOR TFT_CYAN
#define TEXT_COLOR TFT_WHITE
#define VALUE_COLOR TFT_GREEN
#define BUTTON_COLOR TFT_DARKGREY
#define BUTTON_ACTIVE_COLOR TFT_NAVY

TFT_eSPI tft = TFT_eSPI();

// ======================================================
// touch module
// ======================================================

#define TOUCH_CS 14
#define TOUCH_IRQ 34
#define TOUCH_ROTATION 1

#define TOUCH_MIN_X 350
#define TOUCH_MAX_X 3800
#define TOUCH_MIN_Y 350
#define TOUCH_MAX_Y 3850

#define TOUCH_DEBOUNCE_DELAY 250

XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
unsigned long lastTouchTime = 0;

// ======================================================
// WiFi/web module
// ======================================================

#define WIFI_AP_SSID "SmartRoom-ESP32"
#define WIFI_AP_PASSWORD "12345678"

WebServer server(80);
Preferences preferences;

#define SETTINGS_NAMESPACE "smart-room"

// ======================================================
// BME280 module
// ======================================================

#define BME280_SDA 21
#define BME280_SCL 22

#define BME280_ADDRESS 0x76
#define BME280_ADDRESS_ALT 0x77

#define BME280_UPDATE_INTERVAL 2000

Adafruit_BME280 bme;

bool bmeReady = false;
unsigned long lastBmeUpdate = 0;

float temperature = 0.0;
float humidity = 0.0;
float pressure = 0.0;

bool temperatureStatsReady = false;
float minTemperature = 0.0;
float maxTemperature = 0.0;

// ======================================================
// DS3231 clock module
// ======================================================

#define CLOCK_UPDATE_INTERVAL 1000

RTC_DS3231 rtc;

bool rtcReady = false;
unsigned long lastClockUpdate = 0;
String currentTimeText = "--:--:--";

// ======================================================
// RGB LED module
// ======================================================

#define RGB_RED_PIN 27
#define RGB_GREEN_PIN 26
#define RGB_BLUE_PIN 25

#define RGB_COMMON_ANODE true

float tempBlueMax = 25.0;
float tempGreenMax = 30.0;
float tempYellowMax = 35.0;

// ======================================================
// temperature state module
// ======================================================

enum TemperatureState {
    TEMP_STATE_UNKNOWN,
    TEMP_STATE_BLUE,
    TEMP_STATE_GREEN,
    TEMP_STATE_YELLOW,
    TEMP_STATE_RED
};

TemperatureState lastTemperatureState = TEMP_STATE_UNKNOWN;

// ======================================================
// buzzer module
// ======================================================

#define BUZZER_PIN 13
#define BUZZER_CHANNEL 3
#define BUZZER_PWM_FREQUENCY 2000
#define BUZZER_PWM_RESOLUTION 8
#define NOTE_GAP 35

#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_REST 0

bool buzzerEnabled = true;

struct MelodyNote {
    int frequency;
    int duration;
};

MelodyNote blueMelody[] = {
    { NOTE_G5, 140 },
    { NOTE_E5, 140 },
    { NOTE_D5, 140 },
    { NOTE_C5, 220 },
    { NOTE_REST, 80 },
    { NOTE_E5, 140 },
    { NOTE_D5, 140 },
    { NOTE_C5, 260 }
};

MelodyNote greenMelody[] = {
    { NOTE_C5, 120 },
    { NOTE_E5, 120 },
    { NOTE_G5, 120 },
    { NOTE_C6, 180 },
    { NOTE_REST, 70 },
    { NOTE_G5, 120 },
    { NOTE_C6, 240 }
};

MelodyNote yellowMelody[] = {
    { NOTE_A4, 120 },
    { NOTE_REST, 40 },
    { NOTE_A4, 120 },
    { NOTE_REST, 40 },
    { NOTE_D5, 160 },
    { NOTE_C5, 160 },
    { NOTE_A4, 220 }
};

MelodyNote redMelody[] = {
    { NOTE_C5, 100 },
    { NOTE_E5, 100 },
    { NOTE_G5, 100 },
    { NOTE_C6, 180 },
    { NOTE_REST, 60 },
    { NOTE_C5, 100 },
    { NOTE_E5, 100 },
    { NOTE_G5, 100 },
    { NOTE_C6, 260 }
};

const MelodyNote* activeMelody = nullptr;
int activeMelodyLength = 0;
int activeMelodyIndex = 0;
unsigned long melodyStepStartedAt = 0;
bool melodyPlaying = false;
bool melodyGapPhase = false;

// ======================================================
// app screen module
// ======================================================

enum AppScreen {
    SCREEN_MAIN,
    SCREEN_SETTINGS,
    SCREEN_STATUS,
    SCREEN_ABOUT
};

AppScreen currentScreen = SCREEN_MAIN;

// ======================================================
// common structures
// ======================================================

struct ScreenPoint {
    int x;
    int y;
};

// ======================================================
// function declarations
// ======================================================

void drawCurrentScreen();
void updateTemperatureIndicators();
TemperatureState getTemperatureState();
void keepThresholdsValid();
void saveSettings();
void loadSettings();
void updateBuzzer();
void drawMainMuteButton();

// ======================================================
// helper functions
// ======================================================

String formatTwoDigits(int value) {
    if (value < 10) {
        return "0" + String(value);
    }

    return String(value);
}

String getTemperatureStateName(TemperatureState state) {
    if (state == TEMP_STATE_BLUE) {
        return "BLUE / cold";
    }

    if (state == TEMP_STATE_GREEN) {
        return "GREEN / normal";
    }

    if (state == TEMP_STATE_YELLOW) {
        return "YELLOW / warm";
    }

    if (state == TEMP_STATE_RED) {
        return "RED / hot";
    }

    return "UNKNOWN";
}

// ======================================================
// display functions
// ======================================================

void initDisplay() {
    tft.init();
    tft.setRotation(DISPLAY_ROTATION);
    tft.fillScreen(DISPLAY_BACKGROUND);
}

void drawHeader(const String& title) {
    tft.fillRect(0, 0, SCREEN_WIDTH, 38, TFT_NAVY);
    tft.setTextColor(TITLE_COLOR, TFT_NAVY);
    tft.drawCentreString(title, SCREEN_WIDTH / 2, 7, 4);
}

void drawMainMuteButton() {
    uint16_t buttonColor = buzzerEnabled ? TFT_MAROON : TFT_DARKGREEN;

    tft.fillRect(258, 8, 54, 22, buttonColor);
    tft.drawRect(258, 8, 54, 22, TFT_WHITE);

    tft.setTextColor(TFT_WHITE, buttonColor);
    tft.drawCentreString(buzzerEnabled ? "BZ ON" : "BZ OFF", 285, 13, 1);
}

void drawBottomNav() {
    int buttonWidth = SCREEN_WIDTH / 4;
    int y = 205;

    String labels[] = { "MAIN", "SET", "STAT", "INFO" };
    AppScreen screens[] = { SCREEN_MAIN, SCREEN_SETTINGS, SCREEN_STATUS, SCREEN_ABOUT };

    for (int i = 0; i < 4; i++) {
        uint16_t color = currentScreen == screens[i] ? BUTTON_ACTIVE_COLOR : BUTTON_COLOR;

        tft.fillRect(i * buttonWidth, y, buttonWidth, 35, color);
        tft.drawRect(i * buttonWidth, y, buttonWidth, 35, TFT_WHITE);

        tft.setTextColor(TFT_WHITE, color);
        tft.drawCentreString(labels[i], i * buttonWidth + buttonWidth / 2, y + 10, 2);
    }
}

void drawMainValues() {
    tft.fillRect(150, 55, 160, 140, DISPLAY_BACKGROUND);

    if (!bmeReady) {
        tft.setTextColor(TFT_RED, DISPLAY_BACKGROUND);
        tft.drawString("BME280 error", 145, 70, 2);
        return;
    }

    tft.setTextColor(VALUE_COLOR, DISPLAY_BACKGROUND);

    tft.drawString(String(temperature, 1) + " C", 165, 60, 2);
    tft.drawString(String(humidity, 1) + " %", 165, 95, 2);
    tft.drawString(String(pressure, 1) + " hPa", 165, 130, 2);
    tft.drawString(currentTimeText, 165, 165, 2);
}

void drawMainScreen() {
    tft.fillScreen(DISPLAY_BACKGROUND);
    drawHeader("SMART ROOM");
    drawMainMuteButton();

    tft.setTextColor(TEXT_COLOR, DISPLAY_BACKGROUND);
    tft.drawString("Temperature:", 25, 60, 2);
    tft.drawString("Humidity:", 25, 95, 2);
    tft.drawString("Pressure:", 25, 130, 2);
    tft.drawString("Time:", 25, 165, 2);

    drawMainValues();
    drawBottomNav();
}

void drawSettingsScreen() {
    tft.fillScreen(DISPLAY_BACKGROUND);
    drawHeader("SETTINGS");

    tft.setTextColor(TEXT_COLOR, DISPLAY_BACKGROUND);

    tft.drawString("Blue max:", 20, 50, 2);
    tft.drawString(String(tempBlueMax, 1) + " C", 130, 50, 2);

    tft.drawString("Green max:", 20, 85, 2);
    tft.drawString(String(tempGreenMax, 1) + " C", 130, 85, 2);

    tft.drawString("Yellow max:", 20, 120, 2);
    tft.drawString(String(tempYellowMax, 1) + " C", 130, 120, 2);

    tft.drawString("Buzzer:", 20, 155, 2);
    tft.drawString(buzzerEnabled ? "ON" : "OFF", 130, 155, 2);

    tft.fillRect(225, 48, 35, 25, TFT_DARKGREEN);
    tft.fillRect(270, 48, 35, 25, TFT_MAROON);
    tft.fillRect(225, 83, 35, 25, TFT_DARKGREEN);
    tft.fillRect(270, 83, 35, 25, TFT_MAROON);
    tft.fillRect(225, 118, 35, 25, TFT_DARKGREEN);
    tft.fillRect(270, 118, 35, 25, TFT_MAROON);
    tft.fillRect(220, 153, 85, 25, TFT_DARKGREY);

    tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    tft.drawCentreString("+", 242, 53, 2);
    tft.drawCentreString("+", 242, 88, 2);
    tft.drawCentreString("+", 242, 123, 2);

    tft.setTextColor(TFT_WHITE, TFT_MAROON);
    tft.drawCentreString("-", 287, 53, 2);
    tft.drawCentreString("-", 287, 88, 2);
    tft.drawCentreString("-", 287, 123, 2);

    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.drawCentreString("TOGGLE", 262, 158, 2);

    drawBottomNav();
}

void drawStatusScreen() {
    tft.fillScreen(DISPLAY_BACKGROUND);
    drawHeader("STATUS");

    tft.setTextColor(TEXT_COLOR, DISPLAY_BACKGROUND);

    tft.drawString("WiFi AP:", 20, 50, 2);
    tft.drawString(WIFI_AP_SSID, 120, 50, 2);

    tft.drawString("IP:", 20, 75, 2);
    tft.drawString(WiFi.softAPIP().toString(), 120, 75, 2);

    tft.drawString("BME280:", 20, 100, 2);
    tft.drawString(bmeReady ? "OK" : "ERROR", 120, 100, 2);

    tft.drawString("DS3231:", 20, 125, 2);
    tft.drawString(rtcReady ? "OK" : "ERROR", 120, 125, 2);

    tft.drawString("State:", 20, 150, 2);
    tft.drawString(getTemperatureStateName(getTemperatureState()), 120, 150, 2);

    tft.drawString("Min:", 20, 180, 2);
    tft.drawString(temperatureStatsReady ? String(minTemperature, 1) + " C" : "--", 65, 180, 2);

    tft.drawString("Max:", 165, 180, 2);
    tft.drawString(temperatureStatsReady ? String(maxTemperature, 1) + " C" : "--", 210, 180, 2);

    drawBottomNav();
}

void drawAboutScreen() {
    tft.fillScreen(DISPLAY_BACKGROUND);
    drawHeader("ABOUT");

    tft.setTextColor(TEXT_COLOR, DISPLAY_BACKGROUND);
    tft.drawString("Smart Room Monitor", 35, 55, 2);
    tft.drawString("ESP32 + TFT + BME280", 35, 80, 2);
    tft.drawString("DS3231 + RGB + buzzer", 35, 105, 2);
    tft.drawString("Web panel: 192.168.4.1", 35, 130, 2);
    tft.drawString("Wiktor Wypyszynski", 35, 160, 2);
    tft.drawString("Witold Zawada", 35, 182, 2);

    drawBottomNav();
}

void drawCurrentScreen() {
    if (currentScreen == SCREEN_MAIN) {
        drawMainScreen();
        return;
    }

    if (currentScreen == SCREEN_SETTINGS) {
        drawSettingsScreen();
        return;
    }

    if (currentScreen == SCREEN_STATUS) {
        drawStatusScreen();
        return;
    }

    if (currentScreen == SCREEN_ABOUT) {
        drawAboutScreen();
        return;
    }
}

// ======================================================
// touch functions
// ======================================================

void initTouch() {
    ts.begin();
    ts.setRotation(TOUCH_ROTATION);
}

ScreenPoint getTouchPoint() {
    TS_Point rawPoint = ts.getPoint();

    ScreenPoint point;

    point.x = map(rawPoint.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, SCREEN_WIDTH - 1);
    point.y = map(rawPoint.x, TOUCH_MAX_X, TOUCH_MIN_X, 0, SCREEN_HEIGHT - 1);

    point.x = constrain(point.x, 0, SCREEN_WIDTH - 1);
    point.y = constrain(point.y, 0, SCREEN_HEIGHT - 1);

    return point;
}

bool isInside(ScreenPoint point, int x, int y, int width, int height) {
    return point.x >= x &&
           point.x <= x + width &&
           point.y >= y &&
           point.y <= y + height;
}

void handleMainScreenTouch(ScreenPoint point) {
    if (currentScreen != SCREEN_MAIN) {
        return;
    }

    if (!isInside(point, 258, 8, 54, 22)) {
        return;
    }

    buzzerEnabled = !buzzerEnabled;
    saveSettings();
    drawMainScreen();
}

void handleBottomNavTouch(ScreenPoint point) {
    if (point.y < 205) {
        return;
    }

    int section = point.x / (SCREEN_WIDTH / 4);

    if (section == 0) {
        currentScreen = SCREEN_MAIN;
    } else if (section == 1) {
        currentScreen = SCREEN_SETTINGS;
    } else if (section == 2) {
        currentScreen = SCREEN_STATUS;
    } else if (section == 3) {
        currentScreen = SCREEN_ABOUT;
    }

    drawCurrentScreen();
}

void keepThresholdsValid() {
    if (tempBlueMax < 5.0) {
        tempBlueMax = 5.0;
    }

    if (tempGreenMax <= tempBlueMax) {
        tempGreenMax = tempBlueMax + 1.0;
    }

    if (tempYellowMax <= tempGreenMax) {
        tempYellowMax = tempGreenMax + 1.0;
    }
}

void handleSettingsTouch(ScreenPoint point) {
    if (currentScreen != SCREEN_SETTINGS) {
        return;
    }

    if (isInside(point, 225, 48, 35, 25)) {
        tempBlueMax += 0.5;
    } else if (isInside(point, 270, 48, 35, 25)) {
        tempBlueMax -= 0.5;
    } else if (isInside(point, 225, 83, 35, 25)) {
        tempGreenMax += 0.5;
    } else if (isInside(point, 270, 83, 35, 25)) {
        tempGreenMax -= 0.5;
    } else if (isInside(point, 225, 118, 35, 25)) {
        tempYellowMax += 0.5;
    } else if (isInside(point, 270, 118, 35, 25)) {
        tempYellowMax -= 0.5;
    } else if (isInside(point, 220, 153, 85, 25)) {
        buzzerEnabled = !buzzerEnabled;
    } else {
        return;
    }

    keepThresholdsValid();
    updateTemperatureIndicators();
    drawSettingsScreen();
    saveSettings();
}

void updateTouch() {
    if (!ts.touched()) {
        return;
    }

    unsigned long now = millis();

    if (now - lastTouchTime < TOUCH_DEBOUNCE_DELAY) {
        return;
    }

    lastTouchTime = now;

    ScreenPoint point = getTouchPoint();

    handleMainScreenTouch(point);
    handleBottomNavTouch(point);
    handleSettingsTouch(point);
}

// ======================================================
// BME280 functions
// ======================================================

void initBme280() {
    Wire.begin(BME280_SDA, BME280_SCL);

    if (bme.begin(BME280_ADDRESS)) {
        bmeReady = true;
        Serial.println("BME280 found at 0x76");
        return;
    }

    if (bme.begin(BME280_ADDRESS_ALT)) {
        bmeReady = true;
        Serial.println("BME280 found at 0x77");
        return;
    }

    bmeReady = false;
    Serial.println("BME280 not found");
}

void readBme280() {
    if (!bmeReady) {
        return;
    }

    temperature = bme.readTemperature();
    humidity = bme.readHumidity();
    pressure = bme.readPressure() / 100.0;

    if (!temperatureStatsReady) {
        minTemperature = temperature;
        maxTemperature = temperature;
        temperatureStatsReady = true;
        return;
    }

    if (temperature < minTemperature) {
        minTemperature = temperature;
    }

    if (temperature > maxTemperature) {
        maxTemperature = temperature;
    }
}

void updateBme280() {
    unsigned long now = millis();

    if (now - lastBmeUpdate < BME280_UPDATE_INTERVAL) {
        return;
    }

    lastBmeUpdate = now;

    readBme280();

    if (currentScreen == SCREEN_MAIN) {
        drawMainValues();
    }

    if (currentScreen == SCREEN_STATUS) {
        drawStatusScreen();
    }

    updateTemperatureIndicators();

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" C | Humidity: ");
    Serial.print(humidity);
    Serial.print(" % | Pressure: ");
    Serial.print(pressure);
    Serial.println(" hPa");
}

// ======================================================
// DS3231 clock functions
// ======================================================

String getFormattedTime(DateTime now) {
    return formatTwoDigits(now.hour()) + ":" +
           formatTwoDigits(now.minute()) + ":" +
           formatTwoDigits(now.second());
}

void initClock() {
    if (!rtc.begin()) {
        rtcReady = false;
        Serial.println("DS3231 not found");
        return;
    }

    rtcReady = true;
    Serial.println("DS3231 found");

    if (rtc.lostPower()) {
        Serial.println("DS3231 lost power, setting time from compile time");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
}

void readClock() {
    if (!rtcReady) {
        currentTimeText = "--:--:--";
        return;
    }

    DateTime now = rtc.now();
    currentTimeText = getFormattedTime(now);
}

void updateClock() {
    unsigned long nowMillis = millis();

    if (nowMillis - lastClockUpdate < CLOCK_UPDATE_INTERVAL) {
        return;
    }

    lastClockUpdate = nowMillis;

    readClock();

    if (currentScreen == SCREEN_MAIN) {
        drawMainValues();
    }

    Serial.print("Time: ");
    Serial.println(currentTimeText);
}

// ======================================================
// RGB LED functions
// ======================================================

void writeRgbChannel(int pin, bool state) {
    if (RGB_COMMON_ANODE) {
        digitalWrite(pin, state ? LOW : HIGH);
        return;
    }

    digitalWrite(pin, state ? HIGH : LOW);
}

void setRgbLed(bool red, bool green, bool blue) {
    writeRgbChannel(RGB_RED_PIN, red);
    writeRgbChannel(RGB_GREEN_PIN, green);
    writeRgbChannel(RGB_BLUE_PIN, blue);
}

void initRgbLed() {
    pinMode(RGB_RED_PIN, OUTPUT);
    pinMode(RGB_GREEN_PIN, OUTPUT);
    pinMode(RGB_BLUE_PIN, OUTPUT);

    setRgbLed(false, false, false);
}

void updateRgbLedByTemperature() {
    if (!bmeReady) {
        setRgbLed(false, false, false);
        return;
    }

    if (temperature > tempYellowMax) {
        setRgbLed(true, false, false);
        return;
    }

    if (temperature >= tempGreenMax) {
        setRgbLed(true, true, false);
        return;
    }

    if (temperature >= tempBlueMax) {
        setRgbLed(false, true, false);
        return;
    }

    setRgbLed(false, false, true);
}

// ======================================================
// buzzer functions
// ======================================================

void initBuzzer() {
    ledcAttachChannel(
        BUZZER_PIN,
        BUZZER_PWM_FREQUENCY,
        BUZZER_PWM_RESOLUTION,
        BUZZER_CHANNEL
    );

    ledcWriteTone(BUZZER_PIN, 0);
}

void stopMelody() {
    ledcWriteTone(BUZZER_PIN, 0);

    activeMelody = nullptr;
    activeMelodyLength = 0;
    activeMelodyIndex = 0;
    melodyStepStartedAt = 0;
    melodyPlaying = false;
    melodyGapPhase = false;
}

void startCurrentNote() {
    if (activeMelody == nullptr || activeMelodyIndex >= activeMelodyLength) {
        stopMelody();
        return;
    }

    int frequency = activeMelody[activeMelodyIndex].frequency;

    if (frequency == NOTE_REST) {
        ledcWriteTone(BUZZER_PIN, 0);
    } else {
        ledcWriteTone(BUZZER_PIN, frequency);
    }

    melodyStepStartedAt = millis();
    melodyGapPhase = false;
}

void startMelody(const MelodyNote melody[], int melodyLength) {
    if (!buzzerEnabled) {
        stopMelody();
        return;
    }

    activeMelody = melody;
    activeMelodyLength = melodyLength;
    activeMelodyIndex = 0;
    melodyPlaying = true;

    startCurrentNote();
}

void updateBuzzer() {
    if (!melodyPlaying) {
        return;
    }

    if (!buzzerEnabled) {
        stopMelody();
        return;
    }

    unsigned long now = millis();

    if (activeMelody == nullptr || activeMelodyIndex >= activeMelodyLength) {
        stopMelody();
        return;
    }

    if (!melodyGapPhase) {
        int duration = activeMelody[activeMelodyIndex].duration;

        if (now - melodyStepStartedAt >= duration) {
            ledcWriteTone(BUZZER_PIN, 0);
            melodyStepStartedAt = now;
            melodyGapPhase = true;
        }

        return;
    }

    if (now - melodyStepStartedAt >= NOTE_GAP) {
        activeMelodyIndex++;
        startCurrentNote();
    }
}

void playMelodyForTemperatureState(TemperatureState state) {
    if (state == TEMP_STATE_BLUE) {
        startMelody(blueMelody, sizeof(blueMelody) / sizeof(blueMelody[0]));
        return;
    }

    if (state == TEMP_STATE_GREEN) {
        startMelody(greenMelody, sizeof(greenMelody) / sizeof(greenMelody[0]));
        return;
    }

    if (state == TEMP_STATE_YELLOW) {
        startMelody(yellowMelody, sizeof(yellowMelody) / sizeof(yellowMelody[0]));
        return;
    }

    if (state == TEMP_STATE_RED) {
        startMelody(redMelody, sizeof(redMelody) / sizeof(redMelody[0]));
        return;
    }
}

// ======================================================
// temperature indicator functions
// ======================================================

TemperatureState getTemperatureState() {
    if (!bmeReady) {
        return TEMP_STATE_UNKNOWN;
    }

    if (temperature > tempYellowMax) {
        return TEMP_STATE_RED;
    }

    if (temperature >= tempGreenMax) {
        return TEMP_STATE_YELLOW;
    }

    if (temperature >= tempBlueMax) {
        return TEMP_STATE_GREEN;
    }

    return TEMP_STATE_BLUE;
}

void updateBuzzerByTemperatureState(TemperatureState currentState) {
    if (currentState == TEMP_STATE_UNKNOWN) {
        return;
    }

    if (currentState == lastTemperatureState) {
        return;
    }

    playMelodyForTemperatureState(currentState);
    lastTemperatureState = currentState;
}

void updateTemperatureIndicators() {
    TemperatureState currentState = getTemperatureState();

    updateRgbLedByTemperature();
    updateBuzzerByTemperatureState(currentState);
}

// ======================================================
// settings storage functions
// ======================================================

void saveSettings() {
    preferences.begin(SETTINGS_NAMESPACE, false);

    preferences.putFloat("blueMax", tempBlueMax);
    preferences.putFloat("greenMax", tempGreenMax);
    preferences.putFloat("yellowMax", tempYellowMax);
    preferences.putBool("buzzer", buzzerEnabled);

    preferences.end();

    Serial.println("Settings saved");
}

void loadSettings() {
    preferences.begin(SETTINGS_NAMESPACE, true);

    tempBlueMax = preferences.getFloat("blueMax", tempBlueMax);
    tempGreenMax = preferences.getFloat("greenMax", tempGreenMax);
    tempYellowMax = preferences.getFloat("yellowMax", tempYellowMax);
    buzzerEnabled = preferences.getBool("buzzer", buzzerEnabled);

    preferences.end();

    keepThresholdsValid();

    Serial.println("Settings loaded");
}

// ======================================================
// web functions
// ======================================================

String buildStatusJson() {
    String json = "{";

    json += "\"temperature\":" + String(temperature, 1) + ",";
    json += "\"humidity\":" + String(humidity, 1) + ",";
    json += "\"pressure\":" + String(pressure, 1) + ",";
    json += "\"minTemperature\":" + String(minTemperature, 1) + ",";
    json += "\"maxTemperature\":" + String(maxTemperature, 1) + ",";
    json += "\"temperatureStatsReady\":" + String(temperatureStatsReady ? "true" : "false") + ",";
    json += "\"time\":\"" + currentTimeText + "\",";
    json += "\"bmeReady\":" + String(bmeReady ? "true" : "false") + ",";
    json += "\"rtcReady\":" + String(rtcReady ? "true" : "false") + ",";
    json += "\"buzzerEnabled\":" + String(buzzerEnabled ? "true" : "false") + ",";
    json += "\"blueMax\":" + String(tempBlueMax, 1) + ",";
    json += "\"greenMax\":" + String(tempGreenMax, 1) + ",";
    json += "\"yellowMax\":" + String(tempYellowMax, 1) + ",";
    json += "\"state\":\"" + getTemperatureStateName(getTemperatureState()) + "\"";

    json += "}";

    return json;
}

String buildHtmlPage() {
    String page = R"rawliteral(
<!DOCTYPE html>
<html lang="pl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Room</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background: #10131a;
            color: #f5f5f5;
            margin: 0;
            padding: 20px;
        }

        .container {
            max-width: 760px;
            margin: 0 auto;
        }

        .card {
            background: #1b2030;
            border-radius: 16px;
            padding: 18px;
            margin-bottom: 16px;
            box-shadow: 0 8px 24px rgba(0,0,0,0.25);
        }

        h1 {
            margin-top: 0;
            color: #66e3ff;
        }

        h2 {
            margin-top: 0;
        }

        .authors {
            opacity: 0.85;
            line-height: 1.5;
        }

        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
            gap: 12px;
        }

        .metric {
            background: #252c3f;
            border-radius: 12px;
            padding: 14px;
        }

        .label {
            opacity: 0.75;
            font-size: 14px;
        }

        .value {
            font-size: 26px;
            font-weight: bold;
            margin-top: 6px;
        }

        .settings-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
            gap: 12px;
        }

        .setting-box {
            background: #252c3f;
            border-radius: 12px;
            padding: 14px;
        }

        input[type="number"] {
            width: 100px;
            padding: 8px;
            border-radius: 8px;
            border: 0;
            margin-left: 8px;
        }

        button {
            padding: 10px 14px;
            border: 0;
            border-radius: 10px;
            background: #66e3ff;
            color: #10131a;
            font-weight: bold;
            cursor: pointer;
            margin-top: 10px;
        }

        .row {
            margin: 12px 0;
        }

        .hint {
            opacity: 0.7;
            font-size: 14px;
        }

        .state-badge {
            display: inline-block;
            padding: 6px 10px;
            border-radius: 10px;
            color: #10131a;
        }

        .state-blue {
            background: #66a3ff;
        }

        .state-green {
            background: #69e27f;
        }

        .state-yellow {
            background: #ffe066;
        }

        .state-red {
            background: #ff6b6b;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="card">
            <h1>Smart Room</h1>
            <p>ESP32 web panel</p>
            <div class="authors">
                Authors:<br>
                Wiktor Wypyszyński<br>
                Witold Zawada
            </div>
        </div>

        <div class="card">
            <h2>Sensor data</h2>
            <div class="grid">
                <div class="metric">
                    <div class="label">Temperature</div>
                    <div class="value" id="temperature">--</div>
                </div>
                <div class="metric">
                    <div class="label">Humidity</div>
                    <div class="value" id="humidity">--</div>
                </div>
                <div class="metric">
                    <div class="label">Pressure</div>
                    <div class="value" id="pressure">--</div>
                </div>
                <div class="metric">
                    <div class="label">Time</div>
                    <div class="value" id="time">--</div>
                </div>
                <div class="metric">
                    <div class="label">Min temperature</div>
                    <div class="value" id="minTemperature">--</div>
                </div>
                <div class="metric">
                    <div class="label">Max temperature</div>
                    <div class="value" id="maxTemperature">--</div>
                </div>
            </div>
        </div>

        <div class="card">
            <h2>Status</h2>
            <p>State: <b id="state" class="state-badge">--</b></p>
            <p>BME280: <b id="bmeReady">--</b></p>
            <p>DS3231: <b id="rtcReady">--</b></p>
            <p>Buzzer: <b id="buzzerStatus">--</b></p>
        </div>

        <div class="card">
            <h2>Current settings</h2>
            <div class="settings-grid">
                <div class="setting-box">
                    <div class="label">Blue max</div>
                    <div class="value" id="currentBlueMax">--</div>
                </div>
                <div class="setting-box">
                    <div class="label">Green max</div>
                    <div class="value" id="currentGreenMax">--</div>
                </div>
                <div class="setting-box">
                    <div class="label">Yellow max</div>
                    <div class="value" id="currentYellowMax">--</div>
                </div>
                <div class="setting-box">
                    <div class="label">Buzzer enabled</div>
                    <div class="value" id="currentBuzzerEnabled">--</div>
                </div>
            </div>
        </div>

        <div class="card">
            <h2>Modify settings</h2>
            <p class="hint">These fields are for editing only. The current saved values are shown above.</p>

            <div class="row">
                Blue max:
                <input id="blueMax" type="number" step="0.5">
            </div>

            <div class="row">
                Green max:
                <input id="greenMax" type="number" step="0.5">
            </div>

            <div class="row">
                Yellow max:
                <input id="yellowMax" type="number" step="0.5">
            </div>

            <div class="row">
                <label>
                    <input id="buzzerEnabled" type="checkbox">
                    buzzer enabled
                </label>
            </div>

            <button onclick="saveSettings()">Save settings</button>
        </div>
    </div>

    <script>
        let isEditingSettings = false;

        function markEditing() {
            isEditingSettings = true;
        }

        function stopEditing() {
            isEditingSettings = false;
        }

        function getStateClassName(state) {
            if (state.includes('BLUE')) {
                return 'state-badge state-blue';
            }

            if (state.includes('GREEN')) {
                return 'state-badge state-green';
            }

            if (state.includes('YELLOW')) {
                return 'state-badge state-yellow';
            }

            if (state.includes('RED')) {
                return 'state-badge state-red';
            }

            return 'state-badge';
        }

        async function loadStatus() {
            const response = await fetch('/api/status');
            const data = await response.json();

            document.getElementById('temperature').textContent = data.temperature + ' °C';
            document.getElementById('humidity').textContent = data.humidity + ' %';
            document.getElementById('pressure').textContent = data.pressure + ' hPa';
            document.getElementById('time').textContent = data.time;

            document.getElementById('minTemperature').textContent = data.temperatureStatsReady ? data.minTemperature + ' °C' : '--';
            document.getElementById('maxTemperature').textContent = data.temperatureStatsReady ? data.maxTemperature + ' °C' : '--';

            const stateElement = document.getElementById('state');
            stateElement.textContent = data.state;
            stateElement.className = getStateClassName(data.state);

            document.getElementById('bmeReady').textContent = data.bmeReady ? 'OK' : 'ERROR';
            document.getElementById('rtcReady').textContent = data.rtcReady ? 'OK' : 'ERROR';
            document.getElementById('buzzerStatus').textContent = data.buzzerEnabled ? 'ON' : 'OFF';

            document.getElementById('currentBlueMax').textContent = data.blueMax + ' °C';
            document.getElementById('currentGreenMax').textContent = data.greenMax + ' °C';
            document.getElementById('currentYellowMax').textContent = data.yellowMax + ' °C';
            document.getElementById('currentBuzzerEnabled').textContent = data.buzzerEnabled ? 'ON' : 'OFF';

            if (!isEditingSettings) {
                document.getElementById('blueMax').value = data.blueMax;
                document.getElementById('greenMax').value = data.greenMax;
                document.getElementById('yellowMax').value = data.yellowMax;
                document.getElementById('buzzerEnabled').checked = data.buzzerEnabled;
            }
        }

        async function saveSettings() {
            const params = new URLSearchParams();

            params.set('blueMax', document.getElementById('blueMax').value);
            params.set('greenMax', document.getElementById('greenMax').value);
            params.set('yellowMax', document.getElementById('yellowMax').value);
            params.set('buzzerEnabled', document.getElementById('buzzerEnabled').checked ? '1' : '0');

            await fetch('/api/settings', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded'
                },
                body: params.toString()
            });

            stopEditing();
            await loadStatus();
        }

        document.addEventListener('DOMContentLoaded', () => {
            document.getElementById('blueMax').addEventListener('focus', markEditing);
            document.getElementById('greenMax').addEventListener('focus', markEditing);
            document.getElementById('yellowMax').addEventListener('focus', markEditing);
            document.getElementById('buzzerEnabled').addEventListener('change', markEditing);

            loadStatus();
            setInterval(loadStatus, 2000);
        });
    </script>
</body>
</html>
)rawliteral";

    return page;
}

void handleWebRoot() {
    server.send(200, "text/html", buildHtmlPage());
}

void handleWebStatus() {
    server.send(200, "application/json", buildStatusJson());
}

void handleWebSettings() {
    if (server.hasArg("blueMax")) {
        tempBlueMax = server.arg("blueMax").toFloat();
    }

    if (server.hasArg("greenMax")) {
        tempGreenMax = server.arg("greenMax").toFloat();
    }

    if (server.hasArg("yellowMax")) {
        tempYellowMax = server.arg("yellowMax").toFloat();
    }

    if (server.hasArg("buzzerEnabled")) {
        buzzerEnabled = server.arg("buzzerEnabled") == "1";
    }

    keepThresholdsValid();
    updateTemperatureIndicators();
    saveSettings();

    if (currentScreen == SCREEN_SETTINGS || currentScreen == SCREEN_STATUS || currentScreen == SCREEN_MAIN) {
        drawCurrentScreen();
    }

    server.send(200, "application/json", "{\"ok\":true}");
}

void initWebServer() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);

    Serial.print("WiFi AP started. IP: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", handleWebRoot);
    server.on("/api/status", handleWebStatus);
    server.on("/api/settings", HTTP_POST, handleWebSettings);
    server.on("/api/settings", HTTP_GET, handleWebSettings);

    server.begin();

    Serial.println("Web server started");
}

// ======================================================
// app functions
// ======================================================

void setup() {
    Serial.begin(115200);
    delay(500);

    initDisplay();
    initTouch();
    initBme280();
    initClock();
    initRgbLed();
    initBuzzer();
    loadSettings();
    initWebServer();

    readBme280();
    readClock();

    drawCurrentScreen();
    updateRgbLedByTemperature();
    lastTemperatureState = getTemperatureState();
}

void loop() {
    server.handleClient();

    updateTouch();
    updateBme280();
    updateClock();
    updateBuzzer();
}