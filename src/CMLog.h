#ifndef CMLOG_H
#define CMLOG_H

#include <string_view>

// Writes a message to the log
void CM_LogWrite(const std::string_view msg);

// Writes a warning to the log
void CM_LogWarning(const std::string_view msg);

// Logs an error, then exits
void CM_RaiseError(const std::string_view msg, const std::string_view filename, const int line);

// Logs an out-of-bounds message, then exits
template <typename GeoType>
void CM_RaiseUnexpectedOutOfBounds(const GeoType& loc, const std::string_view filename, const int line);

#endif