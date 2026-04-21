#ifndef VARIABLE_H
#define VARIABLE_H

#include <string>
#include <format>
#include <concepts>
#include <functional>
#include <omp.h>
#include "map/types.h"
#include "CMLog.h"

// Define a condition function F which takes a value of type T and returns a bool meaning it can be
// passed to the check_condition method of a variable of type T
template<typename F, typename T>
concept VariableConditionFunction = std::invocable<F, T>
    && std::same_as<std::invoke_result_t<F, T>, bool>;

// A logically rectangular row-major N-dimensional grid holding data of type T
template <size_t N, typename T>
class Variable {
private:
    const std::string_view name; // Variable name
    const std::array<int, N> lbs; // Dimension lower bounds (inclusive)
    const std::array<int, N> ubs; // Dimension upper bounds (inclusive)
    const std::array<size_t, N> extents; // Dimension extents
    const size_t num_elements; // Number of elements in grid
    const T default_value; // Default value for data
    std::unique_ptr<T[]> data; // Pointer to raw, contiguous data

    void check_valid_idxs(const IDX<N, int> idxs) const {
        for (size_t i = 0; i < N; i++) {
            if (idxs[i] < lbs[i] || idxs[i] > ubs[i]) [[unlikely]] {
                std::string msg = std::format(
                    "Variable {} error: Index {} is out of range for array with bounds (",
                    name, idxs.asString()
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
    Variable(
        const std::string_view name,
        const std::array<int, N> lower,
        const std::array<int, N> upper,
        const T default_val
    ) : name(name),
        lbs(lower),
        ubs(upper),
        extents(find_extents(lbs, ubs)),
        num_elements(find_num_elements(extents)),
        default_value(default_val) {

        // Assert that ubs >= lbs
        for (size_t i = 0; i < N; i++) {
            if (lbs[i] > ubs[i]) {
                std::string msg = std::format(
                    "Variable {} error: Bounds of dimension {} ({}:{}) are invalid.",
                    name, i, lbs[i], ubs[i]
                );
                CM_RaiseError(msg, __FILE__, __LINE__);
            }
        }
        
        // Allocate data
        data = std::make_unique<T[]>(num_elements);
        default_all();
    }

    std::array<int, N> get_lbs() const { return lbs; }
    std::array<int, N> get_ubs() const { return ubs; }
    std::array<size_t, N> get_extents() const { return extents; }
    size_t get_num_elements() const { return num_elements; }
    // Returns a pointer to the first element of data
    // Operations on this pointer are NOT thread-safe
    T* get_data() const { return data.get(); }

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

    // Returns the value at indices; thread-safe
    T get(const IDX<N, int>& idxs) const {
        T result;
        #pragma omp atomic read
        result = data[flatten(idxs)];
        return result;
    }

    // Returns a pointer to the element at indices
    // Operations on this pointer are NOT thread-safe
    T* get_ptr(const IDX<N, int>& idxs) const {
        return &data[flatten(idxs)];
    }

    // Sets the value at indices; thread-safe
    void set(const IDX<N, int>& idxs, T value) {
        #pragma omp atomic write
        data[flatten(idxs)] = value;
    }

    // Adds to the value at indices; thread-safe
    void add(const IDX<N, int>& idxs, T value) {
        #pragma omp atomic update
        data[flatten(idxs)] += value;
    }

    // Subtracts from the value at indices; thread-safe
    void subtract(const IDX<N, int>& idxs, T value) {
        #pragma omp atomic update
        data[flatten(idxs)] -= value;
    }

    // Multiplies the value at indices; thread-safe
    template <typename type>
    void multiply(const IDX<N, int>& idxs, type scalar) {
        #pragma omp atomic update
        data[flatten(idxs)] *= scalar;
    }

    // Divides the value at indices; thread-safe
    template <typename type>
    void divide(const IDX<N, int>& idxs, type scalar) {
        #pragma omp atomic update
        data[flatten(idxs)] /= scalar;
    }

    // Set all values to default
    void default_all() {
        for (size_t i = 0; i < num_elements; i++) {
            data[i] = default_value;
        }
    }

    // Checks each of the values against a condition; raises an error if the condition is false
    template<typename F>
    requires VariableConditionFunction<F, T>
    void check_condition(F&& condition) const {
        for (size_t i = 0; i < num_elements; i++) {
            if(!condition(data[i])) [[unlikely]] {
                CM_RaiseError(
                    std::format("Variable {} error: value at {} = {} is invalid",
                        name, unflatten(i).asString(), data[i]
                    ), __FILE__, __LINE__
                );
            }
        }
    }
};

#endif