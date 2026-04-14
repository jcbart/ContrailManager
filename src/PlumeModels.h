#ifndef PLUMEMODELS_H
#define PLUMEMODELS_H

#include <string_view>

namespace PlumeModels {

// Stores model ID and name
struct Model {
    int ID;
    std::string_view name;
};

// CoCiP model ID and name
constexpr Model COCIP = {1, "CoCiP"};

}

#endif