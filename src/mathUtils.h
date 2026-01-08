#ifndef MATHUTILS_H
#define MATHUTILS_H

namespace mathUtils {

const double PI = 3.14159265358979323846264338327950288419716939937510582;
const double RAD_PER_DEG = PI/180;
const double DEG_PER_RAD = 1/RAD_PER_DEG;

template <typename T>
T r_to_v(const T r);

template <typename T>
T v_to_r(const T v);

}

#endif