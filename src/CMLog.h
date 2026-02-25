#ifndef CMLOG_H
#define CMLOG_H

#include <string>

// Writes a message to the log
void CM_LogWrite(const char* msg);

// Writes a message to the log
void CM_LogWrite(std::string msg);

// Logs an error, then exits
void CM_RaiseError(const char* msg, const char* filename, const int line);

// Logs an error, then exits
void CM_RaiseError(std::string msg, const char* filename, const int line);

#endif