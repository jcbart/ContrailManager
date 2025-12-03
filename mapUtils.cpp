#include <cmath>
#include "mapUtils.h"

// Converts a geographic point to a Cartesian point on a unit circle
Cart3D Geo2D_to_Cart3D(Geo2D pointIn) {
    Cart3D pointOut;
    float theta = RAD_PER_DEG * (90. - pointIn.lat);
    float phi = RAD_PER_DEG * pointIn.lon;
    pointOut.x = sin(theta) * cos(phi);
    pointOut.y = sin(theta) * sin(phi);
    pointOut.z = cos(theta);
    // Rescale to reduce precision errors
    float rho = sqrt(std::pow(pointOut.x, 2) + std::pow(pointOut.y, 2)
                     + std::pow(pointOut.z, 2));
    pointOut.x /= rho;
    pointOut.y /= rho;
    pointOut.z /= rho;
    return pointOut;
}

// Converts a Cartesian point on a unit circle to a geographic point
Geo2D Cart3D_to_Geo2D(Cart3D pointIn) {
    Geo2D pointOut;
    float theta = acos(pointIn.z);
    float phi = atan2(pointIn.y, pointIn.x);
    pointOut.lat = 90. - theta/RAD_PER_DEG;
    pointOut.lon = phi/RAD_PER_DEG;
    return pointOut;
}

// Converts a geodetic point to a Cartesian point
Cart3D Geo3D_to_Cart3D(Geo3D pointIn) {
    Cart3D pointOut;
    float theta = RAD_PER_DEG * (90. - pointIn.lat);
    float phi = RAD_PER_DEG * pointIn.lon;
    float rho = EARTH_RADIUS_M + pointIn.alt;
    pointOut.x = rho * sin(theta) * cos(phi);
    pointOut.y = rho * sin(theta) * sin(phi);
    pointOut.z = rho * cos(theta);
    return pointOut;
}

// Converts a Cartesian point to a geodetic point
Geo3D Cart3D_to_Geo3D(Cart3D pointIn) {
    Geo3D pointOut;
    float rho = sqrt(std::pow(pointIn.x, 2) + std::pow(pointIn.y, 2)
                     + std::pow(pointIn.z, 2));
    float theta = acos(pointIn.z/rho);
    float phi = atan2(pointIn.y, pointIn.x);
    pointOut.lat = 90. - theta/RAD_PER_DEG;
    pointOut.lon = phi/RAD_PER_DEG;
    pointOut.alt = rho - EARTH_RADIUS_M;
    return pointOut;
}