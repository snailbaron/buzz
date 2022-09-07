#include "error.hpp"
#include "sdl.hpp"

#include "build-info.hpp"
#include "tempo.hpp"
#include "ve.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

template <class T> struct XYModel {
    T x;
    T y;
};
using XYVector = ve::Vector<XYModel, float>;

struct Hero {
    static constexpr float acceleration = 3000.f;
    static constexpr float deceleration = 2000.f;
    static constexpr float maxSpeed = 500.f;

    XYVector position;
    XYVector velocity;
};

struct KeyboardControllerState {
    bool leftPressed = false;
    bool rightPressed = false;
    bool upPressed = false;
    bool downPressed = false;

    XYVector control() const
    {
        return {1.f * rightPressed - leftPressed, 1.f * upPressed - downPressed};
    }
};

struct HeroSprite {
    const std::vector<SDL_Rect> walkRightFrames {
        {.x = 0, .y = 0, .w = 16, .h = 16},
        {.x = 16, .y = 0, .w = 16, .h = 16},
    };
    static constexpr int scale = 10;
    size_t frame = 0;
    SDL_FRect position {.x = 0, .y = 0, .w = 16 * scale, .h = 16 * scale};
};

int main()
{
    const auto sdlFlags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS;
    sdlCheck(SDL_InitSubSystem(sdlFlags));
    check(IMG_Init(IMG_INIT_PNG) == IMG_INIT_PNG);

    SDL_Window* window = sdlCheck(SDL_CreateWindow(
        "buzz", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, 0));
    SDL_Renderer* renderer = sdlCheck(SDL_CreateRenderer(
        window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));

    SDL_Texture* heroTexture = IMG_LoadTexture(
        renderer, (SOURCE_ROOT / "assets" / "hero.png").c_str());

    auto hero = Hero{};
    auto controller = KeyboardControllerState{};
    auto heroSprite = HeroSprite{};

    auto timer = tempo::FrameTimer{60};
    auto animationMetronome = tempo::Metronome{3};
    bool done = false;
    for (;;) {
        for (auto e = SDL_Event{}; SDL_PollEvent(&e) && !done; ) {
            if (e.type == SDL_QUIT) {
                done = true;
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_q &&
                    e.key.keysym.mod == 0 && e.key.repeat == 0) {
                done = true;
            } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                if (e.key.keysym.sym == SDLK_a) {
                    controller.leftPressed = (e.type == SDL_KEYDOWN);
                } else if (e.key.keysym.sym == SDLK_d) {
                    controller.rightPressed = (e.type == SDL_KEYDOWN);
                } else if (e.key.keysym.sym == SDLK_w) {
                    controller.upPressed = (e.type == SDL_KEYDOWN);
                } else if (e.key.keysym.sym == SDLK_s) {
                    controller.downPressed = (e.type == SDL_KEYDOWN);
                }
            }
        }
        if (done) {
            break;
        }

        if (int framesPassed = timer(); framesPassed > 0) {
            // update world
            for (int i = 0; i < framesPassed; i++) {
                hero.velocity += controller.control() *
                    (hero.acceleration + hero.deceleration) * timer.delta();

                auto speed = length(hero.velocity);
                if (speed > 0) {
                    auto targetSpeed = std::clamp(
                        static_cast<float>(speed - hero.deceleration * timer.delta()),
                        0.f,
                        hero.maxSpeed);
                    hero.velocity *= targetSpeed / speed;
                }

                hero.position += hero.velocity * timer.delta();
            }

            // update graphics
            {
                // hack before events are introduced
                heroSprite.position.x = hero.position.x;
                heroSprite.position.y = -hero.position.y;  // temporary inverse

                int ticks = animationMetronome.ticks(timer.delta());
                heroSprite.frame = (heroSprite.frame + ticks) %
                    heroSprite.walkRightFrames.size();
            }

            // present graphics
            sdlCheck(SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255));
            sdlCheck(SDL_RenderClear(renderer));

            sdlCheck(SDL_RenderCopyF(
                renderer,
                heroTexture,
                &heroSprite.walkRightFrames.at(heroSprite.frame),
                &heroSprite.position));
            SDL_RenderPresent(renderer);
        }
    }

    SDL_DestroyTexture(heroTexture);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    IMG_Quit();
    SDL_QuitSubSystem(sdlFlags);
}
