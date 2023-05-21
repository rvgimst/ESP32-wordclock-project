#pragma once

#include <NeoPixelAnimator.h>
#include <NeoPixelBrightnessBus.h>

#include "BrightnessController.h"
#include "ClockFace.h"
#include "Clockmodes.h"

// The pin to control the matrix
#define NEOPIXEL_PIN 32

//
#define TIME_CHANGE_ANIMATION_SPEED 400

class Display
{
public:
  Display(ClockFace &clockFace, uint8_t pin = NEOPIXEL_PIN);

  void setup();
  void loop();
  void setColor(const RgbColor &color);

  // Sets the sensor sensitivity of the brightness controller.
  void setSensorSensitivity(int value) { _brightnessController.setSensorSensitivity(value); }

  // Sets whether to show AM/PM information on the display.
  void setShowAmPm(bool show_ampm) { _show_ampm = show_ampm; }

  // Sets the clock mode.
  void setClockMode(ClockMode mode) { clock_mode_ = mode; }

  // Starts an animation to update the clock to a new time if necessary.
  void updateWithTime(int hour, int minute, int second, int animationSpeed = TIME_CHANGE_ANIMATION_SPEED);

private:
  // Updates pixel color on the display.
  void _update(int animationSpeed = TIME_CHANGE_ANIMATION_SPEED, bool fadeToBlack = false);

  // Update the clock with realtime clock data, using updateWithTime()
  void _updateClockRealTime();

  // To know which pixels to turn on and off, one needs to know which letter
  // matches which LED, and the orientation of the display. This is the job
  // of the clockFace.
  ClockFace &_clockFace;

  // Whether the display should show AM/PM information.
  bool _show_ampm = 1;

  // Clock display mode.
  ClockMode clock_mode_ = ClockMode::REAL_TIME;

  // Color of the LEDs. Can be manipulated via Web configuration interface.
  RgbColor _color;

  // Addressable bus to control the LEDs.
  NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> _pixels;

  // Reacts to change in ambient light to adapt the power of the LEDs
  BrightnessController _brightnessController;

  // Animation time management object.
  // Uses centiseconds as precision, so an animation can range from 1/100 of a
  // second to a little bit more than 10 minutes.
  NeoPixelAnimator _animations;

  //======================================
  // Color test constants, variables and functions

  const uint8_t POPULATION_THRESHOLD = 60; // delay [ms] between new pixel animations
  uint8_t gHue = 0;
  unsigned long t_lastPixelStart = 0;
  bool oneTimeUpdate = true;

  // what is stored for state is specific to the need, in this case, the colors.
  // basically what ever you need inside the animation update function
  struct MyAnimationState
  {
      RgbColor StartingColor;
      RgbColor EndingColor;
  };

  // one entry per pixel to match the animation timing manager
  MyAnimationState *animationState;

  //void _blendAnimUpdate(const AnimationParam& param);
  void _blendAnimUpdate(const AnimationParam &param);
  void _colorTestPixelStart(float luminance = 0.2f);
  void _colorTestAnimatePixel(uint16_t pixel, float luminance);
  void _colorTestLoop();
};
