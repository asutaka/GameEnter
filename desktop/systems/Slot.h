#pragma once
#include <string>
#include <SDL2/SDL.h>

namespace nes {

struct Slot {
    std::string rom_path;
    std::string name;
    std::string cover_path;
    bool occupied = false;
    SDL_Texture* cover_texture = nullptr;
};

}
