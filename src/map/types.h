#ifndef MAPTYPES_H
#define MAPTYPES_H

#include <cmath>
#include <array>
#include <string>
#include <format>
#include <concepts>

// Forward declarations
struct Geo2D;
struct Geo3D;

// A structure to define a location in geographic (lon, lat) coordinates
struct Geo2D {
    double lon; // (degrees), West is negative
    double lat; // (degrees), South is negative

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
        return std::format("({}, {})", lon, lat);
    }
};

// A structure to define a location in geodetic (lon, lat, alt) coordinates
struct Geo3D {
    double lon; // (degrees), West is negative
    double lat; // (degrees), South is negative
    // altitude (m) above mean sea level
    // For Flight, this is pressure altitude
    // For anything else, this is geopotential height
    double alt;

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
        return std::format("({}, {}, {})", lon, lat, alt);
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

// A template structure to store N indices of type T
template <size_t N, typename T>
requires std::same_as<T, int> || std::same_as<T, float> || std::same_as<T, double>
struct IDX {
    std::array<T, N> vals{};

    // Constructor without values
    IDX() = default;

    // Constructor from lvalue array
    explicit IDX(const std::array<T, N>& arr) : vals(arr) {}

    // Constructor from rvalue array
    explicit IDX(std::array<T, N>&& arr) : vals(std::move(arr)) {}

    // Variadic constructor from list of values, e.g. IDX<3, int> ijk{1, 2, 3} 
    template <typename... U>
    requires (sizeof...(U) == N) && (std::same_as<std::remove_cvref_t<U>, T> &&...)
    IDX(U&&... u) : vals{ { std::forward<U>(u)... } } {}

    // Set values from lvalue array
    void set(const std::array<T, N>& arr) {
        vals = arr;
    }
    // Set values from rvalue array
    void set(std::array<T, N>&& arr) {
        vals = std::move(arr);
    }

    // Return element address
    T& operator[](const size_t i) { return vals[i]; }
    // Return element address (read-only)
    const T& operator[](const size_t i) const { return vals[i]; }

    bool operator==(const IDX& other) const {
        return vals == other.vals;
    }

    // Return an IDX object with a different number of indices and/or different data type
    // Converting from a float type to int uses rounding
    // If the target number of indices is more than that of this object, the remainder are
    // defaulted
    // If the target number of indices is fewer than that of this object, the excess are cut off
    template <size_t targetN, typename targetT>
    requires std::floating_point<T> && std::same_as<targetT, int>
    operator IDX<targetN, targetT>() const {
        IDX<targetN, targetT> target;
        for (size_t i = 0; i < std::min(N, targetN); i++) {
            target.vals[i] = static_cast<targetT>(std::floor(vals[i] + 0.5));
        }
        for (size_t i = std::min(N, targetN); i < targetN; i++) {
            target.vals[i] = targetT{};
        }
        return target;
    }

    // Return an IDX object with a different number of indices and/or different data type
    // If the target number of indices is more than that of this object, the remainder are
    // defaulted
    // If the target number of indices is fewer than that of this object, the excess are cut off
    template <size_t targetN, typename targetT>
    operator IDX<targetN, targetT>() const {
        IDX<targetN, targetT> target;
        for (size_t i = 0; i < std::min(N, targetN); i++) {
            target.vals[i] = static_cast<targetT>(vals[i]);
        }
        for (size_t i = std::min(N, targetN); i < targetN; i++) {
            target.vals[i] = targetT{};
        }
        return target;
    }

    // Return indices as string in form (i, j, ...)
    std::string asString() const {
        std::string result = "(";
        for (size_t i = 0; i < N - 1; i++) {
            result += std::format("{}, ", vals[i]);
        }
        result += std::format("{})", vals[N - 1]);
        return result;
    }
};

// IDX hashing struct for creating std::unordered_map
template <size_t N, typename T>
struct IDXHasher {
    size_t operator()(const IDX<N, T>& idxs) const {
        size_t h = 0;
        for (size_t i = 0; i < N; i++) {
            h ^= std::hash<T>()(idxs[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }
};

#endif