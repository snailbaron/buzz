#include "sdl.hpp"

void sdlCheck(int code, std::source_location location)
{
    if (code != 0) {
        throw Error{std::move(location)} << SDL_GetError();
    }
}
