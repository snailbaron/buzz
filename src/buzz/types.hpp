#pragma once

#include "ve.hpp"

template <class T> struct XYModel {
    T x;
    T y;
};

using XYVector = ve::Vector<XYModel, float>;
using ScreenVector = ve::Vector<XYModel, int>;
