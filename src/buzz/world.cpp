#include "world.hpp"

#include "events.hpp"

#include <iostream>

World::World(evening::Channel& worldEvents, evening::Channel& controlEvents)
    : _worldEvents(worldEvents)
{
    subscribe<XYVector>(controlEvents, [this] (const auto& control) {
        if (_control) {
            _control->control = control;
            if (length(_control->control) > 1) {
                _control->control = unit(_control->control);
            }
        }
    });
}

void World::initTestLevel()
{
    {
        auto hero = _ecs.createEntity();
        const auto& location = _ecs.add(hero, WorldLocation{
            .position = {0, 0},
            .radius = 1.f,
        });
        _ecs.add(hero, WorldMovement{
            .velocity = {0, 0},
            .maxSpeed = 8.f,
            .accelerationTime = 0.2f,
            .decelerationTime = 0.2f,
        });
        _control = &_ecs.add(hero, Control{});
        _worldEvents.push(Spawn{
            .entity = hero,
            .objectType = ObjectType::Hero,
            .location = location.position,
        });
    }

    {
        auto tree1 = _ecs.createEntity();
        const auto& location = _ecs.add(tree1, WorldLocation{
            .position = {1.f, -1.f},
            .radius = 1,
        });
        _worldEvents.push(Spawn{
            .entity = tree1,
            .objectType = ObjectType::Tree,
            .location = location.position,
        });
    }

    {
        auto tree2 = _ecs.createEntity();
        const auto& location = _ecs.add(tree2, WorldLocation{
            .position = {2.f, 3.f},
            .radius = 1,
        });
        _worldEvents.push(Spawn{
            .entity = tree2,
            .objectType = ObjectType::Tree,
            .location = location.position,
        });
    }
}

void World::update(double delta)
{
    auto acceleration = [] (const WorldMovement& m) {
        return m.maxSpeed / m.accelerationTime;
    };
    auto deceleration = [] (const WorldMovement& m) {
        return m.maxSpeed / m.decelerationTime;
    };

    for (auto& e : _ecs.entities<Control>()) {
        const auto& control = _ecs.component<Control>(e).control;
        auto& movement = _ecs.component<WorldMovement>(e);
        movement.velocity += control *
            (acceleration(movement) + deceleration(movement)) * delta;
    }

    // update moving object positions
    for (auto& e : _ecs.entities<WorldMovement>()) {
        auto& location = _ecs.component<WorldLocation>(e);
        auto& movement = _ecs.component<WorldMovement>(e);

        auto speed = length(movement.velocity);
        if (speed > 0) {
            auto targetSpeed = std::clamp(
                static_cast<float>(speed - deceleration(movement) * delta),
                0.f,
                movement.maxSpeed);
            movement.velocity *= targetSpeed / speed;
        }

        location.position += movement.velocity * delta;

        _worldEvents.push(Move{
            .entity = e,
            .location = location.position,
            .velocity = movement.velocity});
    }

    // check for collisions with a stupid loop
    for (auto& movingEntity : _ecs.entities<WorldMovement>()) {
        auto& movingEntityLocation = _ecs.component<WorldLocation>(movingEntity);
        for (const auto& someObjectLocation : _ecs.components<WorldLocation>()) {
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
