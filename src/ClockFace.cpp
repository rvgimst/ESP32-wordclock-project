#include "logging.h"

#include <NeoPixelBus.h> // Only need NeoTopology

#include "ClockFace.h"

// static
int ClockFace::pixelCount()
{
  return NEOPIXEL_COUNT;
}

ClockFace::ClockFace(LightSensorPosition position) : _hour(-1), _minute(-1), _second(-1),
                                                     _position(position), _state(NEOPIXEL_COUNT){};

void ClockFace::setLightSensorPosition(LightSensorPosition position)
{
  _position = position;
}

uint16_t ClockFace::map(int16_t x, int16_t y)
{
  switch (_position)
  {
  case LightSensorPosition::Top:
  {
    static NeoTopology<ColumnMajorAlternating90Layout> sensor_on_top(
        NEOPIXEL_ROWS, NEOPIXEL_COLUMNS);
    return sensor_on_top.Map(x, y) + NEOPIXEL_SIGNALS;
  }
  case LightSensorPosition::Bottom:
  {
    static NeoTopology<ColumnMajorAlternating270Layout> sensor_on_bottom(
        NEOPIXEL_ROWS, NEOPIXEL_COLUMNS);
    return sensor_on_bottom.Map(x, y) + NEOPIXEL_SIGNALS;
  }
  default:
    DCHECK(false, static_cast<int>(_position));
  }
  return 0;
}

uint16_t ClockFace::mapMinute(Corners corner)
{
  switch (_position)
  {
  case LightSensorPosition::Bottom:
    return (static_cast<uint16_t>(corner) + 2) % 4;
  case LightSensorPosition::Top:
    return static_cast<uint16_t>(corner);
  default:
    DCHECK(false, static_cast<int>(corner));
  }
  return 0;
}

// Lit a segment in updateState.
void ClockFace::updateSegment(int x, int y, int length)
{
  for (int i = x; i <= x + length - 1; i++)
    _state[map(i, y)] = true;
}

// Lit a sequence of letters on the board [PUBLIC]
void ClockFace::showLetterSequence(String str)
{
  coord bestSolution[PUZZLE_MAX_SEQUENCE];
  coord runningSolution[PUZZLE_MAX_SEQUENCE];
  int bestCost = 9999;
  int runningCost = 0;
  int bestNumElems = 0;
  int runningNumElems = 0;

  // Reset the board to all black
  for (int i = 0; i < NEOPIXEL_COUNT; i++)
    _state[i] = false;

  findLetterSequence(str, &bestCost, bestSolution, &bestNumElems, runningCost, runningSolution, runningNumElems);
  Serial.printf("ClockFace::showLetterSequence(%s) score:%d\n", str, bestCost);

  if (bestCost == 9999) { // no solution found
    // light up the corner LEDs only
    for (int i = 0; i < 4; i++) {
      _state[i] = true;
    }
  }
  else {
    for (int i = 0; i < bestNumElems; i++) {
      _state[map(bestSolution[i].y, bestSolution[i].x)] = true;
    }
  }
}

// Given a sequence of letters, find a solution with the shortest path between them.
// NOTE: This is a recursive function
// return value: sequence of coordinates that make up the given sequence
void ClockFace::findLetterSequence(String str, int *bestCost, coord *bestSolution, int *bestNumElems, int runningCost, coord *runningSolution, int runningNumElems)
// to add: prevCoord to indicate location of last letter
//         currentCost
{
  if (str == "") { // end of search (no more letters to search for in the word)
    // update best* params if current solution is better
    if (runningCost < *bestCost) {
      int x,y;
      *bestCost = runningCost;
      *bestNumElems = runningNumElems;
      Serial.printf("ClockFace::findLetterSequence() SOLUTION FOUND, score:%d\n", *bestCost);
      Serial.printf("  SOLUTION (x,y): ");
      for (int i=0; i<(runningNumElems); i++) {
        bestSolution[i] = runningSolution[i];
        x = bestSolution[i].x;
        y = bestSolution[i].y;
        Serial.printf("(%c,%d,%d) ", _letters[x][y], x, y);
      }
      Serial.printf("\n");
    }
    return;
  }

  char c = str[0]; // search character
  coord lastCoord;
  int dist = 0;

  // get the last added coordinate/letter from the running solution
  // (needed to calculate the distance to that position)
  if (runningNumElems > 0) { // was there a previous letter chosen?
    lastCoord = runningSolution[runningNumElems-1];
  }

  str.remove(0, 1); // remove the first character from string so we can pass on the rest recursively

  // go over the board and find all instances of our search character
  for (int x=0; x<NEOPIXEL_ROWS; x++) {
    for (int y=0; y<NEOPIXEL_COLUMNS; y++) {
      if (_letters[x][y] == c) {
        if (runningNumElems > 0) {
          // determine the distance between this letter and the previous (if any)
          dist = abs(x - lastCoord.x) + abs(y - lastCoord.y); // maybe make this a function later?
        }
        if (runningCost + dist < *bestCost) { // solution is still better than last found solution
          // add c's position to runningSolution at the end (NumElems)
          runningSolution[runningNumElems].x = x;
          runningSolution[runningNumElems].y = y;
          // recursive call
          findLetterSequence(str, bestCost, bestSolution, bestNumElems, runningCost+dist, runningSolution, runningNumElems+1);
          // remove c's position from runningSolution is not needed since NumElems does not change in local scope
        }
      }
    } // continue to next letter on the board
  }

  // ideas:
  // use _letters[][] to access the board's letter Layout
    
  //   take the first letter L in str
  //   for all L in _letters do
  //   {
  //     // determine distance D between prevcoord and L's coordinates on the board
  //     if (currentCost + D > bestCost) break; // (next letter L since it won't get better, heuristic)
  //     findLetterSequence(str-L, currentCost+D, bestCost, solution)
  //   }

  // TEST
  // *bestCost = 10;
  // for (int i = 0; i < str.length(); i++) {
  //   bestSolution[i].x = i;
  //   bestSolution[i].y = 0;
  // }
}


//
// Constants to match the ClockFace.
//
// Custom French face. Can show time with some less usual variations like
// MINUIT TROIS QUARTS or DEUX HEURES PILE. This also includes all the letters
// of the alphabets as well as !. Letters in lowercase below are not used
// by the clock.

// ILbESTjDEUX
// QUATRETROIS
// NEUFUNESEPT
// HUITSIXCINQ
// MIDIXMINUIT
// ONZEwHEURES
// MOINSyLEDIX
// ETTROISDEMI
// VINGT-CINQk
// QUARTSPILE!
//

// All the segments of words on the board. The first too numbers are the
// coordinate of the first letter of the word, the last is the length. A word
// must always be on one row.
#define FR_S_IL 0, 0, 2
#define FR_S_EST 3, 0, 3

#define FR_H_UNE 4, 2, 3
#define FR_H_DEUX 7, 0, 4
#define FR_H_TROIS 6, 1, 5
#define FR_H_QUATRE 0, 1, 6
#define FR_H_CINQ 7, 3, 4
#define FR_H_SIX 4, 3, 3
#define FR_H_SEPT 7, 2, 4
#define FR_H_HUIT 0, 3, 4
#define FR_H_NEUF 0, 2, 4
#define FR_H_DIX 2, 4, 3
#define FR_H_ONZE 0, 5, 4

#define FR_H_HEURE 5, 5, 5
#define FR_H_HEURES 5, 5, 6

#define FR_H_MIDI 0, 4, 4
#define FR_H_MINUIT 5, 4, 6

#define FR_M_MOINS 0, 6, 5
#define FR_M_LE 6, 6, 2
#define FR_M_ET 0, 7, 2
#define FR_M_TROIS 2, 7, 5

#define FR_M_DIX 8, 6, 3
#define FR_M_VINGT 0, 8, 5
#define FR_M_VINGTCINQ 0, 8, 10
#define FR_M_CINQ 6, 8, 4
#define FR_M_DEMI 7, 7, 4
#define FR_M_QUART 0, 9, 5
#define FR_M_QUARTS 0, 9, 6
#define FR_M_PILE 6, 9, 4

FrenchClockFace::FrenchClockFace(LightSensorPosition position) : ClockFace(position)
{
  strncpy(_letters[0], "ILbESTjDEUX", NEOPIXEL_ROWS);
}

bool FrenchClockFace::stateForTime(int hour, int minute, int second, bool show_ampm)
{
  if (hour == _hour && minute == _minute)
  {
    return false;
  }
  _hour = hour;
  _minute = minute;

  DLOGLN("update state");

  // TODO move to a more convenient place
  // Reset the board to all black
  for (int i = 0; i < NEOPIXEL_COUNT; i++)
    _state[i] = false;

  int leftover = minute % 5;
  minute = minute - leftover;

  if (minute >= 35)
    hour = (hour + 1) % 24; // Switch to "TO" minutes the next hour

  updateSegment(FR_S_IL);
  updateSegment(FR_S_EST);

  switch (hour)
  {
  case 0:
    updateSegment(FR_H_MINUIT);
    break;
  case 1:
  case 13:
    updateSegment(FR_H_UNE);
    break;
  case 2:
  case 14:
    updateSegment(FR_H_DEUX);
    break;
  case 3:
  case 15:
    updateSegment(FR_H_TROIS);
    break;
  case 4:
  case 16:
    updateSegment(FR_H_QUATRE);
    break;
  case 5:
  case 17:
    updateSegment(FR_H_CINQ);
    break;
  case 6:
  case 18:
    updateSegment(FR_H_SIX);
    break;
  case 7:
  case 19:
    updateSegment(FR_H_SEPT);
    break;
  case 8:
  case 20:
    updateSegment(FR_H_HUIT);
    break;
  case 9:
  case 21:
    updateSegment(FR_H_NEUF);
    break;
  case 10:
  case 22:
    updateSegment(FR_H_DIX);
    break;
  case 11:
  case 23:
    updateSegment(FR_H_ONZE);
    break;
  case 12:
    updateSegment(FR_H_MIDI);
    break;
  default:
    DLOG("Invalid hour ");
    DLOGLN(hour);
  }
  switch (hour)
  {
  case 0:
  case 12:
    break;
  case 1:
  case 13:
    updateSegment(FR_H_HEURE);
    break;
  default:
    updateSegment(FR_H_HEURES);
    break;
  }

  switch (minute)
  {
  case 0:
    break;
  case 5:
    updateSegment(FR_M_CINQ);
    break;
  case 10:
    updateSegment(FR_M_DIX);
    break;
  case 15:
    updateSegment(FR_M_ET);
    updateSegment(FR_M_QUART);
    break;
  case 20:
    updateSegment(FR_M_VINGT);
    break;
  case 25:
    updateSegment(FR_M_VINGTCINQ);
    break;
  case 30:
    updateSegment(FR_M_ET);
    updateSegment(FR_M_DEMI);
    break;
  case 35:
    updateSegment(FR_M_MOINS);
    updateSegment(FR_M_VINGTCINQ);
    break;
  case 40:
    updateSegment(FR_M_MOINS);
    updateSegment(FR_M_VINGT);
    break;
  case 45:
    updateSegment(FR_M_MOINS);
    updateSegment(FR_M_LE);
    updateSegment(FR_M_QUART);
    break;
  case 50:
    updateSegment(FR_M_MOINS);
    updateSegment(FR_M_DIX);
    break;
  case 55:
    updateSegment(FR_M_MOINS);
    updateSegment(FR_M_CINQ);
    break;
  default:
    DLOG("Invalid minute ");
    DLOGLN(minute);
  }

  switch (leftover)
  {
  case 4:
    _state[mapMinute(TopLeft)] = true;
  case 3: // fall through
    _state[mapMinute(BottomLeft)] = true;
  case 2: // fall through
    _state[mapMinute(BottomRight)] = true;
  case 1: // fall through
    _state[mapMinute(TopRight)] = true;
  case 0: // fall through
    break;
  }
  return true;
}

// Constants to match the EnglishClockFace.
//
// Letters in lowercase below are not used by the clock.

// ITlISasAM PM
// A c QUARTER dc
// TWENTY FIVE x
// HALF s TEN u TO
// PAST bu NINE
// ONE SIX THREE
// FOUR FIVE TWO
// EIGHT ELEVEN
// SEVEN TWELVE
// TEN sz O'CLOCK

// All the segments of words on the board. The first too numbers are the
// coordinate of the first letter of the word, the last is the length. A word
// must always be on one row.
#define EN_S_IT 0, 0, 2
#define EN_S_IS 3, 0, 2

#define EN_H_ONE 0, 5, 3
#define EN_H_TWO 8, 6, 3
#define EN_H_THREE 6, 5, 5
#define EN_H_FOUR 0, 6, 4
#define EN_H_FIVE 4, 6, 4
#define EN_H_SIX 3, 5, 3
#define EN_H_SEVEN 0, 8, 5
#define EN_H_EIGHT 0, 7, 5
#define EN_H_NINE 7, 4, 4
#define EN_H_TEN 0, 9, 3
#define EN_H_ELEVEN 5, 7, 6
#define EN_H_TWELVE 5, 8, 6

#define EN_H_AM 7, 0, 2
#define EN_H_PM 9, 0, 2

#define EN_M_A 0, 1, 1
#define EN_M_PAST 0, 4, 4
#define EN_M_TO 9, 3, 2

#define EN_M_TEN 5, 3, 3
#define EN_M_QUARTER 2, 1, 7
#define EN_M_TWENTY 0, 2, 6
#define EN_M_TWENTYFIVE 0, 2, 10
#define EN_M_FIVE 6, 2, 4
#define EN_M_HALF 0, 3, 4
#define EN_M_QUART 0, 9, 5
#define EN_M_QUARTS 0, 9, 6

#define EN_M_OCLOCK 5, 9, 7

EnglishClockFace::EnglishClockFace(LightSensorPosition position) : ClockFace(position)
{
  // fill the array of letters. This is used for finding words in puzzle-mode
  int i=0;
  strncpy(_letters[i++], "ITLISASAMPM", NEOPIXEL_COLUMNS);
  strncpy(_letters[i++], "ACQUARTERDC", NEOPIXEL_COLUMNS);
  strncpy(_letters[i++], "TWENTYFIVEX", NEOPIXEL_COLUMNS);
  strncpy(_letters[i++], "HALFSTENJTO", NEOPIXEL_COLUMNS);
  strncpy(_letters[i++], "PASTEBUNINE", NEOPIXEL_COLUMNS);
  strncpy(_letters[i++], "ONESIXTHREE", NEOPIXEL_COLUMNS);
  strncpy(_letters[i++], "FOURFIVETWO", NEOPIXEL_COLUMNS);
  strncpy(_letters[i++], "EIGHTELEVEN", NEOPIXEL_COLUMNS);
  strncpy(_letters[i++], "SEVENTWELVE", NEOPIXEL_COLUMNS);
  strncpy(_letters[i++], "TENSZOCLOCK", NEOPIXEL_COLUMNS);
}

bool EnglishClockFace::stateForTime(int hour, int minute, int second, bool show_ampm)
{
  if (hour == _hour && minute == _minute && show_ampm == _show_ampm)
  {
    return false;
  }
  _hour = hour;
  _minute = minute;
  _show_ampm = show_ampm;
  Serial.printf("EnglishClockFace::stateForTime() time:%02d:%02d:%02d\n",
                hour, minute, second);

  DLOGLN("update state");

  // Reset the board to all black
  for (int i = 0; i < NEOPIXEL_COUNT; i++)
    _state[i] = false;

  int leftover = minute % 5;
  minute = minute - leftover;

  if (minute >= 35)
    hour = (hour + 1) % 24; // Switch to "TO" minutes the next hour

  updateSegment(EN_S_IT);
  updateSegment(EN_S_IS);

  if (show_ampm)
  {
    if (_hour < 12) // RVG: we use the real clock hour for AM/PM
    {
      updateSegment(EN_H_AM);
    }
    else
    {
      updateSegment(EN_H_PM);
    }
  }

  switch (hour)
  {
  case 0:
    updateSegment(EN_H_TWELVE);
    break;
  case 1:
  case 13:
    updateSegment(EN_H_ONE);
    break;
  case 2:
  case 14:
    updateSegment(EN_H_TWO);
    break;
  case 3:
  case 15:
    updateSegment(EN_H_THREE);
    break;
  case 4:
  case 16:
    updateSegment(EN_H_FOUR);
    break;
  case 5:
  case 17:
    updateSegment(EN_H_FIVE);
    break;
  case 6:
  case 18:
    updateSegment(EN_H_SIX);
    break;
  case 7:
  case 19:
    updateSegment(EN_H_SEVEN);
    break;
  case 8:
  case 20:
    updateSegment(EN_H_EIGHT);
    break;
  case 9:
  case 21:
    updateSegment(EN_H_NINE);
    break;
  case 10:
  case 22:
    updateSegment(EN_H_TEN);
    break;
  case 11:
  case 23:
    updateSegment(EN_H_ELEVEN);
    break;
  case 12:
    updateSegment(EN_H_TWELVE);
    break;
  default:
    DLOG("Invalid hour ");
    DLOGLN(hour);
  }

  switch (minute)
  {
  case 0:
    updateSegment(EN_M_OCLOCK);
    break;
  case 5:
    updateSegment(EN_M_FIVE);
    updateSegment(EN_M_PAST);
    break;
  case 10:
    updateSegment(EN_M_TEN);
    updateSegment(EN_M_PAST);
    break;
  case 15:
    updateSegment(EN_M_A);
    updateSegment(EN_M_QUARTER);
    updateSegment(EN_M_PAST);
    break;
  case 20:
    updateSegment(EN_M_TWENTY);
    updateSegment(EN_M_PAST);
    break;
  case 25:
    updateSegment(EN_M_TWENTYFIVE);
    updateSegment(EN_M_PAST);
    break;
  case 30:
    updateSegment(EN_M_HALF);
    updateSegment(EN_M_PAST);
    break;
  case 35:
    updateSegment(EN_M_TWENTYFIVE);
    updateSegment(EN_M_TO);
    break;
  case 40:
    updateSegment(EN_M_TWENTY);
    updateSegment(EN_M_TO);
    break;
  case 45:
    updateSegment(EN_M_A);
    updateSegment(EN_M_QUARTER);
    updateSegment(EN_M_TO);
    break;
  case 50:
    updateSegment(EN_M_TEN);
    updateSegment(EN_M_TO);
    break;
  case 55:
    updateSegment(EN_M_FIVE);
    updateSegment(EN_M_TO);
    break;
  default:
    DLOG("Invalid minute ");
    DLOGLN(minute);
  }

  switch (leftover)
  {
  case 4:
    _state[mapMinute(TopLeft)] = true;
  case 3: // fall through
    _state[mapMinute(BottomLeft)] = true;
  case 2: // fall through
    _state[mapMinute(BottomRight)] = true;
  case 1: // fall through
    _state[mapMinute(TopRight)] = true;
  case 0: // fall through
    break;
  }
  return true;
}
