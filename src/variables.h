#ifndef VARIABLES_H
#define VARIABLES_H

#include <string>
#include <sstream>
#include <omp.h>
#include "map/types.h"
#include "CMLog.h"

template <typename T>
class Variable2D {
private:
    std::string name;
    const int ids, ide, jds, jde;
    const int i_size, j_size;
    const size_t num_elements;

    T* data; // Raw, contiguous data

    // Checks indices are valid
    constexpr void check_valid(const int i, const int j) const {
        if (i < ids || i > ide || j < jds || j > jde) [[unlikely]] {
            std::stringstream ss;
            ss << "Variable2D " << name << " error: Index (i,j)=(" << i << "," << j
                << ") is out of range for array of size (ids:ide,jds:jde)=("
                << ids << ":" << ide << "," << jds << ":" << jde << ")";
            CM_RaiseError(ss.str(), __FILE__, __LINE__);
        }
    }

public:
    // Constructor
    Variable2D(std::string name, int ids, int ide, int jds, int jde)
        : name(name), ids(ids), ide(ide), jds(jds), jde(jde),
          i_size(ide - ids + 1), j_size(jde - jds + 1),
          num_elements(i_size * j_size) {
        
        // Allocate a 1D block of memory
        data = new T[num_elements];
        clear_all();
    }

    // Destructor
    ~Variable2D() {
        if (data != nullptr) {
            delete[] data;
        }
    }

    int get_ids() const { return ids; };
    int get_ide() const { return ide; };
    int get_jds() const { return jds; };
    int get_jde() const { return jde; };
    int get_i_size() const { return i_size; };
    int get_j_size() const { return j_size; };
    size_t get_num_elements() const { return num_elements; };
    // Returns a pointer to the first element of data
    // Operations on this pointer are NOT thread-safe
    T* get_data() const { return data; };

    // Flatten 2D indices
    constexpr size_t get_1D_index_from_2D(const int i, const int j) const {
        check_valid(i, j);
        return (static_cast<size_t>(i) - ids) * j_size +
               (static_cast<size_t>(j) - jds);
    }

    // Returns the value at indices; thread-safe
    constexpr T get(const int i, const int j) const {
        T result;
        #pragma omp atomic read
        result = data[get_1D_index_from_2D(i, j)];
        return result;
    }

    // Returns the value at indices; thread-safe
    constexpr T get(const IDX2<int>& ij) const {
        return get(ij.i, ij.j);
    }

    // Returns a pointer to the element at indices
    // Operations on this pointer are NOT thread-safe
    constexpr T* get_ptr(const int i, const int j) const {
        return &data[get_1D_index_from_2D(i, j)];
    }

    // Returns a pointer to the element at indices
    // Operations on this pointer are NOT thread-safe
    constexpr T* get_ptr(const IDX2<int>& ij) const {
        return get_ptr(ij.i, ij.j);
    }

    // Sets the value at indices; thread-safe
    constexpr void set(const int i, const int j, T value) {
        #pragma omp atomic write
        data[get_1D_index_from_2D(i, j)] = value;
    }

    // Sets the value at indices; thread-safe
    constexpr void set(const IDX2<int>& ij, T value) {
        set(ij.i, ij.j, value);
    }

    // Adds to the value at indices; thread-safe
    constexpr void add(const int i, const int j, T value) {
        #pragma omp atomic update
        data[get_1D_index_from_2D(i, j)] += value;
    }

    // Adds to the value at indices; thread-safe
    constexpr void add(const IDX2<int>& ij, T value) {
        add(ij.i, ij.j, value);
    }

    // Subtracts from the value at indices; thread-safe
    constexpr void subtract(const int i, const int j, T value) {
        #pragma omp atomic update
        data[get_1D_index_from_2D(i, j)] -= value;
    }

    // Subtracts from the value at indices; thread-safe
    constexpr void subtract(const IDX2<int>& ij, T value) {
        subtract(ij.i, ij.j, value);
    }

    // Multiplies the value at indices; thread-safe
    template <typename type>
    constexpr void multiply(const int i, const int j, type scalar) {
        #pragma omp atomic update
        data[get_1D_index_from_2D(i, j)] *= scalar;
    }

    // Multiplies the value at indices; thread-safe
    template <typename type>
    constexpr void multiply(const IDX2<int>& ij, type scalar) {
        multiply(ij.i, ij.j, scalar);
    }

    // Divides the value at indices; thread-safe
    template <typename type>
    constexpr void divide(const int i, const int j, type scalar) {
        #pragma omp atomic update
        data[get_1D_index_from_2D(i, j)] /= scalar;
    }

    // Divides the value at indices; thread-safe
    template <typename type>
    constexpr void divide(const IDX2<int>& ij, type scalar) {
        divide(ij.i, ij.j, scalar);
    }

    // Set all values to zero
    void clear_all() {
        for (size_t i = 0; i < num_elements; i++) {
            data[i] = 0;
        }
    }
};

template <typename T>
class Variable3D {
private:
    std::string name;
    const int ids, ide, jds, jde, kds, kde;
    const int i_size, j_size, k_size;
    const size_t num_elements;

    T* data; // Raw, contiguous data

    // Checks indices are valid
    constexpr void check_valid(const int i, const int j, const int k) const {
        if (i < ids || i > ide || j < jds || j > jde || k < kds || k > kde) [[unlikely]] {
            std::stringstream ss;
            ss << "Variable3D " << name << " error: Index (i,j,k)=(" << i << "," << j << "," << k
                << ") is out of range for array of size (ids:ide,jds:jde,kds:kde)=("
                << ids << ":" << ide << "," << jds << ":" << jde << "," << kds << ":" << kde << ")";
            CM_RaiseError(ss.str(), __FILE__, __LINE__);
        }
    }

public:
    // Constructor
    Variable3D(std::string name, int ids, int ide, int jds, int jde, int kds, int kde)
        : name(name), ids(ids), ide(ide), jds(jds), jde(jde), kds(kds), kde(kde),
          i_size(ide - ids + 1), j_size(jde - jds + 1), k_size(kde - kds + 1),
          num_elements(i_size * j_size * k_size) {
        
        // Allocate a 1D block of memory
        data = new T[num_elements];
        clear_all();
    }
    
    // Destructor
    ~Variable3D() {
        if (data != nullptr) {
            delete[] data;
        }
    }

    int get_ids() const { return ids; };
    int get_ide() const { return ide; };
    int get_jds() const { return jds; };
    int get_jde() const { return jde; };
    int get_kds() const { return kds; };
    int get_kde() const { return kde; };
    int get_i_size() const { return i_size; };
    int get_j_size() const { return j_size; };
    int get_k_size() const { return k_size; };
    size_t get_num_elements() const { return num_elements; };
    // Returns a pointer to the first element of data
    // Operations on this pointer are NOT thread-safe
    T* get_data() const { return data; };

    // Flatten 3D indices
    constexpr size_t get_1D_index_from_3D(const int i, const int j, const int k) const {
        check_valid(i, j, k);
        return (static_cast<size_t>(i) - ids) * j_size * k_size +
               (static_cast<size_t>(j) - jds) * k_size +
               (static_cast<size_t>(k) - kds);
    }

    // Returns the value at indices; thread-safe
    constexpr T get(const int i, const int j, const int k) const {
        T result;
        #pragma omp atomic read
        result = data[get_1D_index_from_3D(i, j, k)];
        return result;
    }

    // Returns the value at indices; thread-safe
    constexpr T get(const IDX3<int>& ijk) const {
        return get(ijk.i, ijk.j, ijk.k);
    }

    // Returns a pointer to the element at indices
    // Operations on this pointer are NOT thread-safe
    constexpr T* get_ptr(const int i, const int j, const int k) const {
        return &data[get_1D_index_from_3D(i, j, k)];
    }

    // Returns a pointer to the element at indices
    // Operations on this pointer are NOT thread-safe
    constexpr T* get_ptr(const IDX3<int>& ijk) const {
        return get_ptr(ijk.i, ijk.j, ijk.k);
    }

    // Sets the value at indices; thread-safe
    constexpr void set(const int i, const int j, const int k, T value) {
        #pragma omp atomic write
        data[get_1D_index_from_3D(i, j, k)] = value;
    }

    // Sets the value at indices; thread-safe
    constexpr void set(const IDX3<int>& ijk, T value) {
        set(ijk.i, ijk.j, ijk.k, value);
    }

    // Adds to the value at indices; thread-safe
    constexpr void add(const int i, const int j, const int k, T value) {
        #pragma omp atomic update
        data[get_1D_index_from_3D(i, j, k)] += value;
    }

    // Adds to the value at indices; thread-safe
    constexpr void add(const IDX3<int>& ijk, T value) {
        add(ijk.i, ijk.j, ijk.k, value);
    }

    // Subtracts from the value at indices; thread-safe
    constexpr void subtract(const int i, const int j, const int k, T value) {
        #pragma omp atomic update
        data[get_1D_index_from_3D(i, j, k)] -= value;
    }

    // Subtracts from the value at indices; thread-safe
    constexpr void subtract(const IDX3<int>& ijk, T value) {
        subtract(ijk.i, ijk.j, ijk.k, value);
    }

    // Multiplies the value at indices; thread-safe
    template <typename type>
    constexpr void multiply(const int i, const int j, const int k, type scalar) {
        #pragma omp atomic update
        data[get_1D_index_from_3D(i, j, k)] *= scalar;
    }

    // Multiplies the value at indices; thread-safe
    template <typename type>
    constexpr void multiply(const IDX3<int>& ijk, type scalar) {
        multiply(ijk.i, ijk.j, ijk.k, scalar);
    }

    // Divides the value at indices; thread-safe
    template <typename type>
    constexpr void divide(const int i, const int j, const int k, type scalar) {
        #pragma omp atomic update
        data[get_1D_index_from_3D(i, j, k)] /= scalar;
    }

    // Divides the value at indices; thread-safe
    template <typename type>
    constexpr void divide(const IDX3<int>& ijk, type scalar) {
        divide(ijk.i, ijk.j, ijk.k, scalar);
    }

    // Set all values to zero
    void clear_all() {
        for (size_t i = 0; i < num_elements; i++) {
            data[i] = 0;
        }
    }
};

#endif