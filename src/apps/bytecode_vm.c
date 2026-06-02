#include "apps/bytecode_vm.h"

#include "apps/app_format.h"
#include "debug/log.h"
#include "display/console.h"
#include "input/keyboard.h"

#define BCVM_STACK_MAX 256U
#define BCVM_LOCALS_MAX 64U
#define BCVM_CALL_DEPTH_MAX 32U
#define BCVM_MAX_STEPS 200000U

static uint16_t read_u16le(const uint8_t* source) {
    return (uint16_t)(source[0] | ((uint16_t)source[1] << 8));
}

static uint32_t read_u32le(const uint8_t* source) {
    return (uint32_t)source[0]
        | ((uint32_t)source[1] << 8)
        | ((uint32_t)source[2] << 16)
        | ((uint32_t)source[3] << 24);
}

static int32_t read_i32le(const uint8_t* source) {
    return (int32_t)read_u32le(source);
}

static void vm_write_int(int32_t value) {
    if (value < 0) {
        console_write_char('-');
        console_write_uint((unsigned int)(-value));
        return;
    }

    console_write_uint((unsigned int)value);
}

static void vm_erase_last_echoed_char(void) {
    int x = console_get_x();
    int y = console_get_y();

    if (x <= 0) {
        return;
    }

    console_set_cursor(x - 1, y);
    console_write_char(' ');
    console_set_cursor(x - 1, y);
}

static int32_t vm_read_int(void) {
    char buffer[32];
    uint32_t length = 0;

    while (1) {
        KeyEvent event = keyboard_read_event();

        if (event.type == KEY_EVENT_ENTER) {
            int32_t value = 0;
            int sign = 1;
            uint32_t i = 0;

            console_write_char('\n');

            if (length == 0U) {
                return 0;
            }

            if (buffer[0] == '-') {
                sign = -1;
                i = 1;
            }

            for (; i < length; i++) {
                value = value * 10 + (int32_t)(buffer[i] - '0');
            }

            return value * sign;
        }

        if (event.type == KEY_EVENT_BACKSPACE) {
            if (length > 0U) {
                length--;
                vm_erase_last_echoed_char();
            }
            continue;
        }

        if (event.type != KEY_EVENT_CHARACTER) {
            continue;
        }

        if (event.character == '-' && length == 0U) {
            buffer[length++] = event.character;
            console_write_char(event.character);
            continue;
        }

        if (event.character >= '0' && event.character <= '9') {
            if (length + 1U < sizeof(buffer)) {
                buffer[length++] = event.character;
                console_write_char(event.character);
            }
        }
    }
}

static int vm_read_header(const uint8_t* image, uint32_t image_size, bcvm_image_header_t* out_header) {
    if (image == 0 || out_header == 0 || image_size < BCVM_IMAGE_HEADER_SIZE) {
        return -1;
    }

    out_header->magic = read_u32le(&image[0]);
    out_header->version = read_u16le(&image[4]);
    out_header->reserved = read_u16le(&image[6]);
    out_header->code_size = read_u32le(&image[8]);
    out_header->data_size = read_u32le(&image[12]);
    out_header->entry_offset = read_u32le(&image[16]);

    if (out_header->magic != BCVM_MAGIC || out_header->version != BCVM_VERSION) {
        return -1;
    }

    if (out_header->code_size > (image_size - BCVM_IMAGE_HEADER_SIZE)) {
        return -1;
    }

    if (out_header->data_size > (image_size - BCVM_IMAGE_HEADER_SIZE - out_header->code_size)) {
        return -1;
    }

    if (out_header->entry_offset >= out_header->code_size) {
        return -1;
    }

    return 0;
}

int bytecode_vm_run(const uint8_t* image, uint32_t image_size, const char* args) {
    bcvm_image_header_t header;
    const uint8_t* code;
    const uint8_t* data;
    int32_t stack[BCVM_STACK_MAX] = {0};
    int32_t locals[BCVM_CALL_DEPTH_MAX + 1U][BCVM_LOCALS_MAX];
    uint32_t call_return_pc[BCVM_CALL_DEPTH_MAX];
    uint32_t sp = 0;
    uint32_t call_depth = 0;
    uint32_t pc = 0;
    uint32_t steps = 0;

    (void)args;

    if (vm_read_header(image, image_size, &header) != 0) {
        ERROR_LOG("BCVM invalid image header");
        return -1;
    }

    {
        volatile int32_t* init_locals = &locals[0][0];
        volatile uint32_t* init_returns = &call_return_pc[0];

        for (uint32_t i = 0; i < (BCVM_CALL_DEPTH_MAX + 1U) * BCVM_LOCALS_MAX; i++) {
            init_locals[i] = 0;
        }

        for (uint32_t i = 0; i < BCVM_CALL_DEPTH_MAX; i++) {
            init_returns[i] = 0U;
        }
    }

    code = image + BCVM_IMAGE_HEADER_SIZE;
    data = code + header.code_size;
    pc = header.entry_offset;

    DEBUG_LOG("BCVM started");

    while (pc < header.code_size && steps < BCVM_MAX_STEPS) {
        uint8_t opcode = code[pc++];
        steps++;

        switch (opcode) {
            case BCVM_OP_PUSH_I32: {
                if (pc + 4U > header.code_size || sp >= BCVM_STACK_MAX) {
                    ERROR_LOG("BCVM PUSH_I32 fault");
                    return -1;
                }

                stack[sp++] = read_i32le(&code[pc]);
                pc += 4U;
                break;
            }
            case BCVM_OP_LOAD_LOCAL: {
                uint8_t index;
                if (pc >= header.code_size || sp >= BCVM_STACK_MAX) {
                    ERROR_LOG("BCVM LOAD_LOCAL fault");
                    return -1;
                }

                index = code[pc++];
                if (index >= BCVM_LOCALS_MAX) {
                    return -1;
                }

                stack[sp++] = locals[call_depth][index];
                break;
            }
            case BCVM_OP_STORE_LOCAL: {
                uint8_t index;
                if (pc >= header.code_size || sp == 0U) {
                    ERROR_LOG("BCVM STORE_LOCAL fault");
                    return -1;
                }

                index = code[pc++];
                if (index >= BCVM_LOCALS_MAX) {
                    return -1;
                }

                locals[call_depth][index] = stack[--sp];
                break;
            }
            case BCVM_OP_ADD:
            case BCVM_OP_SUB:
            case BCVM_OP_MUL:
            case BCVM_OP_DIV: {
                int32_t rhs;
                int32_t lhs;
                if (sp < 2U) {
                    ERROR_LOG("BCVM arithmetic underflow");
                    return -1;
                }

                rhs = stack[--sp];
                lhs = stack[--sp];

                if (opcode == BCVM_OP_ADD) {
                    stack[sp++] = lhs + rhs;
                } else if (opcode == BCVM_OP_SUB) {
                    stack[sp++] = lhs - rhs;
                } else if (opcode == BCVM_OP_MUL) {
                    stack[sp++] = lhs * rhs;
                } else {
                    if (rhs == 0) {
                        ERROR_LOG("BCVM divide by zero");
                        return -1;
                    }
                    stack[sp++] = lhs / rhs;
                }
                break;
            }
            case BCVM_OP_NEG: {
                if (sp == 0U) {
                    return -1;
                }
                stack[sp - 1U] = -stack[sp - 1U];
                break;
            }
            case BCVM_OP_EQ:
            case BCVM_OP_NE:
            case BCVM_OP_LT:
            case BCVM_OP_LE:
            case BCVM_OP_GT:
            case BCVM_OP_GE: {
                int32_t rhs;
                int32_t lhs;
                int32_t result = 0;

                if (sp < 2U) {
                    ERROR_LOG("BCVM compare underflow");
                    return -1;
                }

                rhs = stack[--sp];
                lhs = stack[--sp];

                if (opcode == BCVM_OP_EQ) {
                    result = (lhs == rhs) ? 1 : 0;
                } else if (opcode == BCVM_OP_NE) {
                    result = (lhs != rhs) ? 1 : 0;
                } else if (opcode == BCVM_OP_LT) {
                    result = (lhs < rhs) ? 1 : 0;
                } else if (opcode == BCVM_OP_LE) {
                    result = (lhs <= rhs) ? 1 : 0;
                } else if (opcode == BCVM_OP_GT) {
                    result = (lhs > rhs) ? 1 : 0;
                } else {
                    result = (lhs >= rhs) ? 1 : 0;
                }

                stack[sp++] = result;
                break;
            }
            case BCVM_OP_JMP: {
                uint32_t target;
                if (pc + 4U > header.code_size) {
                    ERROR_LOG("BCVM JMP fault");
                    return -1;
                }

                target = read_u32le(&code[pc]);
                if (target >= header.code_size) {
                    return -1;
                }

                pc = target;
                break;
            }
            case BCVM_OP_JMP_IF_ZERO: {
                uint32_t target;
                int32_t cond;

                if (pc + 4U > header.code_size || sp == 0U) {
                    ERROR_LOG("BCVM JMP_IF_ZERO fault");
                    return -1;
                }

                target = read_u32le(&code[pc]);
                pc += 4U;
                cond = stack[--sp];

                if (cond == 0) {
                    if (target >= header.code_size) {
                        return -1;
                    }
                    pc = target;
                }
                break;
            }
            case BCVM_OP_CALL: {
                uint32_t target;
                volatile int32_t* frame_locals;

                if (pc + 4U > header.code_size || call_depth >= BCVM_CALL_DEPTH_MAX) {
                    ERROR_LOG("BCVM CALL fault");
                    return -1;
                }

                target = read_u32le(&code[pc]);
                pc += 4U;

                if (target >= header.code_size) {
                    return -1;
                }

                call_return_pc[call_depth] = pc;
                call_depth++;
                frame_locals = &locals[call_depth][0];
                for (uint32_t i = 0; i < BCVM_LOCALS_MAX; i++) {
                    frame_locals[i] = 0;
                }
                pc = target;
                break;
            }
            case BCVM_OP_CALL_ARGS: {
                uint32_t target;
                uint8_t arg_count;
                volatile int32_t* frame_locals;

                if (pc + 5U > header.code_size || call_depth >= BCVM_CALL_DEPTH_MAX) {
                    ERROR_LOG("BCVM CALL_ARGS fault");
                    return -1;
                }

                target = read_u32le(&code[pc]);
                pc += 4U;
                arg_count = code[pc++];

                if (target >= header.code_size || arg_count > BCVM_LOCALS_MAX || sp < arg_count) {
                    return -1;
                }

                call_return_pc[call_depth] = pc;
                call_depth++;

                frame_locals = &locals[call_depth][0];
                for (uint32_t i = 0; i < BCVM_LOCALS_MAX; i++) {
                    frame_locals[i] = 0;
                }

                for (uint32_t i = 0; i < (uint32_t)arg_count; i++) {
                    frame_locals[(uint32_t)arg_count - 1U - i] = stack[--sp];
                }

                pc = target;
                break;
            }
            case BCVM_OP_PRINT_STR: {
                uint16_t offset;
                uint16_t length;
                if (pc + 4U > header.code_size) {
                    ERROR_LOG("BCVM PRINT_STR fault");
                    return -1;
                }

                offset = read_u16le(&code[pc]);
                length = read_u16le(&code[pc + 2U]);
                pc += 4U;

                if ((uint32_t)offset + (uint32_t)length > header.data_size) {
                    return -1;
                }

                for (uint32_t i = 0; i < (uint32_t)length; i++) {
                    console_write_char((char)data[offset + i]);
                }
                break;
            }
            case BCVM_OP_PRINT_INT: {
                if (sp == 0U) {
                    return -1;
                }

                vm_write_int(stack[--sp]);
                break;
            }
            case BCVM_OP_PRINT_NL:
                console_write_char('\n');
                break;
            case BCVM_OP_READ_INT:
                if (sp >= BCVM_STACK_MAX) {
                    return -1;
                }
                stack[sp++] = vm_read_int();
                break;
            case BCVM_OP_POP:
                if (sp == 0U) {
                    return -1;
                }
                sp--;
                break;
            case BCVM_OP_RET:
                if (sp == 0U) {
                    ERROR_LOG("BCVM RET fault");
                    return -1;
                }

                if (call_depth == 0U) {
                    DEBUG_LOG("BCVM finished");
                    return 0;
                }

                {
                    int32_t ret_value = stack[--sp];
                    call_depth--;
                    if (sp >= BCVM_STACK_MAX) {
                        return -1;
                    }
                    stack[sp++] = ret_value;
                    pc = call_return_pc[call_depth];
                }
                break;
            case BCVM_OP_HALT:
                DEBUG_LOG("BCVM finished");
                return 0;
            default:
                ERROR_LOG("BCVM unknown opcode");
                return -1;
        }
    }

    ERROR_LOG("BCVM execution aborted");
    return -1;
}
