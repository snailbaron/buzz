#pragma once

#include "error.hpp"

#include <SDL.h>
#include <SDL_image.h>

#include <utility>

void sdlCheck(int code, std::source_location location = std::source_location::current());

template <class T>
T* sdlCheck(T* ptr, std::source_location location = std::source_location::current())
{
    if (!ptr) {
        throw Error{std::move(location)} << SDL_GetError();
    }
    return ptr;
}
