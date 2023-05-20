#ifndef WORDCLOCK_CLOCK_H_
#define WORDCLOCK_CLOCK_H_

#include <NeoPixelBus.h>
#include <RTClib.h> // RVG: only for data structures

// The number of predefined color palettes.
#define PALETTE_COUNT 7
// Number of words lit up on the clock face.
#define SHOWN_WORD_COUNT 3

// Time to show when RTC is not available.
const DateTime DEFAULT_DATETIME(2020, 1, 1, 0, 0, 0);

// Clock display mode.
enum class ClockMode {
    // Show current time.
    REAL_TIME,
    // Advance time quickly.
    FAST_TIME,
    // Cycle through random times.
    RANDOM,
    // Advance time 1 hour 5 minutes per second.
    CYCLE_HOURS,
    // Advance time 5 minutes per second.
    CYCLE_5_MINS,
    // Cycle through all of the clock's words.
    CYCLE_WORDS,
    // Cycle through all of the clock's color palettes.
    CYCLE_PALETTES,

    // Largest numeric value of a clock mode.
    MAX_VALUE = CYCLE_PALETTES,
};

// Maintains and renders the word clock's state.
class WordClock {
  public:
    // Constructs a new word clock with the provided dependencies.
    WordClock(NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod>* led_strip);

    ~WordClock();

    WordClock(const WordClock&) = delete;
    WordClock& operator=(const WordClock&) = delete;

    WordClock(WordClock&&) = delete;
    WordClock& operator=(WordClock&&) = delete;

    // Initializes the word clock. Expects clock's dependencies to be
    // initialized.
    void setup();
    // Executes clock's event loop. Expects the word clock to be initialized.
    void loop();

    // Sets whether daylight saving time is enabled.
    void setDst(bool dst) { dst_ = dst; }

    // Sets a new palette id.
    void setPaletteId(int palette_id) { palette_id_ = palette_id; }
    // Sets color 1 in the custom palette.
    void setCustomColor1(const RgbColor& color) { custom_palette_[0] = color; }
    // Sets color 2 in the custom palette.
    void setCustomColor2(const RgbColor& color) { custom_palette_[1] = color; }
    // Sets color 3 in the custom palette.
    void setCustomColor3(const RgbColor& color) { custom_palette_[2] = color; }
    // Sets whether to render the period character.
    void setPeriod(bool period) { period_ = period; }

    // Sets the clock mode.
    void setClockMode(ClockMode mode) { clock_mode_ = mode; }
    // Sets FAST_TIME clock mode speed factor.
    void setFastTimeFactor(int factor) { fast_time_factor_ = factor; }

    // get the time according to the time received from the NTP server
    DateTime getCurrentTime();
    
  private:
    // Updates clock display state to match the provided `datetime`.
    void setDisplayStateToDateTime_(DateTime datetime);
    // Updates the clock's display state for the current tick.
    void updateDisplayState_();
    // Draws the current clock state.
    void renderDisplayState_();

    // Updates clock state.
    void update_();

    // Whether the word clock was initialized.
    bool initialized_ = false;

    // RTC chip interface.
//    RTC_DS3231* rtc_ = nullptr;
    // Clock display mode.
    ClockMode clock_mode_ = ClockMode::REAL_TIME;
    // Whether daylight saving time is enabled.
    bool dst_ = false;
    // FAST_TIME mode speed factor relative to REAL_TIME mode.
    int fast_time_factor_ = 30;
    // Current datetime for RANDOM display mode.
    DateTime current_random_datetime_;

    // Number of elapsed update ticks.
    unsigned long tick_id_ = 0;
    // Time in milliseconds when the clock was last updated.
    unsigned long last_tick_ms_ = 0;
    // What time it is within a 5 minute block.
    float block_progress_ = 0.0f;

    // LED strip interface.
    NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod>* led_strip_ = nullptr;
    // User settable color palette.
    RgbColor custom_palette_[SHOWN_WORD_COUNT];
    // Id of the color palette used to light up words.
    int palette_id_ = 0;
    // Indexes of the lit up words.
    unsigned char shown_words_[SHOWN_WORD_COUNT] = {0};
    // Number of shown words.
    int shown_word_count_ = 0;
    // Whether to render the period character.
    bool period_ = false;
};

#endif  // WORDCLOCK_CLOCK_H_
