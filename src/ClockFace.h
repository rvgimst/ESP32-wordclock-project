#pragma once

#include <vector>

// The number of LEDs connected before the start of the matrix.
#define NEOPIXEL_SIGNALS 4

// Matrix dimensions.
#define NEOPIXEL_ROWS 11
#define NEOPIXEL_COLUMNS 10

// Number of LEDs on the whole strip.
#define NEOPIXEL_COUNT (NEOPIXEL_ROWS * NEOPIXEL_COLUMNS + NEOPIXEL_SIGNALS)

// max length of word to be found in the board
#define PUZZLE_MAX_SEQUENCE 20

class ClockFace
{
public:
  static int pixelCount();

  // The orientation of the clock is infered from where the light sensor is.
  enum class LightSensorPosition
  {
    Bottom,
    Top
  };

  ClockFace(LightSensorPosition position);

  // Rotates the display.
  void setLightSensorPosition(LightSensorPosition position);

  // Updates the state by setting to true all the LEDs that need to be turned on
  // for the given time. Returns false if there is no change since last update,
  // in which case the state is not updated.
  virtual bool stateForTime(int hour, int minute, int second, bool show_ampm) = 0;

  // Returns the state of all LEDs as pixels. Updated when updateStateForTime()
  // is called.
  const std::vector<bool> &getState() { return _state; };

  // public puzzle mode word finder function
  void showLetterSequence(String str);

protected:
  // Lights up a segment in the state.
  void updateSegment(int x, int y, int length);

  // Returns the index of the LED in the strip given a position on the grid.
  uint16_t map(int16_t x, int16_t y);

  // The first four LED are the corner ones, counting minutes. They are assumed
  // to be wired in clockwise order, starting from the light sensor position.
  // mapMinute() returns the proper index based on desired location taking
  // orientation into account..
  enum Corners
  {
    TopLeft,
    BottomLeft,
    BottomRight,
    TopRight
  };
  uint16_t mapMinute(Corners corner);

  // To avoid refreshing too often, this stores the time of the previous UI
  // update. If nothing changed, there will be no interuption of animations.
  int _hour, _minute, _second;
  bool _show_ampm;

  LightSensorPosition _position;

  // Stores the bits of the clock that need to be turned on.
  std::vector<bool> _state;

  //////////////////////////////////////////////////////////////////////////////////
  // Declarations & defines for puzzle-mode where a sequence of letters is shown 
  // with minimum distance between each consecutive letter
  //////////////////////////////////////////////////////////////////////////////////
  struct coord { // struct for storing a sequence of letters on the board
    int x;
    int y;
  };

  // 2D array of the letter-elements of the clock (top left == 0,0)
  char _letters[NEOPIXEL_ROWS][NEOPIXEL_COLUMNS];

  // recursive function to find best match for a word on the board
  void findLetterSequence(String str, int *bestCost, coord *bestSolution, int *bestNumElems, int runningCost, coord *runningSolution, int runningNumElems);
};

class FrenchClockFace : public ClockFace
{
public:
  FrenchClockFace(LightSensorPosition position);

  virtual bool stateForTime(int hour, int minute, int second, bool show_ampm);
};

class EnglishClockFace : public ClockFace
{
public:
  EnglishClockFace(LightSensorPosition position);

  virtual bool stateForTime(int hour, int minute, int second, bool show_ampm);
};
