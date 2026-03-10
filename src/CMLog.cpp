#include <iostream>
#include <format>
#include <ESMC.h>
#include "CMLog.h"
#include "mapUtils.h"

void CM_LogWrite(const std::string_view msg) {
    int rc = ESMC_LogWrite(std::string(msg).c_str(), ESMC_LOGMSG_INFO);
}

void CM_RaiseError(const std::string_view msg, const std::string_view filename, const int line) {
    std::string error = std::format("CM error in {}, line {}: {}", filename, line, msg);

    int rc = ESMC_LogWrite(error.c_str(), ESMC_LOGMSG_ERROR);
    std::cerr << error << std::endl;
    exit(EXIT_FAILURE);
}

template <typename GeoType>
void CM_RaiseUnexpectedOutOfBounds(const GeoType& loc, const std::string_view filename, const int line) {
    std::string msg = std::format("expected location to be in domain, but loc_to_ijk failed at {}",
                                  loc.asString());
    CM_RaiseError(msg, filename, line);
}

// Types to compile
template void CM_RaiseUnexpectedOutOfBounds(const Geo2D& loc, const std::string_view filename, const int line);
template void CM_RaiseUnexpectedOutOfBounds(const Geo3D& loc, const std::string_view filename, const int line);