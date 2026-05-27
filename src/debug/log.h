#ifndef PRISMOS_DEBUG_LOG_H
#define PRISMOS_DEBUG_LOG_H

void log_write(const char* level, const char* file, int line, const char* message);

// Centralized log macros keep call sites short and include source location.
#define DEBUG_LOG(message) log_write("DEBUG", __FILE__, __LINE__, (message))
#define WARNINIG_LOG(message) log_write("WARNING", __FILE__, __LINE__, (message))
// Backward-compatible alias for conventional spelling.
#define WARNING_LOG(message) WARNINIG_LOG((message))
#define ERROR_LOG(message) log_write("ERROR", __FILE__, __LINE__, (message))

#endif
