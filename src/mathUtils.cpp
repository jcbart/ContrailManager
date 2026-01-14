#include <cmath>
#include "mathUtils.h"

using namespace mathUtils;

// List of types to compile
template float mathUtils::r_to_v<float>(float);
template double mathUtils::r_to_v<double>(double);
template float mathUtils::v_to_r<float>(float);
template double mathUtils::v_to_r<double>(double);

// Returns the volume of a sphere if given its radius
template <typename T>
T mathUtils::r_to_v(const T r) {
    return (4./3. * PI * r*r*r);
}

// Returns the radius of a sphere if given its volume
template <typename T>
T mathUtils::v_to_r(const T v) {
    return std::pow(3.*v / (4.*PI), 1./3.);
}