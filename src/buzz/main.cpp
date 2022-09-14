#include "error.hpp"
#include "events.hpp"
#include "scene.hpp"
#include "sdl.hpp"
#include "world.hpp"

#include "build-info.hpp"
#include "tempo.hpp"
#include "thing.hpp"
#include "ve.hpp"

#include <algorithm>
#include <map>
#include <tuple>
#include <vector>

struct KeyboardControllerState {
    bool leftPressed = false;
    bool rightPressed = false;
    bool upPressed = false;
    bool downPressed = false;

    XYVector control() const
    {
        return XYVector{
            1.f * rightPressed - leftPressed,
            1.f * upPressed - downPressed};
    }
};

int main()
{
    evening::Channel worldEvents;
    evening::Channel controlEvents;

    auto world = World{worldEvents, controlEvents};
    auto view = View{worldEvents};

    world.initTestLevel();

    auto controller = KeyboardControllerState{};

    auto timer = tempo::FrameTimer{60};
    for (;;) {
        bool done = false;
        for (auto e = SDL_Event{}; SDL_PollEvent(&e) && !done; ) {
            if (e.type == SDL_QUIT) {
                done = true;
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_q &&
                    (e.key.keysym.mod & (KMOD_CTRL | KMOD_ALT | KMOD_SHIFT)) == 0 &&
                    e.key.repeat == 0) {
                done = true;
            } else if ((e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) &&
                    e.key.repeat == 0) {

                if (e.key.keysym.sym == SDLK_a) {
                    controller.leftPressed = (e.type == SDL_KEYDOWN);
                } else if (e.key.keysym.sym == SDLK_d) {
                    controller.rightPressed = (e.type == SDL_KEYDOWN);
                } else if (e.key.keysym.sym == SDLK_w) {
                    controller.upPressed = (e.type == SDL_KEYDOWN);
                } else if (e.key.keysym.sym == SDLK_s) {
                    controller.downPressed = (e.type == SDL_KEYDOWN);
                }

                controlEvents.push(controller.control());
            }
        }
        if (done) {
            break;
        }

        controlEvents.deliver();

        if (int framesPassed = timer(); framesPassed > 0) {
            for (int i = 0; i < framesPassed; i++) {
                world.update(timer.delta());
            }

            worldEvents.deliver();

            view.update(framesPassed * timer.delta());
            view.present();
        }
    }
}
