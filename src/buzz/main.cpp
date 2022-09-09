#include "error.hpp"
#include "sdl.hpp"

#include "build-info.hpp"
#include "tempo.hpp"
#include "thing.hpp"
#include "ve.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <tuple>
#include <vector>

template <class T> struct XYModel {
    T x;
    T y;
};
using XYVector = ve::Vector<XYModel, float>;

struct WorldLocation {
    XYVector position;
    float radius = 0.f;
};

struct WorldMovement {
    XYVector velocity;
    float maxSpeed = 0.f;
    float accelerationTime = 0.f;
    float decelerationTime = 0.f;
};

struct Control {};

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

enum class MoveDirection {
    Down,
    Up,
    Left,
    Right,
};

enum class MoveSpeed {
    Stand,
    Walk,
};

struct DirectionalSprite {
    DirectionalSprite(SDL_Texture* texture)
        : texture(texture)
    {
        for (auto moveSpeed : {MoveSpeed::Walk, MoveSpeed::Stand}) {
            for (auto moveDirection : {
                    MoveDirection::Right,
                    MoveDirection::Left,
                    MoveDirection::Down,
                    MoveDirection::Up}) {
                int i = frames.size();
                frames.emplace(
                    std::tuple<MoveDirection, MoveSpeed>{moveDirection, moveSpeed},
                    std::vector<SDL_Rect>
                    {
                        SDL_Rect{.x = 0, .y = 16 * i, .w = 16, .h = 16},
                        SDL_Rect{.x = 16, .y = 16 * i, .w = 16, .h = 16},
                    });
            }
        }
    }

    const size_t frameNumber = 2;
    static constexpr int scale = 10;
    static constexpr float minWalkSpeed = 50.f;

    SDL_Texture* texture = nullptr;
    std::map<std::tuple<MoveDirection, MoveSpeed>, std::vector<SDL_Rect>> frames;

    MoveDirection direction = MoveDirection::Down;
    size_t frame = 0;
    SDL_FRect position {.x = 0, .y = 0, .w = 16 * scale, .h = 16 * scale};
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

    auto controller = KeyboardControllerState{};

    const auto sdlFlags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS;
    sdlCheck(SDL_InitSubSystem(sdlFlags));
    check(IMG_Init(IMG_INIT_PNG) == IMG_INIT_PNG);

    SDL_Window* window = sdlCheck(SDL_CreateWindow(
        "buzz", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, 0));
    SDL_Renderer* renderer = sdlCheck(SDL_CreateRenderer(
        window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));

    SDL_Texture* heroTexture = IMG_LoadTexture(
        renderer, (SOURCE_ROOT / "assets" / "hero.png").c_str());
    SDL_Texture* treeTexture = IMG_LoadTexture(
        renderer, (SOURCE_ROOT / "assets"/ "tree.png").c_str());

    auto heroSprite = DirectionalSprite{heroTexture};

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

            // update and present graphics
            {
                const auto& heroPosition = ecs.component<WorldLocation>(hero).position;
                heroSprite.position.x = heroPosition.x - 80;
                heroSprite.position.y = -heroPosition.y + 80;

                const auto& heroVelocity = ecs.component<WorldMovement>(hero).velocity;
                auto heroSpeed = length(heroVelocity);
                if (heroSpeed > 0) {
                    if (1.5 * std::abs(heroVelocity.x) > std::abs(heroVelocity.y)) {
                        if (heroVelocity.x > 0) {
                            heroSprite.direction = MoveDirection::Right;
                        } else {
                            heroSprite.direction = MoveDirection::Left;
                        }
                    } else {
                        if (heroVelocity.y > 0) {
                            heroSprite.direction = MoveDirection::Up;
                        } else {
                            heroSprite.direction = MoveDirection::Down;
                        }
                    }
                }

                auto moveSpeed = heroSpeed < heroSprite.minWalkSpeed ?
                    MoveSpeed::Stand : MoveSpeed::Walk;

                auto frames = heroSprite.frames.at({heroSprite.direction, moveSpeed});
                int ticks = animationMetronome.ticks(timer.delta());
                heroSprite.frame =
                    (heroSprite.frame + ticks) % heroSprite.frameNumber;
                const auto& currentFrame = frames.at(heroSprite.frame);

                sdlCheck(SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255));
                sdlCheck(SDL_RenderClear(renderer));

                for (const auto& tree : {tree1, tree2}) {
                    const auto& treePosition = ecs.component<WorldLocation>(tree).position;
                    float scale = 10.f;
                    auto position = SDL_FRect {
                        .x = treePosition.x - 4 * scale,
                        .y = -treePosition.y + 8 * scale,
                        .w = 8 * scale,
                        .h = 16 * scale};
                    sdlCheck(SDL_RenderCopyF(
                        renderer,
                        treeTexture,
                        nullptr,
                        &position));
                }

                sdlCheck(SDL_RenderCopyF(
                    renderer,
                    heroTexture,
                    &currentFrame,
                    &heroSprite.position));

                SDL_RenderPresent(renderer);
            }
        }
    }

    SDL_DestroyTexture(treeTexture);
    SDL_DestroyTexture(heroTexture);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    IMG_Quit();
    SDL_QuitSubSystem(sdlFlags);
}
