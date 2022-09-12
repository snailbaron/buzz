#pragma once

#include "ve.hpp"

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
