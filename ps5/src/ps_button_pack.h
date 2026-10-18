// DBZ Budokai 3 PS5 - the PlayStation buttons.
//
// <app>/ps_buttons/pack is a texture pack (made by ps5/tools/ps_buttons.py,
// ps_prompts.py and ps_controller.py) with the game's Xbox button pictures
// drawn as PlayStation ones: the LB LT RB RT Y B A X strip, the START button,
// the controller picture, and the BACK button (the touch pad). With the hub's
// Button Prompts on PlayStation it goes first in the game's pack list (a
// small pack wins over a big one, so over an HD pack too).
#pragma once

#include <cstdint>
#include <vector>

namespace budokai3 {

// At start (OnPostSetup, before the GPU reads the pack list): makes the BACK
// picture and adds the pack to the game's list, when the prompts are
// PlayStation. The BACK texture also carries the game's words ("BACK : Help"),
// which don't ship with the app: it's made here from the game's own texture
// (or an installed HD pack's), with the touch pad (ps_buttons/touch_pad.png)
// where the Xbox button was.
void PrepareButtonPack();

// For NovaPowers' own screens (game_style.cpp Img): the PlayStation picture
// for the game piece `name` ("botones", "boton_start", "boton_back"), its
// pixels at the pack's size and the size of the game's (the screens cut it
// with the game's coordinates). False with Xbox prompts or no picture.
bool PsButtonPicture(const char* name, std::vector<uint8_t>& rgba, int& width, int& height, float& game_width,
                     float& game_height);

}  // namespace budokai3
