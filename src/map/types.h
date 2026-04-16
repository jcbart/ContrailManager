#ifndef MAPTYPES_H
#define MAPTYPES_H

#include <string>
#include <sstream>

// Forward declarations
struct Geo2D;
struct Geo3D;

// A structure to define a location in geographic (lon, lat) coordinates
struct Geo2D {
    double lon; // degrees, West is negative
    double lat; // degrees, South is negative

    // Constructor without values
    Geo2D() {}

    // Constructor with values
    Geo2D(double lon, double lat) : lon(lon), lat(lat) {}

    // Set values
    void set(double lon, double lat) {
        this->lon = lon;
        this->lat = lat;
    }

    // Return a Geo3D version of a Geo2D object (alt not set)
    inline operator Geo3D() const;

    // Return location (lon, lat) as string
    std::string asString() const {
        std::stringstream ss;
        ss << "(" << lon << ", " << lat << ")";
        return ss.str();
    }
};

// A structure to define a location in geodetic (lon, lat, alt) coordinates
struct Geo3D {
    double lon; // degrees, West is negative
    double lat; // degrees, South is negative
    double alt; // metres above mean sea level

    // Constructor without values
    Geo3D() {}

    // Constructor with values
    Geo3D(double lon, double lat, double alt) : lon(lon), lat(lat), alt(alt) {}

    // Set values
    void set(double lon, double lat, double alt) {
        this->lon = lon;
        this->lat = lat;
        this->alt = alt;
    }

    // Return a Geo2D version of a Geo3D object (alt stripped)
    inline operator Geo2D() const;

    // Return location (lon, lat, alt) as string
    std::string asString() const {
        std::stringstream ss;
        ss << "(" << lon << ", " << lat << ", " << alt << ")";
        return ss.str();
    }
};

// Return a Geo3D version of a Geo2D object (alt set to 0)
inline Geo2D::operator Geo3D() const {
    return Geo3D(
        this->lon,
        this->lat,
        0
    );
}

// Return a Geo2D version of a Geo3D object (alt stripped)
inline Geo3D::operator Geo2D() const {
    return Geo2D(
        this->lon,
        this->lat
    );
}

// A structure to define a location (or vector) in Cartesian (x, y, z) coordinates
struct Cart3D {
    double x;
    double y;
    double z;

    // Constructor without values
    Cart3D() {}

    // Constructor with values
    Cart3D(double x, double y, double z) : x(x), y(y), z(z) {}

    // Set values
    void set(double x, double y, double z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    // Sum two Cart3D points
    Cart3D operator+(const Cart3D& other) const {
        return Cart3D(
            this->x + other.x,
            this->y + other.y,
            this->z + other.z
        );
    }

    // Subtract one Cart3D point from another
    Cart3D operator-(const Cart3D& other) const {
        return Cart3D(
            this->x - other.x,
            this->y - other.y,
            this->z - other.z
        );
    }
};

// Forward declarations
template <typename dtype>
struct IDX2;
template <typename dtype>
struct IDX3;

// A template structure to store 2 indices of type dtype
template <typename dtype>
struct IDX2 {
    dtype i;
    dtype j;

    // Constructor without values
    IDX2() {}

    // Constructor with values
    IDX2(dtype i, dtype j) : i(i), j(j) {}

    // Set values
    void set(dtype i, dtype j) {
        this->i = i;
        this->j = j;
    }

    // Return an IDX2 object with a different data type
    template <typename dtypeTarget>
    operator IDX2<dtypeTarget>() const {
        return IDX2<dtypeTarget>(
            static_cast<dtypeTarget>(i),
            static_cast<dtypeTarget>(j)
        );
    }

    // Return a IDX3 version of an IDX2 object (k not set)
    template <typename dtypeTarget>
    inline operator IDX3<dtypeTarget>() const;

    // Return location (i, j) as string
    std::string asString() const {
        std::stringstream ss;
        ss << "(" << i << ", " << j << ")";
        return ss.str();
    }
};

// A template structure to store 3 indices of type dtype
template <typename dtype>
struct IDX3 {
    dtype i;
    dtype j;
    dtype k;

    // Constructor without values
    IDX3() {}

    // Constructor with values
    IDX3(dtype i, dtype j, dtype k) : i(i), j(j), k(k) {}

    // Set values
    void set(dtype i, dtype j, dtype k) {
        this->i = i;
        this->j = j;
        this->k = k;
    }

    // Return an IDX3 object with a different data type
    template <typename dtypeTarget>
    operator IDX3<dtypeTarget>() const {
        return IDX3<dtypeTarget>(
            static_cast<dtypeTarget>(i),
            static_cast<dtypeTarget>(j),
            static_cast<dtypeTarget>(k)
        );
    }

    // Return a IDX2 version of an IDX3 object (k stripped)
    template <typename dtypeTarget>
    inline operator IDX2<dtypeTarget>() const;

    // Return location (i, j, k) as string
    std::string asString() const {
        std::stringstream ss;
        ss << "(" << i << ", " << j << ", " << k << ")";
        return ss.str();
    }
};

// Return a IDX3 version of an IDX2 object (k set to 0)
template <typename dtypeSource>
template <typename dtypeTarget>
inline IDX2<dtypeSource>::operator IDX3<dtypeTarget>() const {
    return IDX3<dtypeTarget>(
        static_cast<dtypeTarget>(i),
        static_cast<dtypeTarget>(j),
        static_cast<dtypeTarget>(0)
    );
}

// Return a IDX2 version of an IDX3 object (k stripped)
template <typename dtypeSource>
template <typename dtypeTarget>
inline IDX3<dtypeSource>::operator IDX2<dtypeTarget>() const {
    return IDX2<dtypeTarget>(
        static_cast<dtypeTarget>(i),
        static_cast<dtypeTarget>(j)
    );
}

#endif