#ifndef PRISMOS_APPS_PRISMCC_RUNTIME_H
#define PRISMOS_APPS_PRISMCC_RUNTIME_H

#include <stdint.h>

int prismcc_compile_file(const char* abs_input_path, const char* abs_output_path, char* out_error, uint32_t out_error_capacity);

#endif
