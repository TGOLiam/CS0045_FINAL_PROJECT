#pragma once

#include "../helicopter/Helicopter.h"
#include "../input/Input.h"
#include "../renderer/Renderer.h"

class HUD {
public:
  // Samples flight telemetry after simulation. Display values are filtered
  // here; the helicopter simulation state is never modified by the HUD.
  static void update(const Helicopter &helicopter, float groundHeight,
                     float deltaTime);

  // Makes the next flight entry use the live values immediately.
  static void reset();

  static void render(Renderer &renderer, const Helicopter &helicopter,
                     int width, int height, float groundHeight);

  // Returns true if the Main Menu button was clicked.
  static bool handleInput(Input &input, int width, int height);
};
