#pragma once

#include "types.hpp"

#include "evening.hpp"
#include "thing.hpp"

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

struct Control {
    XYVector control;
};

class World : public evening::Subscriber {
public:
    World(evening::Channel& worldEvents, evening::Channel& controlEvents);

    void initTestLevel();

    void update(double delta);

    thing::EntityManager _ecs;
private:
    evening::Channel& _worldEvents;
    Control* _control = nullptr;
};
