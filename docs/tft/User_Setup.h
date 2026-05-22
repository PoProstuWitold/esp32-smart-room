//                            USER DEFINED SETTINGS
//   Set driver type, fonts to be loaded, pins used and SPI control method etc.
//
//   This configuration is prepared for:
//   ESP32 WROVER + ILI9341 SPI TFT + XPT2046 touch controller

#define USER_SETUP_INFO "ESP32 WROVER + ILI9341 + XPT2046"

// Define to disable all #warnings in library
//#define DISABLE_ALL_LIBRARY_WARNINGS

// ##################################################################################
//
// Section 1. Call up the right driver file and any options for it
//
// ##################################################################################

// Only define one driver, the other ones must be commented out
#define ILI9341_2_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Some displays support SPI reads via the MISO pin, other displays have a single
// bi-directional SDA pin and the library will try to read this via the MOSI line.
// To use the SDA line for reading data from the TFT uncomment the following line:

// #define TFT_SDA_READ

// For ST7735, ST7789 and ILI9341 ONLY, define the colour order IF the blue and red are swapped
// Uncomment ONE of the lines below only if colours are wrong:
//
// #define TFT_RGB_ORDER TFT_RGB
#define TFT_RGB_ORDER TFT_BGR

// If colours are inverted (white shows as black) then uncomment one of the next
// 2 lines and try both options if needed:
//
#define TFT_INVERSION_ON
// #define TFT_INVERSION_OFF

// ##################################################################################
//
// Section 2. Define the pins that are used to interface with the display here
//
// ##################################################################################

// ESP32 WROVER SPI pins
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18

// TFT control pins
#define TFT_CS   5
#define TFT_DC   32
#define TFT_RST  33

// Touch controller chip select pin
#define TOUCH_CS 14

// Optional backlight pin definition
// Not used here because LED is connected directly to 3.3V
// #define TFT_BL 21
// #define TFT_BACKLIGHT_ON HIGH

// ##################################################################################
//
// Section 3. Define the fonts that are to be used here
//
// ##################################################################################

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

// ##################################################################################
//
// Section 4. Other options
//
// ##################################################################################

// SPI clock frequency
#define SPI_FREQUENCY       10000000
#define SPI_READ_FREQUENCY  10000000
#define SPI_TOUCH_FREQUENCY 2500000

// Transactions are automatically enabled on ESP32 by the library,
// so no need to define SUPPORT_TRANSACTIONS here.
