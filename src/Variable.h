#ifndef VARIABLE_H
#define VARIABLE_H

#include <string>
#include <format>
#include <concepts>
#include <functional>
#include <omp.h>
#include "Grid.h"
#include "CMLog.h"

// Define a condition function F which takes a value of type T and returns a bool meaning it can be
// passed to the check_condition method of a variable of type T
template<typename F, typename T>
concept VariableConditionFunction = std::invocable<F, T>
    && std::same_as<std::invoke_result_t<F, T>, bool>;

// A container holding a N-dimensional grid and data of type T
template <size_t N, typename T>
class Variable {
private:
    std::unique_ptr<T[]> data; // Pointer to raw, contiguous data

public:
    const std::string_view name; // Variable name
    const Grid<N> grid; // Grid
    const T default_value; // Default value for data

    // Constructor
    Variable(
        const std::string_view name,
        const Grid<N> grid,
        const T default_val
    ) : name(name),
        grid(grid),
        default_value(default_val) {
        
        // Allocate data
        data = std::make_unique<T[]>(grid.get_num_elements());
        default_all();
    }

    // Returns a pointer to the first element of data
    // Operations on this pointer are NOT thread-safe
    T* get_data() const { return data.get(); }

    // Returns the value at indices; thread-safe
    T get(const IDX<N, int>& idxs) const {
        T result;
        #pragma omp atomic read
        result = data[grid.flatten(idxs)];
        return result;
    }

    // Returns a pointer to the element at indices
    // Operations on this pointer are NOT thread-safe
    T* get_ptr(const IDX<N, int>& idxs) const {
        return &data[grid.flatten(idxs)];
    }

    // Sets the value at indices; thread-safe
    void set(const IDX<N, int>& idxs, T value) {
        #pragma omp atomic write
        data[grid.flatten(idxs)] = value;
    }

    // Adds to the value at indices; thread-safe
    void add(const IDX<N, int>& idxs, T value) {
        #pragma omp atomic update
        data[grid.flatten(idxs)] += value;
    }

    // Subtracts from the value at indices; thread-safe
    void subtract(const IDX<N, int>& idxs, T value) {
        #pragma omp atomic update
        data[grid.flatten(idxs)] -= value;
    }

    // Multiplies the value at indices; thread-safe
    template <typename type>
    void multiply(const IDX<N, int>& idxs, type scalar) {
        #pragma omp atomic update
        data[grid.flatten(idxs)] *= scalar;
    }

    // Divides the value at indices; thread-safe
    template <typename type>
    void divide(const IDX<N, int>& idxs, type scalar) {
        #pragma omp atomic update
        data[grid.flatten(idxs)] /= scalar;
    }

    // Set all values to default
    void default_all() {
        for (size_t i = 0; i < grid.get_num_elements(); i++) {
            data[i] = default_value;
        }
    }

    // Checks each of the values against a condition; raises an error if the condition is false
    template<typename F>
    requires VariableConditionFunction<F, T>
    void check_condition(F&& condition) const {
        for (size_t i = 0; i < grid.get_num_elements(); i++) {
            if(!condition(data[i])) [[unlikely]] {
                CM_RaiseError(
                    std::format("Variable {} error: value at {} = {} is invalid",
                        name, grid.unflatten(i).asString(), data[i]
                    ), __FILE__, __LINE__
                );
            }
        }
    }
};

#endif