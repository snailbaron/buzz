#pragma once

#include "types.hpp"

#include "evening.hpp"
#include "thing.hpp"
#include "ve.hpp"

enum ObjectType {
    Hero,
    Tree,
};

struct Spawn {
    thing::Entity entity;
    ObjectType objectType;
    XYVector location;
};

struct Move {
    thing::Entity entity;
    XYVector location;
    XYVector velocity;
};
