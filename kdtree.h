#ifndef KDTREE
#define KDTREE

#include <nanoflann.hpp>
#include "mapUtils.h"

// Define make_unique function for C++11
namespace local {
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    // Allocates T and passes all arguments (args) to its constructor
    // std::forward maintains the value category (lvalue/rvalue) of the arguments
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
} // end namespace local

// nanoflann PointCloud structure
struct PointCloud {
    std::vector<Cart3D> points;

    // Return number of data points
    inline size_t kdtree_get_point_count() const { return points.size(); }

    // Return the dim'th component of the idx'th point in the class
    inline float kdtree_get_pt(const size_t idx, const size_t dim) const {
        if (dim == 0) return points[idx].x;
        else if (dim == 1) return points[idx].y;
        else return points[idx].z;
    }

    // Optional bounding-box computation: return false to default to a standard
    // bbox computation loop.
    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /* bb */) const {
        return false;
    }
};

// Combines the nanoflann k-d tree functionality
class KDTreeIndexer {
public:
    // Define desired k-d tree type
    using my_kd_tree_t = nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<float, PointCloud>,
        PointCloud, 3 /* dim */
        >;
    
    PointCloud cloud;

    std::unique_ptr<my_kd_tree_t> index;

    // Calls buildIndex() on KDTreeIndexer::index
    void buildIndex() {
        index = local::make_unique<my_kd_tree_t>(3 /* dim */, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10 /* max leaf */));
        index->buildIndex();
    }

    // Find closest n nearest neighbours (converts pointIn to Cartesian)
    // out_indices is given the n indices of KDTreeIndexer::cloud closest to pointIn
    // out_dist_sqr is given the SQUARED distances to those indices
    void search(Geo2D& pointIn, int n, std::vector<uint32_t>& out_indices, std::vector<float>& out_dist_sqr) {
        // Convert to Cartesian
        const Cart3D pointInCart = Geo2D_to_Cart3D(pointIn);
        // Convert to type required by nanoflann
        const float query_pt[3] = {pointInCart.x, pointInCart.y, pointInCart.z};
        // Do knnSearch
        size_t num_results = index->knnSearch(&query_pt[0], n, &out_indices[0], &out_dist_sqr[0]);

        // If there are fewer points than requested
        out_indices.resize(num_results);
        out_dist_sqr.resize(num_results);
    }
};

#endif