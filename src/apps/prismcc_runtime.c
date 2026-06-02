#include "apps/prismcc_runtime.h"

#include "apps/app_format.h"
#include "debug/log.h"
#include "filesystem/vfs.h"

#define PRISMCC_MAX_SOURCE_SIZE (32U * 1024U)
#define PRISMCC_MAX_CODE_SIZE (48U * 1024U)
#define PRISMCC_MAX_DATA_SIZE (16U * 1024U)
#define PRISMCC_MAX_LOCALS 64U
#define PRISMCC_MAX_TOKEN_TEXT 64U
#define PRISMCC_MAX_FUNCTIONS 64U
#define PRISMCC_MAX_CALL_PATCHES 256U

typedef enum {
    TOK_EOF = 0,
    TOK_INT,
    TOK_MAIN,
    TOK_RETURN,
    TOK_PRINT,
    TOK_PRINT_INT,
    TOK_INPUT_INT,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_SEMI,
    TOK_ASSIGN,
    TOK_COMMA,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_EQEQ,
    TOK_NEQ,
    TOK_LT,
    TOK_LTE,
    TOK_GT,
    TOK_GTE
} TokenType;

typedef struct {
    TokenType type;
    char text[PRISMCC_MAX_TOKEN_TEXT];
    int32_t number;
} Token;

typedef struct {
    const char* src;
    uint32_t length;
    uint32_t position;
    Token current;
} Lexer;

typedef struct {
    char name[32];
    uint8_t index;
} Local;

typedef struct {
    char name[32];
    uint32_t entry_offset;
    uint8_t param_count;
} FunctionDef;

typedef struct {
    char callee[32];
    uint32_t code_immediate_offset;
    uint8_t arg_count;
} CallPatch;

typedef struct {
    Lexer lexer;
    uint8_t code[PRISMCC_MAX_CODE_SIZE];
    uint32_t code_size;
    uint8_t data[PRISMCC_MAX_DATA_SIZE];
    uint32_t data_size;
    Local locals[PRISMCC_MAX_LOCALS];
    uint32_t local_count;
    FunctionDef functions[PRISMCC_MAX_FUNCTIONS];
    uint32_t function_count;
    CallPatch call_patches[PRISMCC_MAX_CALL_PATCHES];
    uint32_t call_patch_count;
    uint32_t main_entry;
    const char* error;
} PrismCompiler;

static char source_buffer[PRISMCC_MAX_SOURCE_SIZE + 1U];
static uint8_t output_buffer[PRISM_APP_HEADER_SIZE + BCVM_IMAGE_HEADER_SIZE + PRISMCC_MAX_CODE_SIZE + PRISMCC_MAX_DATA_SIZE];
static PrismCompiler compiler;

static void set_error(char* out, uint32_t capacity, const char* message) {
    uint32_t i = 0;

    if (out == 0 || capacity == 0U) {
        return;
    }

    while (message[i] != '\0' && i + 1U < capacity) {
        out[i] = message[i];
        i++;
    }

    out[i] = '\0';
}

static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int string_equals(const char* left, const char* right) {
    while (*left != '\0' && *right != '\0') {
        if (*left != *right) {
            return 0;
        }

        left++;
        right++;
    }

    return *left == '\0' && *right == '\0';
}

static uint32_t string_length(const char* text) {
    uint32_t length = 0;

    while (text[length] != '\0') {
        length++;
    }

    return length;
}

static void copy_string(char* destination, const char* source, uint32_t capacity) {
    uint32_t i = 0;

    if (capacity == 0U) {
        return;
    }

    while (source[i] != '\0' && i + 1U < capacity) {
        destination[i] = source[i];
        i++;
    }

    destination[i] = '\0';
}

static int token_is_name(TokenType type) {
    return type == TOK_IDENTIFIER || type == TOK_MAIN;
}

static TokenType keyword_type(const char* text) {
    if (string_equals(text, "int")) {
        return TOK_INT;
    }

    if (string_equals(text, "main")) {
        return TOK_MAIN;
    }

    if (string_equals(text, "return")) {
        return TOK_RETURN;
    }

    if (string_equals(text, "print")) {
        return TOK_PRINT;
    }

    if (string_equals(text, "print_int")) {
        return TOK_PRINT_INT;
    }

    if (string_equals(text, "input_int")) {
        return TOK_INPUT_INT;
    }

    if (string_equals(text, "if")) {
        return TOK_IF;
    }

    if (string_equals(text, "else")) {
        return TOK_ELSE;
    }

    if (string_equals(text, "while")) {
        return TOK_WHILE;
    }

    if (string_equals(text, "for")) {
        return TOK_FOR;
    }

    return TOK_IDENTIFIER;
}

static void lexer_skip_ws(Lexer* lexer) {
    while (lexer->position < lexer->length) {
        char c = lexer->src[lexer->position];

        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            lexer->position++;
            continue;
        }

        if (c == '/' && (lexer->position + 1U) < lexer->length && lexer->src[lexer->position + 1U] == '/') {
            lexer->position += 2U;
            while (lexer->position < lexer->length && lexer->src[lexer->position] != '\n') {
                lexer->position++;
            }
            continue;
        }

        break;
    }
}

static Token lexer_next(Lexer* lexer, const char** out_error) {
    Token token;
    uint32_t text_index = 0;

    token.type = TOK_EOF;
    token.text[0] = '\0';
    token.number = 0;

    lexer_skip_ws(lexer);

    if (lexer->position >= lexer->length) {
        token.type = TOK_EOF;
        return token;
    }

    {
        char c = lexer->src[lexer->position++];

        if (is_alpha(c)) {
            token.text[text_index++] = c;
            while (lexer->position < lexer->length
                && (is_alpha(lexer->src[lexer->position]) || is_digit(lexer->src[lexer->position]))) {
                if (text_index + 1U >= PRISMCC_MAX_TOKEN_TEXT) {
                    *out_error = "identifier too long";
                    return token;
                }

                token.text[text_index++] = lexer->src[lexer->position++];
            }

            token.text[text_index] = '\0';
            token.type = keyword_type(token.text);
            return token;
        }

        if (is_digit(c)) {
            int32_t value = (int32_t)(c - '0');
            while (lexer->position < lexer->length && is_digit(lexer->src[lexer->position])) {
                value = value * 10 + (int32_t)(lexer->src[lexer->position] - '0');
                lexer->position++;
            }

            token.type = TOK_NUMBER;
            token.number = value;
            return token;
        }

        switch (c) {
            case '(': token.type = TOK_LPAREN; return token;
            case ')': token.type = TOK_RPAREN; return token;
            case '{': token.type = TOK_LBRACE; return token;
            case '}': token.type = TOK_RBRACE; return token;
            case ';': token.type = TOK_SEMI; return token;
            case ',': token.type = TOK_COMMA; return token;
            case '+': token.type = TOK_PLUS; return token;
            case '-': token.type = TOK_MINUS; return token;
            case '*': token.type = TOK_STAR; return token;
            case '/': token.type = TOK_SLASH; return token;
            case '=':
                if (lexer->position < lexer->length && lexer->src[lexer->position] == '=') {
                    lexer->position++;
                    token.type = TOK_EQEQ;
                } else {
                    token.type = TOK_ASSIGN;
                }
                return token;
            case '!':
                if (lexer->position < lexer->length && lexer->src[lexer->position] == '=') {
                    lexer->position++;
                    token.type = TOK_NEQ;
                    return token;
                }

                *out_error = "unexpected character";
                return token;
            case '<':
                if (lexer->position < lexer->length && lexer->src[lexer->position] == '=') {
                    lexer->position++;
                    token.type = TOK_LTE;
                } else {
                    token.type = TOK_LT;
                }
                return token;
            case '>':
                if (lexer->position < lexer->length && lexer->src[lexer->position] == '=') {
                    lexer->position++;
                    token.type = TOK_GTE;
                } else {
                    token.type = TOK_GT;
                }
                return token;
            case '"':
                while (lexer->position < lexer->length) {
                    char sc = lexer->src[lexer->position++];

                    if (sc == '"') {
                        token.text[text_index] = '\0';
                        token.type = TOK_STRING;
                        return token;
                    }

                    if (sc == '\\') {
                        if (lexer->position >= lexer->length) {
                            *out_error = "unterminated string";
                            return token;
                        }

                        sc = lexer->src[lexer->position++];
                        if (sc == 'n') {
                            sc = '\n';
                        } else if (sc == 't') {
                            sc = '\t';
                        }
                    }

                    if (text_index + 1U >= PRISMCC_MAX_TOKEN_TEXT) {
                        *out_error = "string too long";
                        return token;
                    }

                    token.text[text_index++] = sc;
                }

                *out_error = "unterminated string";
                return token;
            default:
                *out_error = "unexpected character";
                return token;
        }
    }
}

static void next_token(PrismCompiler* compiler) {
    const char* lexer_error = 0;

    compiler->lexer.current = lexer_next(&compiler->lexer, &lexer_error);
    if (lexer_error != 0 && compiler->error == 0) {
        compiler->error = lexer_error;
    }
}

static int expect(PrismCompiler* compiler, TokenType type, const char* message) {
    if (compiler->error != 0) {
        return -1;
    }

    if (compiler->lexer.current.type != type) {
        compiler->error = message;
        return -1;
    }

    next_token(compiler);
    return 0;
}

static int emit_u8(PrismCompiler* compiler, uint8_t value) {
    if (compiler->code_size >= PRISMCC_MAX_CODE_SIZE) {
        compiler->error = "code buffer full";
        return -1;
    }

    compiler->code[compiler->code_size++] = value;
    return 0;
}

static int emit_u16(PrismCompiler* compiler, uint16_t value) {
    if (emit_u8(compiler, (uint8_t)(value & 0xFFU)) != 0) {
        return -1;
    }

    return emit_u8(compiler, (uint8_t)((value >> 8) & 0xFFU));
}

static int emit_u32(PrismCompiler* compiler, uint32_t value) {
    if (emit_u8(compiler, (uint8_t)(value & 0xFFU)) != 0) {
        return -1;
    }
    if (emit_u8(compiler, (uint8_t)((value >> 8) & 0xFFU)) != 0) {
        return -1;
    }
    if (emit_u8(compiler, (uint8_t)((value >> 16) & 0xFFU)) != 0) {
        return -1;
    }
    return emit_u8(compiler, (uint8_t)((value >> 24) & 0xFFU));
}

static void patch_u32_at(PrismCompiler* compiler, uint32_t offset, uint32_t value) {
    if (offset + 4U > compiler->code_size) {
        compiler->error = "internal patch range error";
        return;
    }

    compiler->code[offset] = (uint8_t)(value & 0xFFU);
    compiler->code[offset + 1U] = (uint8_t)((value >> 8) & 0xFFU);
    compiler->code[offset + 2U] = (uint8_t)((value >> 16) & 0xFFU);
    compiler->code[offset + 3U] = (uint8_t)((value >> 24) & 0xFFU);
}

static int emit_jump_placeholder(PrismCompiler* compiler, uint8_t opcode, uint32_t* out_immediate_offset) {
    if (emit_u8(compiler, opcode) != 0) {
        return -1;
    }

    *out_immediate_offset = compiler->code_size;
    return emit_u32(compiler, 0U);
}

static int add_data(PrismCompiler* compiler, const char* bytes, uint16_t length, uint16_t* out_offset) {
    uint32_t end = compiler->data_size + (uint32_t)length;

    if (end > PRISMCC_MAX_DATA_SIZE) {
        compiler->error = "data section full";
        return -1;
    }

    *out_offset = (uint16_t)compiler->data_size;
    for (uint32_t i = 0; i < (uint32_t)length; i++) {
        compiler->data[compiler->data_size + i] = (uint8_t)bytes[i];
    }

    compiler->data_size = end;
    return 0;
}

static int find_local(PrismCompiler* compiler, const char* name) {
    for (uint32_t i = 0; i < compiler->local_count; i++) {
        if (string_equals(compiler->locals[i].name, name)) {
            return (int)compiler->locals[i].index;
        }
    }

    return -1;
}

static int add_local(PrismCompiler* compiler, const char* name, uint8_t* out_index) {
    if (compiler->local_count >= PRISMCC_MAX_LOCALS) {
        compiler->error = "too many locals";
        return -1;
    }

    if (find_local(compiler, name) >= 0) {
        compiler->error = "duplicate local";
        return -1;
    }

    copy_string(compiler->locals[compiler->local_count].name, name, sizeof(compiler->locals[compiler->local_count].name));
    compiler->locals[compiler->local_count].index = (uint8_t)compiler->local_count;
    *out_index = (uint8_t)compiler->local_count;
    compiler->local_count++;
    return 0;
}

static int find_function(PrismCompiler* compiler, const char* name) {
    for (uint32_t i = 0; i < compiler->function_count; i++) {
        if (string_equals(compiler->functions[i].name, name)) {
            return (int)i;
        }
    }

    return -1;
}

static int add_function(PrismCompiler* compiler, const char* name, uint32_t entry_offset, uint8_t param_count) {
    if (compiler->function_count >= PRISMCC_MAX_FUNCTIONS) {
        compiler->error = "too many functions";
        return -1;
    }

    if (find_function(compiler, name) >= 0) {
        compiler->error = "duplicate function";
        return -1;
    }

    copy_string(compiler->functions[compiler->function_count].name, name, sizeof(compiler->functions[compiler->function_count].name));
    compiler->functions[compiler->function_count].entry_offset = entry_offset;
    compiler->functions[compiler->function_count].param_count = param_count;
    compiler->function_count++;
    return 0;
}

static int add_call_patch(PrismCompiler* compiler, const char* callee, uint32_t immediate_offset, uint8_t arg_count) {
    if (compiler->call_patch_count >= PRISMCC_MAX_CALL_PATCHES) {
        compiler->error = "too many call sites";
        return -1;
    }

    copy_string(compiler->call_patches[compiler->call_patch_count].callee,
        callee,
        sizeof(compiler->call_patches[compiler->call_patch_count].callee));
    compiler->call_patches[compiler->call_patch_count].code_immediate_offset = immediate_offset;
    compiler->call_patches[compiler->call_patch_count].arg_count = arg_count;
    compiler->call_patch_count++;
    return 0;
}

static int emit_call_by_name(PrismCompiler* compiler, const char* name, uint8_t arg_count) {
    uint32_t patch_offset;

    if (emit_u8(compiler, BCVM_OP_CALL_ARGS) != 0) {
        return -1;
    }

    patch_offset = compiler->code_size;
    if (emit_u32(compiler, 0U) != 0) {
        return -1;
    }

    if (emit_u8(compiler, arg_count) != 0) {
        return -1;
    }

    return add_call_patch(compiler, name, patch_offset, arg_count);
}

static int parse_expression(PrismCompiler* compiler);
static int parse_statement(PrismCompiler* compiler, int* saw_return);

static int parse_declaration(PrismCompiler* compiler, int expect_semicolon) {
    char name[PRISMCC_MAX_TOKEN_TEXT];
    uint8_t local_index;

    if (expect(compiler, TOK_INT, "expected int") != 0) {
        return -1;
    }

    if (!token_is_name(compiler->lexer.current.type)) {
        compiler->error = "expected identifier after int";
        return -1;
    }

    copy_string(name, compiler->lexer.current.text, sizeof(name));
    next_token(compiler);

    if (add_local(compiler, name, &local_index) != 0) {
        return -1;
    }

    if (compiler->lexer.current.type == TOK_ASSIGN) {
        next_token(compiler);
        if (parse_expression(compiler) != 0) {
            return -1;
        }
    } else {
        if (emit_u8(compiler, BCVM_OP_PUSH_I32) != 0 || emit_u32(compiler, 0U) != 0) {
            return -1;
        }
    }

    if (emit_u8(compiler, BCVM_OP_STORE_LOCAL) != 0 || emit_u8(compiler, local_index) != 0) {
        return -1;
    }

    if (expect_semicolon) {
        return expect(compiler, TOK_SEMI, "expected ';' after declaration");
    }

    return 0;
}

static int parse_call_after_name(PrismCompiler* compiler, const char* name, int discard_return) {
    uint8_t arg_count = 0;

    if (expect(compiler, TOK_LPAREN, "expected '(' in function call") != 0) {
        return -1;
    }

    if (compiler->lexer.current.type != TOK_RPAREN) {
        while (1) {
            if (arg_count >= 255U) {
                compiler->error = "too many call arguments";
                return -1;
            }

            if (parse_expression(compiler) != 0) {
                return -1;
            }

            arg_count++;
            if (compiler->lexer.current.type != TOK_COMMA) {
                break;
            }

            next_token(compiler);
        }
    }

    if (expect(compiler, TOK_RPAREN, "expected ')' after call arguments") != 0) {
        return -1;
    }

    if (emit_call_by_name(compiler, name, arg_count) != 0) {
        return -1;
    }

    if (discard_return) {
        return emit_u8(compiler, BCVM_OP_POP);
    }

    return 0;
}

static int parse_assignment_after_name(PrismCompiler* compiler, const char* name) {
    int local_index = find_local(compiler, name);

    if (local_index < 0) {
        compiler->error = "assignment to unknown identifier";
        return -1;
    }

    if (expect(compiler, TOK_ASSIGN, "expected '=' in assignment") != 0) {
        return -1;
    }

    if (parse_expression(compiler) != 0) {
        return -1;
    }

    if (emit_u8(compiler, BCVM_OP_STORE_LOCAL) != 0 || emit_u8(compiler, (uint8_t)local_index) != 0) {
        return -1;
    }

    return 0;
}

static int parse_for_clause_item(PrismCompiler* compiler, int allow_declaration) {
    if (compiler->lexer.current.type == TOK_INT) {
        if (!allow_declaration) {
            compiler->error = "for increment does not support declaration";
            return -1;
        }
        return parse_declaration(compiler, 0);
    }

    if (token_is_name(compiler->lexer.current.type)) {
        char name[PRISMCC_MAX_TOKEN_TEXT];

        copy_string(name, compiler->lexer.current.text, sizeof(name));
        next_token(compiler);

        if (compiler->lexer.current.type == TOK_ASSIGN) {
            return parse_assignment_after_name(compiler, name);
        }

        if (compiler->lexer.current.type == TOK_LPAREN) {
            return parse_call_after_name(compiler, name, 1);
        }

        compiler->error = "unsupported for clause item";
        return -1;
    }

    if (parse_expression(compiler) != 0) {
        return -1;
    }

    return emit_u8(compiler, BCVM_OP_POP);
}

static int parse_primary(PrismCompiler* compiler) {
    Token token = compiler->lexer.current;

    if (token.type == TOK_INPUT_INT) {
        next_token(compiler);

        if (expect(compiler, TOK_LPAREN, "expected '(' after input_int") != 0) {
            return -1;
        }

        if (expect(compiler, TOK_RPAREN, "expected ')' after input_int") != 0) {
            return -1;
        }

        return emit_u8(compiler, BCVM_OP_READ_INT);
    }

    if (token.type == TOK_NUMBER) {
        if (emit_u8(compiler, BCVM_OP_PUSH_I32) != 0 || emit_u32(compiler, (uint32_t)token.number) != 0) {
            return -1;
        }

        next_token(compiler);
        return compiler->error == 0 ? 0 : -1;
    }

    if (token_is_name(token.type)) {
        char name[PRISMCC_MAX_TOKEN_TEXT];
        int local_index;

        copy_string(name, token.text, sizeof(name));
        next_token(compiler);

        if (compiler->lexer.current.type == TOK_LPAREN) {
            return parse_call_after_name(compiler, name, 0);
        }

        local_index = find_local(compiler, name);
        if (local_index < 0) {
            compiler->error = "unknown identifier";
            return -1;
        }

        if (emit_u8(compiler, BCVM_OP_LOAD_LOCAL) != 0 || emit_u8(compiler, (uint8_t)local_index) != 0) {
            return -1;
        }

        return 0;
    }

    if (token.type == TOK_LPAREN) {
        next_token(compiler);
        if (parse_expression(compiler) != 0) {
            return -1;
        }

        return expect(compiler, TOK_RPAREN, "expected ')' after expression");
    }

    compiler->error = "invalid expression";
    return -1;
}

static int parse_unary(PrismCompiler* compiler) {
    if (compiler->lexer.current.type == TOK_MINUS) {
        next_token(compiler);
        if (parse_unary(compiler) != 0) {
            return -1;
        }

        return emit_u8(compiler, BCVM_OP_NEG);
    }

    return parse_primary(compiler);
}

static int parse_term(PrismCompiler* compiler) {
    if (parse_unary(compiler) != 0) {
        return -1;
    }

    while (compiler->lexer.current.type == TOK_STAR || compiler->lexer.current.type == TOK_SLASH) {
        TokenType op = compiler->lexer.current.type;
        next_token(compiler);

        if (parse_unary(compiler) != 0) {
            return -1;
        }

        if (emit_u8(compiler, op == TOK_STAR ? BCVM_OP_MUL : BCVM_OP_DIV) != 0) {
            return -1;
        }
    }

    return compiler->error == 0 ? 0 : -1;
}

static int parse_additive(PrismCompiler* compiler) {
    if (parse_term(compiler) != 0) {
        return -1;
    }

    while (compiler->lexer.current.type == TOK_PLUS || compiler->lexer.current.type == TOK_MINUS) {
        TokenType op = compiler->lexer.current.type;
        next_token(compiler);

        if (parse_term(compiler) != 0) {
            return -1;
        }

        if (emit_u8(compiler, op == TOK_PLUS ? BCVM_OP_ADD : BCVM_OP_SUB) != 0) {
            return -1;
        }
    }

    return compiler->error == 0 ? 0 : -1;
}

static int parse_relational(PrismCompiler* compiler) {
    if (parse_additive(compiler) != 0) {
        return -1;
    }

    while (compiler->lexer.current.type == TOK_LT
        || compiler->lexer.current.type == TOK_LTE
        || compiler->lexer.current.type == TOK_GT
        || compiler->lexer.current.type == TOK_GTE) {
        TokenType op = compiler->lexer.current.type;
        next_token(compiler);

        if (parse_additive(compiler) != 0) {
            return -1;
        }

        if (op == TOK_LT) {
            if (emit_u8(compiler, BCVM_OP_LT) != 0) {
                return -1;
            }
        } else if (op == TOK_LTE) {
            if (emit_u8(compiler, BCVM_OP_LE) != 0) {
                return -1;
            }
        } else if (op == TOK_GT) {
            if (emit_u8(compiler, BCVM_OP_GT) != 0) {
                return -1;
            }
        } else {
            if (emit_u8(compiler, BCVM_OP_GE) != 0) {
                return -1;
            }
        }
    }

    return compiler->error == 0 ? 0 : -1;
}

static int parse_expression(PrismCompiler* compiler) {
    if (parse_relational(compiler) != 0) {
        return -1;
    }

    while (compiler->lexer.current.type == TOK_EQEQ || compiler->lexer.current.type == TOK_NEQ) {
        TokenType op = compiler->lexer.current.type;
        next_token(compiler);

        if (parse_relational(compiler) != 0) {
            return -1;
        }

        if (emit_u8(compiler, op == TOK_EQEQ ? BCVM_OP_EQ : BCVM_OP_NE) != 0) {
            return -1;
        }
    }

    return compiler->error == 0 ? 0 : -1;
}

static int parse_block(PrismCompiler* compiler, int* saw_return) {
    if (expect(compiler, TOK_LBRACE, "expected '{'") != 0) {
        return -1;
    }

    while (compiler->lexer.current.type != TOK_RBRACE && compiler->lexer.current.type != TOK_EOF) {
        if (parse_statement(compiler, saw_return) != 0) {
            return -1;
        }
    }

    return expect(compiler, TOK_RBRACE, "expected '}'");
}

static int parse_if_statement(PrismCompiler* compiler, int* saw_return) {
    uint32_t false_jump;

    next_token(compiler);
    if (expect(compiler, TOK_LPAREN, "expected '(' after if") != 0) {
        return -1;
    }

    if (parse_expression(compiler) != 0) {
        return -1;
    }

    if (expect(compiler, TOK_RPAREN, "expected ')' after if condition") != 0) {
        return -1;
    }

    if (emit_jump_placeholder(compiler, BCVM_OP_JMP_IF_ZERO, &false_jump) != 0) {
        return -1;
    }

    if (parse_statement(compiler, saw_return) != 0) {
        return -1;
    }

    if (compiler->lexer.current.type == TOK_ELSE) {
        uint32_t end_jump;

        if (emit_jump_placeholder(compiler, BCVM_OP_JMP, &end_jump) != 0) {
            return -1;
        }

        patch_u32_at(compiler, false_jump, compiler->code_size);

        next_token(compiler);
        if (parse_statement(compiler, saw_return) != 0) {
            return -1;
        }

        patch_u32_at(compiler, end_jump, compiler->code_size);
        return compiler->error == 0 ? 0 : -1;
    }

    patch_u32_at(compiler, false_jump, compiler->code_size);
    return compiler->error == 0 ? 0 : -1;
}

static int parse_while_statement(PrismCompiler* compiler, int* saw_return) {
    uint32_t cond_offset;
    uint32_t exit_jump;

    next_token(compiler);
    if (expect(compiler, TOK_LPAREN, "expected '(' after while") != 0) {
        return -1;
    }

    cond_offset = compiler->code_size;

    if (parse_expression(compiler) != 0) {
        return -1;
    }

    if (expect(compiler, TOK_RPAREN, "expected ')' after while condition") != 0) {
        return -1;
    }

    if (emit_jump_placeholder(compiler, BCVM_OP_JMP_IF_ZERO, &exit_jump) != 0) {
        return -1;
    }

    if (parse_statement(compiler, saw_return) != 0) {
        return -1;
    }

    if (emit_u8(compiler, BCVM_OP_JMP) != 0 || emit_u32(compiler, cond_offset) != 0) {
        return -1;
    }

    patch_u32_at(compiler, exit_jump, compiler->code_size);
    return compiler->error == 0 ? 0 : -1;
}

static int parse_for_statement(PrismCompiler* compiler, int* saw_return) {
    uint32_t cond_offset;
    uint32_t exit_jump;
    uint32_t body_jump;
    uint32_t inc_offset;
    uint32_t body_offset;

    next_token(compiler);
    if (expect(compiler, TOK_LPAREN, "expected '(' after for") != 0) {
        return -1;
    }

    if (compiler->lexer.current.type != TOK_SEMI) {
        if (parse_for_clause_item(compiler, 1) != 0) {
            return -1;
        }
    }

    if (expect(compiler, TOK_SEMI, "expected ';' after for init") != 0) {
        return -1;
    }

    cond_offset = compiler->code_size;

    if (compiler->lexer.current.type != TOK_SEMI) {
        if (parse_expression(compiler) != 0) {
            return -1;
        }
    } else {
        if (emit_u8(compiler, BCVM_OP_PUSH_I32) != 0 || emit_u32(compiler, 1U) != 0) {
            return -1;
        }
    }

    if (expect(compiler, TOK_SEMI, "expected ';' after for condition") != 0) {
        return -1;
    }

    if (emit_jump_placeholder(compiler, BCVM_OP_JMP_IF_ZERO, &exit_jump) != 0) {
        return -1;
    }

    if (emit_jump_placeholder(compiler, BCVM_OP_JMP, &body_jump) != 0) {
        return -1;
    }

    inc_offset = compiler->code_size;

    if (compiler->lexer.current.type != TOK_RPAREN) {
        if (parse_for_clause_item(compiler, 0) != 0) {
            return -1;
        }
    }

    if (expect(compiler, TOK_RPAREN, "expected ')' after for clauses") != 0) {
        return -1;
    }

    if (emit_u8(compiler, BCVM_OP_JMP) != 0 || emit_u32(compiler, cond_offset) != 0) {
        return -1;
    }

    body_offset = compiler->code_size;
    patch_u32_at(compiler, body_jump, body_offset);

    if (parse_statement(compiler, saw_return) != 0) {
        return -1;
    }

    if (emit_u8(compiler, BCVM_OP_JMP) != 0 || emit_u32(compiler, inc_offset) != 0) {
        return -1;
    }

    patch_u32_at(compiler, exit_jump, compiler->code_size);
    return compiler->error == 0 ? 0 : -1;
}

static int parse_statement(PrismCompiler* compiler, int* saw_return) {
    if (compiler->lexer.current.type == TOK_LBRACE) {
        return parse_block(compiler, saw_return);
    }

    if (compiler->lexer.current.type == TOK_INT) {
        return parse_declaration(compiler, 1);
    }

    if (compiler->lexer.current.type == TOK_PRINT) {
        Token str;
        uint16_t offset;
        uint16_t length;

        next_token(compiler);
        if (expect(compiler, TOK_LPAREN, "expected '(' after print") != 0) {
            return -1;
        }

        if (compiler->lexer.current.type != TOK_STRING) {
            compiler->error = "print requires string literal";
            return -1;
        }

        str = compiler->lexer.current;
        length = (uint16_t)string_length(str.text);
        if (add_data(compiler, str.text, length, &offset) != 0) {
            return -1;
        }

        next_token(compiler);
        if (expect(compiler, TOK_RPAREN, "expected ')' after print argument") != 0) {
            return -1;
        }

        if (expect(compiler, TOK_SEMI, "expected ';' after print") != 0) {
            return -1;
        }

        if (emit_u8(compiler, BCVM_OP_PRINT_STR) != 0 || emit_u16(compiler, offset) != 0 || emit_u16(compiler, length) != 0) {
            return -1;
        }

        return emit_u8(compiler, BCVM_OP_PRINT_NL);
    }

    if (compiler->lexer.current.type == TOK_PRINT_INT) {
        next_token(compiler);
        if (expect(compiler, TOK_LPAREN, "expected '(' after print_int") != 0) {
            return -1;
        }

        if (parse_expression(compiler) != 0) {
            return -1;
        }

        if (expect(compiler, TOK_RPAREN, "expected ')' after print_int argument") != 0) {
            return -1;
        }

        if (expect(compiler, TOK_SEMI, "expected ';' after print_int") != 0) {
            return -1;
        }

        if (emit_u8(compiler, BCVM_OP_PRINT_INT) != 0) {
            return -1;
        }

        return emit_u8(compiler, BCVM_OP_PRINT_NL);
    }

    if (compiler->lexer.current.type == TOK_IF) {
        return parse_if_statement(compiler, saw_return);
    }

    if (compiler->lexer.current.type == TOK_WHILE) {
        return parse_while_statement(compiler, saw_return);
    }

    if (compiler->lexer.current.type == TOK_FOR) {
        return parse_for_statement(compiler, saw_return);
    }

    if (compiler->lexer.current.type == TOK_RETURN) {
        next_token(compiler);

        if (parse_expression(compiler) != 0) {
            return -1;
        }

        if (expect(compiler, TOK_SEMI, "expected ';' after return") != 0) {
            return -1;
        }

        if (emit_u8(compiler, BCVM_OP_RET) != 0) {
            return -1;
        }

        *saw_return = 1;
        return 0;
    }

    if (token_is_name(compiler->lexer.current.type)) {
        char name[PRISMCC_MAX_TOKEN_TEXT];

        copy_string(name, compiler->lexer.current.text, sizeof(name));
        next_token(compiler);

        if (compiler->lexer.current.type == TOK_ASSIGN) {
            if (parse_assignment_after_name(compiler, name) != 0) {
                return -1;
            }

            return expect(compiler, TOK_SEMI, "expected ';' after assignment");
        }

        if (compiler->lexer.current.type == TOK_LPAREN) {
            if (parse_call_after_name(compiler, name, 1) != 0) {
                return -1;
            }

            return expect(compiler, TOK_SEMI, "expected ';' after function call");
        }

        compiler->error = "unsupported statement";
        return -1;
    }

    compiler->error = "unsupported statement";
    return -1;
}

static int parse_function(PrismCompiler* compiler) {
    char function_name[PRISMCC_MAX_TOKEN_TEXT];
    uint8_t param_count = 0;
    int saw_return = 0;

    if (expect(compiler, TOK_INT, "expected int at function start") != 0) {
        return -1;
    }

    if (!token_is_name(compiler->lexer.current.type)) {
        compiler->error = "expected function name";
        return -1;
    }

    copy_string(function_name, compiler->lexer.current.text, sizeof(function_name));
    next_token(compiler);

    if (expect(compiler, TOK_LPAREN, "expected '(' after function name") != 0) {
        return -1;
    }

    compiler->local_count = 0;

    if (compiler->lexer.current.type != TOK_RPAREN) {
        while (1) {
            char param_name[PRISMCC_MAX_TOKEN_TEXT];
            uint8_t ignored_index;

            if (expect(compiler, TOK_INT, "expected int parameter type") != 0) {
                return -1;
            }

            if (!token_is_name(compiler->lexer.current.type)) {
                compiler->error = "expected parameter name";
                return -1;
            }

            copy_string(param_name, compiler->lexer.current.text, sizeof(param_name));
            next_token(compiler);

            if (add_local(compiler, param_name, &ignored_index) != 0) {
                return -1;
            }

            param_count++;
            if (compiler->lexer.current.type != TOK_COMMA) {
                break;
            }

            next_token(compiler);
        }
    }

    if (expect(compiler, TOK_RPAREN, "expected ')' after parameter list") != 0) {
        return -1;
    }

    if (add_function(compiler, function_name, compiler->code_size, param_count) != 0) {
        return -1;
    }

    if (string_equals(function_name, "main")) {
        compiler->main_entry = compiler->code_size;
    }

    if (parse_block(compiler, &saw_return) != 0) {
        return -1;
    }

    if (!saw_return) {
        if (emit_u8(compiler, BCVM_OP_PUSH_I32) != 0 || emit_u32(compiler, 0U) != 0 || emit_u8(compiler, BCVM_OP_RET) != 0) {
            return -1;
        }
    }

    return 0;
}

static int resolve_call_patches(PrismCompiler* compiler) {
    for (uint32_t i = 0; i < compiler->call_patch_count; i++) {
        int function_index = find_function(compiler, compiler->call_patches[i].callee);
        uint32_t function_uindex;

        if (function_index < 0) {
            compiler->error = "call to unknown function";
            return -1;
        }

        function_uindex = (uint32_t)function_index;
        if (compiler->functions[function_uindex].param_count != compiler->call_patches[i].arg_count) {
            compiler->error = "function argument count mismatch";
            return -1;
        }

        patch_u32_at(compiler,
            compiler->call_patches[i].code_immediate_offset,
            compiler->functions[function_uindex].entry_offset);

        if (compiler->error != 0) {
            return -1;
        }
    }

    return 0;
}

static int parse_program(PrismCompiler* compiler) {
    while (compiler->lexer.current.type != TOK_EOF) {
        if (compiler->lexer.current.type != TOK_INT) {
            compiler->error = "top-level items must be int methods";
            return -1;
        }

        if (parse_function(compiler) != 0) {
            return -1;
        }
    }

    if (compiler->main_entry == 0xFFFFFFFFU) {
        compiler->error = "missing main function";
        return -1;
    }

    return resolve_call_patches(compiler);
}

static void write_u16le(uint8_t* out, uint16_t value) {
    out[0] = (uint8_t)(value & 0xFFU);
    out[1] = (uint8_t)((value >> 8) & 0xFFU);
}

static void write_u32le(uint8_t* out, uint32_t value) {
    out[0] = (uint8_t)(value & 0xFFU);
    out[1] = (uint8_t)((value >> 8) & 0xFFU);
    out[2] = (uint8_t)((value >> 16) & 0xFFU);
    out[3] = (uint8_t)((value >> 24) & 0xFFU);
}

int prismcc_compile_file(const char* input_abs_path, const char* output_abs_path, char* out_error, uint32_t out_error_capacity) {
    uint32_t source_size = 0;
    uint32_t vm_image_size;
    uint32_t total_size;

    if (input_abs_path == 0 || output_abs_path == 0) {
        set_error(out_error, out_error_capacity, "invalid compiler paths");
        return -1;
    }

    if (vfs_read_file(input_abs_path, source_buffer, sizeof(source_buffer), &source_size) != 0) {
        set_error(out_error, out_error_capacity, "failed to read source file");
        return -1;
    }

    compiler.lexer.src = source_buffer;
    compiler.lexer.length = source_size;
    compiler.lexer.position = 0;
    compiler.lexer.current.type = TOK_EOF;
    compiler.code_size = 0;
    compiler.data_size = 0;
    compiler.local_count = 0;
    compiler.function_count = 0;
    compiler.call_patch_count = 0;
    compiler.main_entry = 0xFFFFFFFFU;
    compiler.error = 0;

    next_token(&compiler);

    if (compiler.error == 0 && parse_program(&compiler) != 0) {
        if (compiler.error == 0) {
            compiler.error = "compile failed";
        }
    }

    if (compiler.error != 0) {
        ERROR_LOG("in-OS prismcc compile failed");
        set_error(out_error, out_error_capacity, compiler.error);
        return -1;
    }

    vm_image_size = BCVM_IMAGE_HEADER_SIZE + compiler.code_size + compiler.data_size;
    total_size = PRISM_APP_HEADER_SIZE + vm_image_size;

    write_u32le(&output_buffer[0], PRISM_APP_MAGIC);
    write_u16le(&output_buffer[4], PRISM_APP_FORMAT_VERSION);
    write_u16le(&output_buffer[6], 0U);
    write_u32le(&output_buffer[8], 0U);
    write_u32le(&output_buffer[12], vm_image_size);
    write_u32le(&output_buffer[16], 0U);

    write_u32le(&output_buffer[PRISM_APP_HEADER_SIZE + 0U], BCVM_MAGIC);
    write_u16le(&output_buffer[PRISM_APP_HEADER_SIZE + 4U], BCVM_VERSION);
    write_u16le(&output_buffer[PRISM_APP_HEADER_SIZE + 6U], 0U);
    write_u32le(&output_buffer[PRISM_APP_HEADER_SIZE + 8U], compiler.code_size);
    write_u32le(&output_buffer[PRISM_APP_HEADER_SIZE + 12U], compiler.data_size);
    write_u32le(&output_buffer[PRISM_APP_HEADER_SIZE + 16U], compiler.main_entry);

    for (uint32_t i = 0; i < compiler.code_size; i++) {
        output_buffer[PRISM_APP_HEADER_SIZE + BCVM_IMAGE_HEADER_SIZE + i] = compiler.code[i];
    }

    for (uint32_t i = 0; i < compiler.data_size; i++) {
        output_buffer[PRISM_APP_HEADER_SIZE + BCVM_IMAGE_HEADER_SIZE + compiler.code_size + i] = compiler.data[i];
    }

    if (vfs_write_file(output_abs_path, (const char*)output_buffer, total_size, 0) != 0) {
        set_error(out_error, out_error_capacity, "failed to write app output");
        return -1;
    }

    DEBUG_LOG("in-OS prismcc compile completed");
    set_error(out_error, out_error_capacity, "ok");
    return 0;
}
