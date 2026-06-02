#ifndef PRISMOS_APPS_APP_FORMAT_H
#define PRISMOS_APPS_APP_FORMAT_H

#include <stdint.h>

#define PRISM_APP_MAGIC 0x50524953U /* 'PRIS' */
#define PRISM_APP_FORMAT_VERSION 1U

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint32_t entry_offset;
    uint32_t image_size;
    uint32_t reserved;
} prism_app_header_t;

#define PRISM_APP_HEADER_SIZE 20U

#define BCVM_MAGIC 0x4D564342U /* 'BCVM' */
#define BCVM_VERSION 1U
#define BCVM_IMAGE_HEADER_SIZE 20U

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
    uint32_t code_size;
    uint32_t data_size;
    uint32_t entry_offset;
} bcvm_image_header_t;

enum {
    BCVM_OP_PUSH_I32 = 0x01,
    BCVM_OP_LOAD_LOCAL = 0x02,
    BCVM_OP_STORE_LOCAL = 0x03,
    BCVM_OP_ADD = 0x04,
    BCVM_OP_SUB = 0x05,
    BCVM_OP_MUL = 0x06,
    BCVM_OP_DIV = 0x07,
    BCVM_OP_PRINT_STR = 0x08,
    BCVM_OP_PRINT_INT = 0x09,
    BCVM_OP_PRINT_NL = 0x0A,
    BCVM_OP_POP = 0x0B,
    BCVM_OP_RET = 0x0C,
    BCVM_OP_HALT = 0x0D,
    BCVM_OP_NEG = 0x0E,
    BCVM_OP_JMP = 0x0F,
    BCVM_OP_JMP_IF_ZERO = 0x10,
    BCVM_OP_EQ = 0x11,
    BCVM_OP_NE = 0x12,
    BCVM_OP_LT = 0x13,
    BCVM_OP_LE = 0x14,
    BCVM_OP_GT = 0x15,
    BCVM_OP_GE = 0x16,
    BCVM_OP_CALL = 0x17,
    BCVM_OP_CALL_ARGS = 0x18,
    BCVM_OP_READ_INT = 0x19,
};

#endif
