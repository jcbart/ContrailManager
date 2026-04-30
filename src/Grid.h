#ifndef GRID_H
#define GRID_H

#include <string>
#include <format>
#include "map/types.h"
#include "CMLog.h"

// A logically rectangular row-major N-dimensional grid
template <size_t N>
class Grid {
private:
    const std::array<int, N> lbs; // Dimension lower bounds (inclusive)
    const std::array<int, N> ubs; // Dimension upper bounds (inclusive)
    const std::array<size_t, N> extents; // Dimension extents
    const size_t num_elements; // Number of elements in grid

    void check_valid_idxs(const IDX<N, int> idxs) const {
        for (size_t i = 0; i < N; i++) {
            if (idxs[i] < lbs[i] || idxs[i] > ubs[i]) [[unlikely]] {
                std::string msg = std::format(
                    "Grid error: Index {} is out of range for array with bounds (", idxs.asString()
                );
                for (size_t i = 0; i < N - 1; i++) {
                    msg += std::format("{}:{}, ", lbs[i], ubs[i]);
                }
                msg += std::format("{}:{})", lbs[N - 1], ubs[N - 1]);
                CM_RaiseError(msg, __FILE__, __LINE__);
            }
        }
    }

    // Helper method for calculating extents given bounds
    static std::array<size_t, N> find_extents(
        const std::array<int, N>& lower,
        const std::array<int, N>& upper)
    {
        std::array<size_t, N> result;
        for (size_t i = 0; i < N; i++) {
            result[i] = static_cast<size_t>(upper[i] - lower[i] + 1);
        }
        return result;
    }

    // Helper method for calculating num_elements given extents
    static size_t find_num_elements(const std::array<size_t, N>& ext) {
        size_t total = 1;
        for (size_t i = 0; i < N; i++) {
            total *= ext[i];
        }
        return total;
    }

public:
    // Constructor
    Grid(
        const std::array<int, N> lower,
        const std::array<int, N> upper
    ) : lbs(lower),
        ubs(upper),
        extents(find_extents(lbs, ubs)),
        num_elements(find_num_elements(extents)) {

        // Assert that ubs >= lbs
        for (size_t i = 0; i < N; i++) {
            if (lbs[i] > ubs[i]) {
                std::string msg = std::format(
                    "Grid error: Bounds of dimension {} ({}:{}) are invalid.",
                    i, lbs[i], ubs[i]
                );
                CM_RaiseError(msg, __FILE__, __LINE__);
            }
        }
    }

    std::array<int, N> get_lbs() const { return lbs; }
    std::array<int, N> get_ubs() const { return ubs; }
    std::array<size_t, N> get_extents() const { return extents; }
    size_t get_num_elements() const { return num_elements; }

    // Turn N-D indices into index for accessing data; checks indices are valid
    size_t flatten(const IDX<N, int>& idxs) const {
        check_valid_idxs(idxs);
        size_t offset = 0;
        size_t stride = 1;
        for (size_t i = N; i-- > 0;) {
            offset += static_cast<size_t>(idxs[i] - lbs[i]) * stride;
            stride *= extents[i];
        }
        return offset;
    }

    // Turn index for accessing data into N-D indices; does not check if index is valid
    IDX<N, int> unflatten(const size_t idx) const {
        size_t offset = idx;
        IDX<N, int> result;
        for (size_t i = N; i-- > 0;) {
            result[i] = static_cast<int>(offset % extents[i]) + lbs[i];
            offset /= extents[i];
        }
        return result;
    }

    
};

#endif