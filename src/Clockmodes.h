#pragma once

#include <Arduino.h>

const char clockmode0[] PROGMEM = "ClockMode/Real Clock";
const char clockmode1[] PROGMEM = "ClockMode/Color Test";

const char modeval0[] PROGMEM = "0";
const char modeval1[] PROGMEM = "1";

const char *const clockmode[] PROGMEM = {clockmode0, clockmode1};
const char *const modeval[] PROGMEM = {modeval0, modeval1};
const char clockModeOptions[] PROGMEM = "data-options='ClockMode/Real Clock|ClockMode/Color Test'";

// Clock display mode.
enum class ClockMode {
    // Show current time.
    REAL_TIME,
    // Color test animation mode
    COLOR_TEST,

    // Largest numeric value of a clock mode.
    MAX_VALUE = COLOR_TEST,
};