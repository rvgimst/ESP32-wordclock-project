#include "logging.h"

#include "Display.h"

Display::Display(ClockFace &clockFace, uint8_t pin)
    : _clockFace(clockFace),
      _pixels(ClockFace::pixelCount(), pin),
      _animations(ClockFace::pixelCount(), NEO_CENTISECONDS),
      _wordPixelsLen(0),
      _wordPixelsIdx(0) {
  animationState = new MyAnimationState[ClockFace::pixelCount()];
}

void Display::setup()
{
  _pixels.Begin();
  _brightnessController.setup();
}

void Display::loop()
{
  switch(clock_mode_) {
    case ClockMode::COLOR_TEST:
      _colorTestLoop();
      break;
    case ClockMode::PUZZLE_MODE:
      _puzzleModeLoop();
      break;
    case ClockMode::REAL_TIME:
    default:
      _updateClockRealTime();
      break;
  }

  // for all modes not using one_time initialization, we set one_time back to true
  if (clock_mode_ != ClockMode::COLOR_TEST && clock_mode_ != ClockMode::PUZZLE_MODE)
    firstTimeUpdate = true;
}

void Display::setColor(const RgbColor &color)
{
  DLOGLN("Updating color");
  Serial.printf("Display::setColor(%d,%d,%d), current=(%d,%d,%d)\n", color.R, color.G, color.B, _color.R, _color.G, _color.B);
  if (_color != color) {
    _color = color;
    _brightnessController.setOriginalColor(color);
    _update(60);
  }
}

void Display::setFindWord(char *value, int len) {
  // strncpy(_findWord, value, len);
  _findWord = String(value);
}

void Display::updateWithTime(int hour, int minute, int second, int animationSpeed)
{
  //Serial.printf("=>Display::updateWithTime(%d,%d,%d,%d)\n", hour, minute, second, animationSpeed);

  if (_clockFace.stateForTime(hour, minute, second, _show_ampm))
  {
    //  Serial.printf("Display::updateForTime() time:%02d:%02d:%02d\n", hour, minute, second);

    DLOG("Time: ");
    DLOG(hour);
    DLOG(":");
    DLOGLN(minute);

    _update(animationSpeed);
  }
}

void Display::_updateClockRealTime() {
  //struct tm: tm_year, tm_mon, tm_mday, tm_hour, tm_min, tm_sec
  struct tm timeinfo;

  //Serial.printf("=>Display::_updateClockRealTime()\n");

  // NOTE: the second arg in getLocalTime() is the max [ms] the function waits 
  //       for the time to be received from the NTP server. Be aware to set it to
  //       a low value to prevent blocking issues!
  //       See the following resources:
  // https://github.com/espressif/arduino-esp32/blob/34125cee1d1c6a81cd1da07de27ce69d851f9389/cores/esp32/esp32-hal-time.c#L87
  // https://techtutorialsx.com/2021/09/01/esp32-system-time-and-sntp/
  // https://www.lucadentella.it/en/2017/05/11/esp32-17-sntp/
  getLocalTime(&timeinfo, 10); 
  updateWithTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  // now check if the brightness has changed and if so, 
  // animate the change in brightness
  _brightnessController.loop();
  _animations.UpdateAnimations();
  if (_brightnessController.hasChanged())
  {
    // RVG for later: we only need to update according to brightness if 
    // the previous call to _clockFace.stateForTime() was false (no change in time value)
    _update(30); // Update in 300 ms
  }
  _pixels.Show();
}

void Display::_update(int animationSpeed, bool fadeToBlack)
{
  Serial.printf("=>Display::_update(%d,%d)\n", animationSpeed, fadeToBlack);

  _animations.StopAll();
  static const RgbColor black = RgbColor(0x00, 0x00, 0x00);

  // For all the LED animate a change from the current visible state to the new
  // one.
  const std::vector<bool> &state = _clockFace.getState();
  for (int index = 0; index < state.size(); index++)
  {
    RgbColor originalColor = _pixels.GetPixelColor(index);
    RgbColor targetColor = (state[index] && !fadeToBlack) ? _brightnessController.getCorrectedColor() : black;

    AnimUpdateCallback animUpdate = [=](const AnimationParam &param) {
      float progress = NeoEase::QuadraticIn(param.progress);
      RgbColor updatedColor = RgbColor::LinearBlend(
          originalColor, targetColor, progress);
      _pixels.SetPixelColor(index, updatedColor);
    };
    _animations.StartAnimation(index, animationSpeed, animUpdate);
  }
}

//======== Color test animation functions ========

// pick a new pixel and start its animation
// Param: float luminance (0..1)
//        0.0 = black, 0.25 is normal, 0.5 is bright
void Display::_colorTestPixelStart(float luminance) {
  // The 4 corner pixels should always be animated (to support matching the
  //  faceplate corner holes over the corner LEDs)
  for (int i = 0; i < 4; i++) {
    if (!_animations.IsAnimationActive(i)) {
      _colorTestAnimatePixel(i, luminance);
    }
  }

  // pick a random pixel from the rest of the pixels
  uint16_t pixel = random(_clockFace.pixelCount() - 4) + 4;

  if (!_animations.IsAnimationActive(pixel)) {
    _colorTestAnimatePixel(pixel, luminance);
  }
}

// pick random time and next Hue color to animate the given pixel
void Display::_colorTestAnimatePixel(uint16_t pixel, float luminance) {
  // we use HslColor object as it allows us to easily pick a color
  // with the same saturation and luminance 
  //  leds[pos] += CHSV( gHue + random8(64), 200, 255);

  RgbColor startingRgbColor = RgbColor(HslColor(gHue++ / 255.0f, 1.0f, luminance));
  // set this color as the original color in the BrightnessController
  _brightnessController.setOriginalColor(startingRgbColor);
  // update brightness according to lightsensor value
  _brightnessController.loop();
  //  set that as the startingColor
  _pixels.SetPixelColor(pixel, _brightnessController.getCorrectedColor());
  _pixels.Show();

  // fade to black
  uint16_t time = random(170, 210); // time in centiseconds!! See Display::Display()
  animationState[pixel].StartingColor = _pixels.GetPixelColor(pixel);
  animationState[pixel].EndingColor = RgbColor(0);

  AnimUpdateCallback blendAnimUpdate = [=](const AnimationParam &param) {
    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation
    RgbColor updatedColor = RgbColor::LinearBlend(
        animationState[param.index].StartingColor,
        animationState[param.index].EndingColor,
        param.progress);

    // apply the color to the pixel
    _pixels.SetPixelColor(param.index, updatedColor);
  };

  _animations.StartAnimation(pixel, time, blendAnimUpdate);
}

void Display::_colorTestLoop() {
  if (firstTimeUpdate) {
    // call _update() but fade all pixels to black
    _update(40, true);
    firstTimeUpdate = false;
  }

  if (millis() - t_lastPixelStart > POPULATION_THRESHOLD) {
      _colorTestPixelStart(0.4f);
      t_lastPixelStart = millis();
  }

  if (_animations.IsAnimating())
  {
      // the normal loop just needs these two to run the active animations
      _animations.UpdateAnimations();
      _pixels.Show();
  }
}

//======== Puzzle mode functions ========

// Given a pixel, fade it in to the current color
void Display::_puzzleModeAnimatePixel(uint16_t pixel, int animationSpeed) {
  // update brightness according to lightsensor value
  _brightnessController.loop();

  RgbColor originalColor = _pixels.GetPixelColor(pixel);
  RgbColor targetColor = _brightnessController.getCorrectedColor();

  AnimUpdateCallback blendAnimUpdate = [=](const AnimationParam &param) {
    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation
    float progress = NeoEase::CubicOut(param.progress);
    RgbColor updatedColor = RgbColor::LinearBlend(
        originalColor, targetColor, progress);

    // apply the color to the pixel
    _pixels.SetPixelColor(param.index, updatedColor);
  };

  _animations.StartAnimation(pixel, animationSpeed, blendAnimUpdate);
}

// main loop for puzzle mode. Take either the word from the configuration portal or from Serial input
// and tell the ClockFace to search for the word in the clock board and if found, show it.
void Display::_puzzleModeLoop() {
  static String str;
  static PuzzleState puzzleState = PUZZLE_F2B;

  if (firstTimeUpdate) {
    puzzleState = PUZZLE_F2B;
    firstTimeUpdate = false;
  }

  //Serial.printf("_puzzleModeLoop() puzzleState=(%d)\n", puzzleState);

  if (puzzleState == PUZZLE_F2B) {
    // call _update() but fade all pixels to black
    _update(PUZZLE_DURATION_F2B, true); // create animations to fade board to black
    puzzleState = PUZZLE_IDLE_AFTER_F2B;
  }

  // this block is to make sure that animations complete (incl fade to black) before continuing
  if (_animations.IsAnimating()) {
    // the normal loop just needs these two to run the active animations
    _animations.UpdateAnimations();
    _pixels.Show();
    t_lastAnimation = millis();
    return; // no other action until animations complete
  }
  
  // This block makes sure we wait for the right duration after an animation is finished
  // we use the puzzle state to control this
  if (puzzleState == PUZZLE_IDLE_AFTER_F2B) {
    //Serial.printf("_puzzleModeLoop() millis()=(%d) t_lastAnimation=(%d)\n", millis(), t_lastAnimation);
    if (millis() - t_lastAnimation > PUZZLE_DELAY_AFTER_F2B) { // timeout
      puzzleState = PUZZLE_ANIMATELETTER;
    }
    else { // we wait till delay AFTER F2B is done
      return;
    }
  }

  if (puzzleState == PUZZLE_IDLE_BEFORE_F2B) {
    if (millis() - t_lastAnimation > PUZZLE_DELAY_BEFORE_F2B) { // timeout
      puzzleState = PUZZLE_F2B;
      return;
    }
    else { // we wait till delay BEFORE F2B is done
      return;
    }
  }

  // Now check if there are any letters that need to be faded in.
  // Every letter is shown one by one with a delay between each letter
  if (puzzleState == PUZZLE_ANIMATELETTER && _wordPixelsLen > 0) {
    if (_wordPixelsIdx < _wordPixelsLen) { // still letters waiting
      _puzzleModeAnimatePixel(_wordPixels[_wordPixelsIdx], PUZZLE_DURATION_LETTER);
      Serial.printf("  Animating (%c)\n", str[_wordPixelsIdx]);
      _wordPixelsIdx++;
    }
    else { // we're at the end, reset the word
      _wordPixelsLen = 0;
      _wordPixelsIdx = 0;
      str = String();
      // puzzleState = PUZZLE_IDLE_BEFORE_F2B;
    }
    return; // no other action until we're done with the letters
  }

  // this block is to determine the search string/word
  if (!_findWord.equals("")) { // the internal parameter _findWord is set from the webinterface
    str = _findWord;
    _findWord = String(); // empty _findWord to prevent looping
  } else { // Check Serial line
    if (Serial.available()) { // data incoming on the serial input
      // NOTE on readString using Serial !!
      // using VSCode's serial monitor imposes different behavior than when using the ArduinoIDEs serial monitor
      // ArduinoIDE allows you to set linefeed(\r) and carriage return(\n) separately.
      // When using ArduinoIDE serial monitor, set to "No line ending" for code below to work correclty
      // The VSC monitor sends \r\n as part of the string to the code, which add 2 characters to the string
      // We solve this by trimming the string.
      str = Serial.readString(); // Serial.readString() or Serial.readStringUntil('\n');
    }
  }

  //str = String("escape"); // for testing
  str.trim();
  str.toUpperCase();
  if (str.length() > 0) {
    Serial.printf("Display::_puzzleModeLoop() str=(%s) len=%d\n", str, str.length());

    if (_clockFace.determineLetterSequence(str, _wordPixels, &_wordPixelsLen)) {
      puzzleState = PUZZLE_F2B; // fade all pixels to black
      // NOTE: we don't do anything with _wordPixels here. After fade2black, the letters are processed

      // for now, animate lighting up all letters together in found soluton
      // for (int i = 0; i < str.length(); i++) {
      //   _puzzleModeAnimatePixel(_wordPixels[i], 50);
      // }
    }
    else { // add animations to light up corner pixels to indicate NOT FOUND
      _update(40, true); // first set all pixels to fade to black...
      for (int i = 0; i < 4; i++) {
        _puzzleModeAnimatePixel(i, 30); // ...overwrite animations for corners to fade in
      }
    }
  }
}