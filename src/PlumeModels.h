#ifndef PLUMEMODELS_H
#define PLUMEMODELS_H

#include <string_view>
#include <utility>

namespace PlumeModels {

// Stores model ID and name
struct Model {
    int ID;
    std::string_view name;
};

//using Model = std::pair<int, std::string_view>;

// CoCiP model ID and name
constexpr Model COCIP = {1, "CoCiP"};

}

#endif