#pragma once

#include <Arduino.h>

const char clockmode0[] PROGMEM = "ClockMode/Real Clock";
const char clockmode1[] PROGMEM = "ClockMode/Color Test";
const char clockmode2[] PROGMEM = "ClockMode/Word Puzzle";

const char modeval0[] PROGMEM = "0";
const char modeval1[] PROGMEM = "1";
const char modeval2[] PROGMEM = "2";

const char *const clockmode[] PROGMEM = {clockmode0, clockmode1, clockmode2};
const char *const modeval[] PROGMEM = {modeval0, modeval1, modeval2};
const char clockModeOptions[] PROGMEM = "data-options='ClockMode/Real Clock|ClockMode/Color Test|ClockMode/Word Puzzle'";

// Clock display mode.
enum class ClockMode {
    // Show current time.
    REAL_TIME,
    // Color test animation mode
    COLOR_TEST,
    // Word finder puzzle mode
    PUZZLE_MODE,

    // Largest numeric value of a clock mode.
    MAX_VALUE = PUZZLE_MODE,
};