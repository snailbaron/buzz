#include "error.hpp"
#include "scene.hpp"
#include "sdl.hpp"
#include "world.hpp"

#include "build-info.hpp"
#include "tempo.hpp"
#include "thing.hpp"
#include "ve.hpp"

#include <algorithm>
#include <iostream>
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
        return {1.f * rightPressed - leftPressed, 1.f * upPressed - downPressed};
    }
};

int main()
{
    auto ecs = thing::EntityManager{};

    auto hero = ecs.createEntity();
    {
        ecs.add(hero, WorldLocation{
            .position = {0, 0},
            .radius = 50.f,
        });
        ecs.add(hero, WorldMovement{
            .velocity = {0, 0},
            .maxSpeed = 500.f,
            .accelerationTime = 0.2f,
            .decelerationTime = 0.2f,
        });
        ecs.add(hero, Control{});
    }

    auto tree1 = ecs.createEntity();
    {
        ecs.add(tree1, WorldLocation{
            .position = {600.f, -100.f},
            .radius = 50,
        });
    }

    auto tree2 = ecs.createEntity();
    {
        ecs.add(tree2, WorldLocation{
            .position = {200.f, -300.f},
            .radius = 50,
        });
    }

    auto view = View{};
    view.addHero(hero);
    view.addTree(tree1);
    view.addTree(tree2);

    auto controller = KeyboardControllerState{};

    auto camera = Camera{};

    auto timer = tempo::FrameTimer{60};
    auto animationMetronome = tempo::Metronome{3};
    bool done = false;
    for (;;) {
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
            }
        }
        if (done) {
            break;
        }

        if (int framesPassed = timer(); framesPassed > 0) {
            // update world
            const double delta = timer.delta();

            for (int i = 0; i < framesPassed; i++) {
                auto acceleration = [] (const WorldMovement& m) {
                    return m.maxSpeed / m.accelerationTime;
                };
                auto deceleration = [] (const WorldMovement& m) {
                    return m.maxSpeed / m.decelerationTime;
                };

                // update acceleration on controlled objects
                //
                for (auto& e : ecs.entities<Control>()) {
                    auto& movement = ecs.component<WorldMovement>(e);
                    movement.velocity += controller.control() *
                        (acceleration(movement) + deceleration(movement)) * delta;
                }

                // update moving object positions
                for (auto& e : ecs.entities<WorldMovement>()) {
                    auto& location = ecs.component<WorldLocation>(e);
                    auto& movement = ecs.component<WorldMovement>(e);

                    auto speed = length(movement.velocity);
                    if (speed > 0) {
                        auto targetSpeed = std::clamp(
                            static_cast<float>(speed - deceleration(movement) * delta),
                            0.f,
                            movement.maxSpeed);
                        movement.velocity *= targetSpeed / speed;
                    }

                    location.position += movement.velocity * delta;
                }

                // check for collisions with a stupid loop
                for (auto& movingEntity : ecs.entities<WorldMovement>()) {
                    auto& movingEntityLocation = ecs.component<WorldLocation>(movingEntity);
                    for (const auto& someObjectLocation : ecs.components<WorldLocation>()) {
                        if (&movingEntityLocation == &someObjectLocation) {
                            continue;
                        }

                        auto oof = movingEntityLocation.radius +
                            someObjectLocation.radius -
                            length(movingEntityLocation.position -
                                someObjectLocation.position);
                        if (oof > 0) {
                            movingEntityLocation.position +=
                                oof * unit(movingEntityLocation.position -
                                    someObjectLocation.position);
                        }
                    }
                }
            }

            view.update(ecs, framesPassed * delta);
            view.present();
        }
    }
}
