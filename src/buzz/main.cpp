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

int main()
{
    evening::Channel worldEvents;
    evening::Channel controlEvents;

    auto world = World{worldEvents, controlEvents};
    auto view = View{worldEvents, controlEvents};

    world.initTestLevel();

    auto timer = tempo::FrameTimer{60};
    for (;;) {
        if (!view.processEvents()) {
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
