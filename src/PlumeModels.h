#ifndef PLUMEMODELS_H
#define PLUMEMODELS_H

#include <string_view>

namespace PlumeModels {

// Stores model ID and name
struct Model {
    int ID;
    std::string_view name;
};

// Contrails as Cloud Enhancement (CaCE) model ID and name
constexpr Model CACE = {1, "CaCE"};

// CoCiP model ID and name
constexpr Model COCIP = {2, "CoCiP"};

}

#endif