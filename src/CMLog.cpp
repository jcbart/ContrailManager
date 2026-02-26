#include <iostream>
#include <ESMC.h>
#include "CMLog.h"
#include "mapUtils.h"

void CM_LogWrite(const char* msg) {
    int rc = ESMC_LogWrite(msg, ESMC_LOGMSG_INFO);
}

void CM_LogWrite(std::string msg) {
    CM_LogWrite(msg.c_str());
}

void CM_RaiseError(const char* msg, const char* filename, const int line) {
    std::string error = std::string("Contrail Manager error in ") + filename
                        + std::string(", line ") + std::to_string(line)
                        + std::string(": ") + msg;

    int rc = ESMC_LogWrite(error.c_str(), ESMC_LOGMSG_ERROR);
    std::cerr << error << std::endl;
    exit(EXIT_FAILURE);
}

void CM_RaiseError(const std::string msg, const char* filename, const int line) {
    CM_RaiseError(msg.c_str(), filename, line);
}

template <typename GeoType>
void CM_RaiseUnexpectedOutOfBounds(const GeoType& loc, const char* filename, const int line) {
    std::string msg = "expected location to be in domain, but loc_to_ijk failed at "
        + loc.asString();
    CM_RaiseError(msg, filename, line);
}

// Types to compile
template void CM_RaiseUnexpectedOutOfBounds(const Geo2D& loc, const char* filename, const int line);
template void CM_RaiseUnexpectedOutOfBounds(const Geo3D& loc, const char* filename, const int line);