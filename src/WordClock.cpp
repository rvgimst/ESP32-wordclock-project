#include <arduino.h>
#include "Display.h"
#include "ClockFace.h"
#include "iot_config.h"

#include <IotWebConf.h>
#include <NeoPixelBus.h>

// Baud rate of the serial output.
#define SERIAL_BAUD_RATE 115200

// Light sensor pin number.
//#define LDR_PIN 25
// LED strip pin number.
//#define NEOPIXEL_PIN 32
// Number of LEDs in the LED strip.
// #define NEOPIXEL_COUNT 114
// Number of LEDs in a single row of the grid.
// #define PIXEL_GRID_WIDTH 11

namespace {
  EnglishClockFace clockFace(ClockFace::LightSensorPosition::Bottom);
  Display display(clockFace);
  IotConfig iot_config(&display);
}  // namespace

// Initializes sketch.
void setup() {
    // Initialize serial output.
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial);

    display.setup();
    iot_config.setup();
}

// Executes the event loop once.
void loop() {
  iot_config.loop();
  display.loop();
}
