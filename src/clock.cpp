// Clock's internal state and rendering.

#include "clock.h"
#include "time.h"

#include <NeoPixelBus.h>
#include <RTClib.h> // RVG: only for data structures

#include <limits.h>

namespace {

// Clock state update period, in milliseconds.
#define CLOCK_UPDATE_PERIOD_MS 10
// Clock state update period, in seconds.
#define CLOCK_UPDATE_PERIOD_S (CLOCK_UPDATE_PERIOD_MS / 1000.0)
// Random clock mode update period, in ticks.
#define CLOCK_UPDATE_TICKS_PER_S (1000 / CLOCK_UPDATE_PERIOD_MS)

static_assert(1000 % CLOCK_UPDATE_PERIOD_MS == 0,
              "CLOCK_UPDATE_PERIOD_MS must divide 1000 evenly.");

// Total number of LED words.
#define WORD_COUNT 37

// Lists of LED indexes that correspond to words.
const unsigned char WORD_INDEXES[] = {
    // Qualifiers.
    113, 112, 111, 93, 94, 95, UCHAR_MAX,  // LYGIAI
    91, 90, UCHAR_MAX,  // PO
    91, 70, 71, 72, UCHAR_MAX,  // PUSĖ
    69, 68, UCHAR_MAX,  // BE

    // Hour numbers, nominative.
    50, 51, 52, 43, 42, UCHAR_MAX,  // PIRMA
    84, 83, 82, UCHAR_MAX,  // DVI
    26, 25, 4, 5, UCHAR_MAX,  // TRYS
    48, 47, 26, 27, 28, 29, 30, 31, UCHAR_MAX,  // KETURIOS
    49, 46, 45, 44, 29, 30, 31, UCHAR_MAX,  // PENKIOS
    32, 33, 34, 35, 36, 15, UCHAR_MAX,  // ŠEŠIOS
    88, 87, 86, 75, 64, 53, 54, 55, 40, UCHAR_MAX,  // SEPTYNIOS
    73, 74, 75, 76, 63, 53, 54, 55, 40, UCHAR_MAX,  // AŠTUONIOS
    67, 66, 65, 64, 53, 54, 55, 40, UCHAR_MAX,  // DEVYNIOS
    110, 109, 108, 107, 106, 105, UCHAR_MAX,  // DEŠIMT
    96, 97, 98, 85, 76, 63, 62, 61, 56, 39, UCHAR_MAX,  // VIENUOLIKA
    84, 83, 77, 62, 61, 56, 39, UCHAR_MAX,  // DVYLIKA

    // Hour numbers, genitive.
    50, 51, 52, 43, 30, 31, UCHAR_MAX,  // PIRMOS
    84, 83, 82, 78, 79, 60, UCHAR_MAX,  // DVIEJŲ
    26, 25, 24, 23, 22, UCHAR_MAX,  // TRIJŲ
    48, 47, 26, 27, 28, 29, 22, UCHAR_MAX,  // KETURIŲ
    49, 46, 45, 44, 29, 22, UCHAR_MAX,  // PENKIŲ
    32, 33, 34, 35, 16, UCHAR_MAX,  // ŠEŠIŲ
    88, 87, 86, 75, 64, 53, 54, 41, UCHAR_MAX,  // SEPTYNIŲ
    73, 74, 75, 76, 63, 53, 54, 41, UCHAR_MAX,  // AŠTUONIŲ
    67, 66, 65, 64, 53, 54, 41, UCHAR_MAX,  // DEVYNIŲ
    110, 109, 108, 107, 106, 105, 104, 103, UCHAR_MAX,  // DEŠIMTOS
    96, 97, 98, 85, 76, 63, 62, 61, 56, 57, 38, UCHAR_MAX,  // VIENUOLIKOS
    84, 83, 77, 62, 61, 56, 57, 38, UCHAR_MAX,  // DVYLIKOS

    // Minute numbers.
    UCHAR_MAX,  // [null]
    21, 20, 19, 18, 17, 12, 13, UCHAR_MAX,  // PENKIOS
    21, 20, 19, 18, 17, 16, UCHAR_MAX,  // PENKIŲ
    102, 81, 80, 59, 58, 37, UCHAR_MAX,  // DEŠIMT
    21, 20, 19, 18, 17, 7, 8, 9, 10, 11, UCHAR_MAX,  // PENKIOLIKA
    21, 20, 19, 18, 17, 7, 8, 9, 10, 12, 13, UCHAR_MAX,  // PENKIOLIKOS
    99, 100, 101, 102, 81, 80, 59, 58, 37, UCHAR_MAX,  // DVIDEŠIMT
    99, 100, 101, 102, 81, 80, 59, 58, 37, 21, 20, 19, 18, 17, 12, 13,
        UCHAR_MAX,  // DVIDEŠIMT PENKIOS
    99, 100, 101, 102, 81, 80, 59, 58, 37, 21, 20, 19, 18, 17, 16,
        UCHAR_MAX,  // DVIDEŠIMT PENKIŲ
};

// Index where qualifier word block begins.
#define WORD_QUALIFIER_START 0
// Index where nominative hour word block begins.
#define WORD_HOUR_NOMINATIVE_START 4
// Index where genitive hour word block begins.
#define WORD_HOUR_GENITIVE_START 16
// Index where nominative minute word block begins.
#define WORD_MINUTE_START 28

// Index of the empty word.
#define NULL_WORD_INDEX 28

// A mapping from minute blocks to qualifier words as indexes of WORD_INDEXES,
// expressed as offsets from WORD_QUALIFIER_START.
const unsigned char QUALIFIER_WORD_OFFSETS[] = {
    0, 1, 1, 1, 1, 1, 2, 3, 3, 3, 3, 3
};

// A mapping from minute blocks to minute words as indexes of WORD_INDEXES,
// expressed as offsets from WORD_MINUTE_START.
const unsigned char MINUTE_WORD_OFFSETS[] = {
    0, 1, 3, 4, 6, 7, 0, 8, 6, 5, 3, 2
};

// Index of the LED that corresponds to the period character.
#define PERIOD_LED_INDEX 14

// LEDs that correspond to the corner indicators.
const unsigned char CORNER_INDEXES[] = {3, 2, 1, 0};
// Number of corner indicators in the clock face.
#define CORNER_COUNT (sizeof(CORNER_INDEXES) / sizeof(*CORNER_INDEXES))

// Color that turns off an LED.
const RgbColor OFF_COLOR = RgbColor(0, 0, 0);
// Color palettes to use for lighting the words.
const RgbColor PALETTES[PALETTE_COUNT][SHOWN_WORD_COUNT] = {
    // Red, orange, white.
    { RgbColor(190, 9, 0), RgbColor(203, 91, 10), RgbColor(254, 204, 92) },
    // Green, yellow, white.
    { RgbColor(12, 102, 0), RgbColor(244, 255, 20), RgbColor(252, 254, 233) },
    // Blue, cyan, white.
    { RgbColor(0, 0, 137), RgbColor(35, 255, 226), RgbColor(241, 254, 250) },
    // Purple, pink, white.
    { RgbColor(24, 0, 96), RgbColor(255, 71, 208), RgbColor(254, 241, 251) },
    // Red, cyan, white
    { RgbColor(144, 14, 0), RgbColor(38, 255, 246), RgbColor(253, 254, 246) },
    // Violet, green, white.
    { RgbColor(80, 0, 130), RgbColor(47, 255, 15), RgbColor(246, 249, 254) },
    // Blue, yellow, white.
    { RgbColor(0, 15, 130), RgbColor(255, 246, 15), RgbColor(254, 246, 247) },
};

// Offsets into the LED index that correspond to beginnings of words.
int word_offsets[WORD_COUNT];

// Builds word offset table.
void buildWordOffsets() {
    Serial.println("=buildWordOffsets()");
    static bool initialized = false;
    if (initialized) return;

    word_offsets[0] = 0;
    int next_offset_index = 1;

    for (int i = 0; i < sizeof(WORD_INDEXES); i++) {
        if (WORD_INDEXES[i] != UCHAR_MAX) continue;
        if (next_offset_index == WORD_COUNT) {
            if (i != sizeof(WORD_INDEXES) - 1) {
                Serial.println("[ERROR] Word offset table is too small.");
            }
            break;
        }
        word_offsets[next_offset_index++] = i + 1;
    }
    if (next_offset_index != WORD_COUNT) {
        Serial.print("[ERROR] Word offset table is too large, found only ");
        Serial.print(next_offset_index);
        Serial.println(" entries.");
    }

    initialized = true;
}

}  // namespace

WordClock::WordClock(NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod>* led_strip)
    : led_strip_(led_strip) {}

WordClock::~WordClock() {}

void WordClock::setup() {
    Serial.println("=WordClock::setup()");
    if (initialized_) {
        Serial.println("[WARN] Trying to setup WordClock multiple times.");
        return;
    }
    buildWordOffsets();

    clock_mode_ = ClockMode::REAL_TIME;
    last_tick_ms_ = millis();
    last_tick_ms_ -= last_tick_ms_ % CLOCK_UPDATE_PERIOD_MS;

    initialized_ = true;
}

DateTime WordClock::getCurrentTime() {
  struct tm timeinfo;

// NOTE: the second arg in getLocalTime() is the max [ms] the function waits 
//       for the time to be received from the NTP server. Be aware to set it to
//       a low value to prevent blocking issues!
//       See the following resources:
// https://github.com/espressif/arduino-esp32/blob/34125cee1d1c6a81cd1da07de27ce69d851f9389/cores/esp32/esp32-hal-time.c#L87
// https://techtutorialsx.com/2021/09/01/esp32-system-time-and-sntp/
// https://www.lucadentella.it/en/2017/05/11/esp32-17-sntp/
  getLocalTime(&timeinfo, 20); 

//  time_t rawtime = time(NULL); // time in seconds since the Epoch (00:00:00 UTC, January 1, 1970)

// For debug only (lots of output generated when commented out)
//  Serial.print(" getCurrentTime: ");
//  Serial.println(&timeinfo, " %A, %B %d %Y %H:%M:%S");

  return DateTime(timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday,
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

void WordClock::setDisplayStateToDateTime_(DateTime datetime) {
    const int minute_block = datetime.minute() / 5;
    const int hour = (datetime.hour() + 11 + (minute_block >= 6 ? 1 : 0)) % 12;
    bool getLTStatus = false;
    static DateTime lastDateTime;

    if (datetime == lastDateTime) return;

    lastDateTime = datetime;

    // temp for debug
    struct tm timeinfo;
    getLTStatus = getLocalTime(&timeinfo, 10);
    Serial.print(&timeinfo, " %A, %B %d %Y %H:%M:%S");
    Serial.printf(" (%d)\n", getLTStatus);
    
    shown_words_[0] =
            WORD_QUALIFIER_START + QUALIFIER_WORD_OFFSETS[minute_block];
    const int hour_offset = minute_block == 0 || minute_block > 6
            ? WORD_HOUR_NOMINATIVE_START : WORD_HOUR_GENITIVE_START;
    shown_words_[1] = hour_offset + hour;
    shown_words_[2] = WORD_MINUTE_START + MINUTE_WORD_OFFSETS[minute_block];

    if (minute_block > 6) {
        const unsigned char swap_tmp = shown_words_[1];
        shown_words_[1] = shown_words_[2];
        shown_words_[2] = swap_tmp;
    }

    shown_word_count_ = minute_block == 0 || minute_block == 6 ? 2 : 3;
    block_progress_ =
            ((datetime.minute() % 5) * 60.0f + datetime.second()) / 300.0f;
}

void WordClock::updateDisplayState_() {
    switch (clock_mode_) {
        case ClockMode::CYCLE_5_MINS:
            setDisplayStateToDateTime_(DateTime(
                    uint32_t(tick_id_ * CLOCK_UPDATE_PERIOD_S) * 60 * 5));
            break;
        case ClockMode::CYCLE_HOURS:
            setDisplayStateToDateTime_(DateTime(
                    uint32_t(tick_id_ * CLOCK_UPDATE_PERIOD_S) * 60 * 65));
            break;
        case ClockMode::CYCLE_PALETTES: {
            setDisplayStateToDateTime_(getCurrentTime());
            palette_id_ = (uint32_t(tick_id_ * CLOCK_UPDATE_PERIOD_S * 0.5)
                           % PALETTE_COUNT) + 1;
            break;
        }
        case ClockMode::CYCLE_WORDS: {
            shown_words_[0] =
                    uint32_t(tick_id_ * CLOCK_UPDATE_PERIOD_S) % WORD_COUNT;
            shown_words_[1] = NULL_WORD_INDEX;
            shown_words_[2] = NULL_WORD_INDEX;
            shown_word_count_ = 1;
            break;
        }
        case ClockMode::FAST_TIME:
            setDisplayStateToDateTime_(DateTime(uint32_t(
                    tick_id_ * fast_time_factor_ * CLOCK_UPDATE_PERIOD_S)));
            break;
        case ClockMode::RANDOM: {
            if (tick_id_ % (CLOCK_UPDATE_TICKS_PER_S * 2) == 0) {
                current_random_datetime_ = DateTime(random(60 * 60 * 24));
            }
            setDisplayStateToDateTime_(current_random_datetime_);
            break;
        }
        case ClockMode::REAL_TIME:
        default:
            setDisplayStateToDateTime_(
//                    getCurrentTime() + TimeSpan(0, dst_ ? 1 : 0, 0, 0));
                    getCurrentTime());
            break;
    }
}

void WordClock::renderDisplayState_() {
    const RgbColor* const palette =
            palette_id_ == 0 ? custom_palette_ : PALETTES[palette_id_ - 1];

    // Render words.
    for (int i = 0; i < SHOWN_WORD_COUNT; i++) {
        for (int j = word_offsets[shown_words_[i]];
             WORD_INDEXES[j] != UCHAR_MAX; j++) {
            const RgbColor* const color = &palette[i];
            led_strip_->SetPixelColor(WORD_INDEXES[j], *color);
        }
    }
    if (period_) {
        led_strip_->SetPixelColor(PERIOD_LED_INDEX,
                                  palette[shown_word_count_ - 1]);
    }

    // Render corner indicators.
    for (int i = 0; i < CORNER_COUNT; i++) {
        const int transition_id = int(block_progress_ * 3.0);
        float transition_factor =
                (block_progress_ * 3.0 - transition_id) * 4.0 - i;
        transition_factor = min(1.0f, max(0.0f, transition_factor));

        const RgbColor* const start_color = transition_id == 0
                ? &OFF_COLOR : &palette[transition_id - 1];
        const RgbColor* const end_color = &palette[transition_id];

        led_strip_->SetPixelColor(CORNER_INDEXES[i],
                                  RgbColor::LinearBlend(*start_color, *end_color,
                                                        transition_factor));
    }
}

void WordClock::update_() {
    updateDisplayState_();

    led_strip_->ClearTo(OFF_COLOR);
    renderDisplayState_();
    led_strip_->Show();
}

void WordClock::loop() {
    if (!initialized_) {
        Serial.println("[ERROR] WordClock not initialized, loop aborted.");
        return;
    }
    const unsigned long current_ms = millis();

    if (current_ms - last_tick_ms_ > CLOCK_UPDATE_PERIOD_MS) {
      update_();
      last_tick_ms_ = current_ms;
      tick_id_++;
    }

//    // Handle clock rollover.
//    if (current_ms < last_tick_ms_) {
//        update_();
//        last_tick_ms_ = 0;
//        tick_id_++;
//        return;
//    }
//
//    const unsigned long update_delay_ms = current_ms - last_tick_ms_;
//    if (update_delay_ms > CLOCK_UPDATE_PERIOD_MS) {
//        update_();
//        last_tick_ms_ += update_delay_ms;
//        last_tick_ms_ -= last_tick_ms_ % CLOCK_UPDATE_PERIOD_MS;
//        tick_id_++;
//    }
}
