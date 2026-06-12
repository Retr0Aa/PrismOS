#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apps/app_format.h"

#define MAX_SOURCE_SIZE (128U * 1024U)
#define MAX_TOK_TEXT 128U
#define MAX_CODE_SIZE (64U * 1024U)
#define MAX_DATA_SIZE (32U * 1024U)
#define MAX_LOCALS 64U

typedef enum {
    TOK_EOF = 0,
    TOK_INT,
    TOK_CHAR,
    TOK_MAIN,
    TOK_RETURN,
    TOK_PRINT,
    TOK_PRINT_INT,
    TOK_PRINT_CHAR,
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
    TOK_SLASH
} TokenType;

typedef struct {
    TokenType type;
    char text[MAX_TOK_TEXT];
    int32_t number;
    uint32_t line;
} Token;

typedef struct {
    const char* src;
    uint32_t len;
    uint32_t pos;
    uint32_t line;
    Token current;
} Lexer;

typedef struct {
    char name[32];
    uint8_t index;
} Local;

typedef struct {
    uint8_t code[MAX_CODE_SIZE];
    uint32_t code_size;
    uint8_t data[MAX_DATA_SIZE];
    uint32_t data_size;
    Local locals[MAX_LOCALS];
    uint32_t local_count;
    Lexer lexer;
} Compiler;

static void fail_at_line(uint32_t line, const char* message) {
    fprintf(stderr, "prismcc:%u: %s\n", line, message);
    exit(1);
}

static void fail(const char* message) {
    fprintf(stderr, "prismcc: %s\n", message);
    exit(1);
}

static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static void lexer_skip_ws(Lexer* lx) {
    while (lx->pos < lx->len) {
        char c = lx->src[lx->pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            lx->pos++;
            continue;
        }

        if (c == '\n') {
            lx->line++;
            lx->pos++;
            continue;
        }

        if (c == '/' && lx->pos + 1U < lx->len && lx->src[lx->pos + 1U] == '/') {
            lx->pos += 2U;
            while (lx->pos < lx->len && lx->src[lx->pos] != '\n') {
                lx->pos++;
            }
            continue;
        }

        break;
    }
}

static TokenType keyword_type(const char* text) {
    if (strcmp(text, "int") == 0) return TOK_INT;
    if (strcmp(text, "char") == 0) return TOK_CHAR;
    if (strcmp(text, "main") == 0) return TOK_MAIN;
    if (strcmp(text, "return") == 0) return TOK_RETURN;
    if (strcmp(text, "print") == 0) return TOK_PRINT;
    if (strcmp(text, "print_int") == 0) return TOK_PRINT_INT;
    if (strcmp(text, "print_char") == 0) return TOK_PRINT_CHAR;
    return TOK_IDENTIFIER;
}

static Token lexer_next(Lexer* lx) {
    Token tk;
    memset(&tk, 0, sizeof(tk));

    lexer_skip_ws(lx);
    tk.line = lx->line;

    if (lx->pos >= lx->len) {
        tk.type = TOK_EOF;
        return tk;
    }

    {
        char c = lx->src[lx->pos++];
        switch (c) {
            case '(': tk.type = TOK_LPAREN; return tk;
            case ')': tk.type = TOK_RPAREN; return tk;
            case '{': tk.type = TOK_LBRACE; return tk;
            case '}': tk.type = TOK_RBRACE; return tk;
            case ';': tk.type = TOK_SEMI; return tk;
            case '=': tk.type = TOK_ASSIGN; return tk;
            case ',': tk.type = TOK_COMMA; return tk;
            case '+': tk.type = TOK_PLUS; return tk;
            case '-': tk.type = TOK_MINUS; return tk;
            case '*': tk.type = TOK_STAR; return tk;
            case '/': tk.type = TOK_SLASH; return tk;
            case '"': {
                uint32_t out = 0;
                while (lx->pos < lx->len) {
                    char sc = lx->src[lx->pos++];
                    if (sc == '"') {
                        break;
                    }

                    if (sc == '\\') {
                        if (lx->pos >= lx->len) {
                            fail_at_line(tk.line, "unterminated string escape");
                        }

                        sc = lx->src[lx->pos++];
                        if (sc == 'n') sc = '\n';
                        else if (sc == 't') sc = '\t';
                    }

                    if (out + 1U >= MAX_TOK_TEXT) {
                        fail_at_line(tk.line, "string literal too long");
                    }
                    tk.text[out++] = sc;
                }

                tk.text[out] = '\0';
                tk.type = TOK_STRING;
                return tk;
            }
            case '\'': {
                char sc;

                if (lx->pos >= lx->len) {
                    fail_at_line(tk.line, "unterminated char literal");
                }

                if (lx->src[lx->pos] == '\'') {
                    fail_at_line(tk.line, "char literal must contain one character");
                }

                sc = lx->src[lx->pos++];
                if (sc == '\\') {
                    if (lx->pos >= lx->len) {
                        fail_at_line(tk.line, "unterminated char literal escape");
                    }

                    sc = lx->src[lx->pos++];
                    if (sc == 'n') sc = '\n';
                    else if (sc == 't') sc = '\t';
                    else if (sc == '\\') sc = '\\';
                    else if (sc == '\'') sc = '\'';
                }

                if (lx->pos >= lx->len || lx->src[lx->pos] != '\'') {
                    fail_at_line(tk.line, "char literal must contain one character");
                }

                lx->pos++;
                tk.type = TOK_NUMBER;
                tk.number = (int32_t)(uint8_t)sc;
                return tk;
            }
            default:
                if (is_alpha(c)) {
                    uint32_t out = 0;
                    tk.text[out++] = c;
                    while (lx->pos < lx->len && (is_alpha(lx->src[lx->pos]) || is_digit(lx->src[lx->pos]))) {
                        if (out + 1U >= MAX_TOK_TEXT) {
                            fail_at_line(tk.line, "identifier too long");
                        }
                        tk.text[out++] = lx->src[lx->pos++];
                    }
                    tk.text[out] = '\0';
                    tk.type = keyword_type(tk.text);
                    return tk;
                }

                if (is_digit(c)) {
                    int32_t value = (int32_t)(c - '0');
                    while (lx->pos < lx->len && is_digit(lx->src[lx->pos])) {
                        value = value * 10 + (int32_t)(lx->src[lx->pos] - '0');
                        lx->pos++;
                    }
                    tk.type = TOK_NUMBER;
                    tk.number = value;
                    return tk;
                }

                fail_at_line(tk.line, "unexpected character");
        }
    }

    tk.type = TOK_EOF;
    return tk;
}

static void next_token(Compiler* c) {
    c->lexer.current = lexer_next(&c->lexer);
}

static void expect(Compiler* c, TokenType type, const char* msg) {
    if (c->lexer.current.type != type) {
        fail_at_line(c->lexer.current.line, msg);
    }
    next_token(c);
}

static void emit_u8(Compiler* c, uint8_t b) {
    if (c->code_size >= MAX_CODE_SIZE) {
        fail("bytecode buffer full");
    }
    c->code[c->code_size++] = b;
}

static void emit_u16(Compiler* c, uint16_t v) {
    emit_u8(c, (uint8_t)(v & 0xFFU));
    emit_u8(c, (uint8_t)((v >> 8) & 0xFFU));
}

static void emit_u32(Compiler* c, uint32_t v) {
    emit_u8(c, (uint8_t)(v & 0xFFU));
    emit_u8(c, (uint8_t)((v >> 8) & 0xFFU));
    emit_u8(c, (uint8_t)((v >> 16) & 0xFFU));
    emit_u8(c, (uint8_t)((v >> 24) & 0xFFU));
}

static uint16_t add_data(Compiler* c, const char* bytes, uint16_t len) {
    uint16_t offset;
    if (c->data_size + (uint32_t)len > MAX_DATA_SIZE) {
        fail("data section full");
    }

    offset = (uint16_t)c->data_size;
    memcpy(&c->data[c->data_size], bytes, len);
    c->data_size += len;
    return offset;
}

static int find_local(Compiler* c, const char* name) {
    for (uint32_t i = 0; i < c->local_count; i++) {
        if (strcmp(c->locals[i].name, name) == 0) {
            return (int)c->locals[i].index;
        }
    }
    return -1;
}

static uint8_t add_local(Compiler* c, const char* name) {
    if (c->local_count >= MAX_LOCALS) {
        fail("too many locals");
    }

    if (strlen(name) >= sizeof(c->locals[0].name)) {
        fail("local name too long");
    }

    strcpy(c->locals[c->local_count].name, name);
    c->locals[c->local_count].index = (uint8_t)c->local_count;
    c->local_count++;
    return (uint8_t)(c->local_count - 1U);
}

static void parse_expression(Compiler* c);

static void parse_primary(Compiler* c) {
    Token tk = c->lexer.current;

    if (tk.type == TOK_NUMBER) {
        emit_u8(c, BCVM_OP_PUSH_I32);
        emit_u32(c, (uint32_t)tk.number);
        next_token(c);
        return;
    }

    if (tk.type == TOK_IDENTIFIER) {
        int idx = find_local(c, tk.text);
        if (idx < 0) {
            fail_at_line(tk.line, "unknown identifier");
        }

        emit_u8(c, BCVM_OP_LOAD_LOCAL);
        emit_u8(c, (uint8_t)idx);
        next_token(c);
        return;
    }

    if (tk.type == TOK_LPAREN) {
        next_token(c);
        parse_expression(c);
        expect(c, TOK_RPAREN, "expected ')' after expression");
        return;
    }

    fail_at_line(tk.line, "invalid expression");
}

static void parse_unary(Compiler* c) {
    if (c->lexer.current.type == TOK_MINUS) {
        next_token(c);
        parse_unary(c);
        emit_u8(c, BCVM_OP_NEG);
        return;
    }

    parse_primary(c);
}

static void parse_term(Compiler* c) {
    parse_unary(c);

    while (c->lexer.current.type == TOK_STAR || c->lexer.current.type == TOK_SLASH) {
        TokenType op = c->lexer.current.type;
        next_token(c);
        parse_unary(c);
        emit_u8(c, op == TOK_STAR ? BCVM_OP_MUL : BCVM_OP_DIV);
    }
}

static void parse_expression(Compiler* c) {
    parse_term(c);

    while (c->lexer.current.type == TOK_PLUS || c->lexer.current.type == TOK_MINUS) {
        TokenType op = c->lexer.current.type;
        next_token(c);
        parse_term(c);
        emit_u8(c, op == TOK_PLUS ? BCVM_OP_ADD : BCVM_OP_SUB);
    }
}

static void parse_statement(Compiler* c, int* saw_return) {
    if (c->lexer.current.type == TOK_INT || c->lexer.current.type == TOK_CHAR) {
        Token ident;
        uint8_t idx;
        const char* type_name = c->lexer.current.type == TOK_CHAR ? "char" : "int";
        next_token(c);

        if (c->lexer.current.type != TOK_IDENTIFIER) {
            char error_msg[64];
            snprintf(error_msg, sizeof(error_msg), "expected identifier after %s", type_name);
            fail_at_line(c->lexer.current.line, error_msg);
        }
        ident = c->lexer.current;
        next_token(c);

        idx = add_local(c, ident.text);
        if (c->lexer.current.type == TOK_ASSIGN) {
            next_token(c);
            parse_expression(c);
        } else {
            emit_u8(c, BCVM_OP_PUSH_I32);
            emit_u32(c, 0U);
        }

        emit_u8(c, BCVM_OP_STORE_LOCAL);
        emit_u8(c, idx);
        expect(c, TOK_SEMI, "expected ';' after declaration");
        return;
    }

    if (c->lexer.current.type == TOK_PRINT) {
        Token str;
        uint16_t off;
        uint16_t len;

        next_token(c);
        expect(c, TOK_LPAREN, "expected '(' after print");
        if (c->lexer.current.type != TOK_STRING) {
            fail_at_line(c->lexer.current.line, "print requires string literal");
        }

        str = c->lexer.current;
        len = (uint16_t)strlen(str.text);
        off = add_data(c, str.text, len);
        next_token(c);
        expect(c, TOK_RPAREN, "expected ')' after print argument");
        expect(c, TOK_SEMI, "expected ';' after print");

        emit_u8(c, BCVM_OP_PRINT_STR);
        emit_u16(c, off);
        emit_u16(c, len);
        emit_u8(c, BCVM_OP_PRINT_NL);
        return;
    }

    if (c->lexer.current.type == TOK_PRINT_INT) {
        next_token(c);
        expect(c, TOK_LPAREN, "expected '(' after print_int");
        parse_expression(c);
        expect(c, TOK_RPAREN, "expected ')' after print_int argument");
        expect(c, TOK_SEMI, "expected ';' after print_int");
        emit_u8(c, BCVM_OP_PRINT_INT);
        emit_u8(c, BCVM_OP_PRINT_NL);
        return;
    }

    if (c->lexer.current.type == TOK_PRINT_CHAR) {
        next_token(c);
        expect(c, TOK_LPAREN, "expected '(' after print_char");
        parse_expression(c);
        expect(c, TOK_RPAREN, "expected ')' after print_char argument");
        expect(c, TOK_SEMI, "expected ';' after print_char");
        emit_u8(c, BCVM_OP_PRINT_CHAR);
        return;
    }

    if (c->lexer.current.type == TOK_RETURN) {
        next_token(c);
        parse_expression(c);
        expect(c, TOK_SEMI, "expected ';' after return");
        emit_u8(c, BCVM_OP_RET);
        *saw_return = 1;
        return;
    }

    if (c->lexer.current.type == TOK_IDENTIFIER) {
        Token ident = c->lexer.current;
        int idx;

        next_token(c);
        expect(c, TOK_ASSIGN, "expected '=' in assignment");
        idx = find_local(c, ident.text);
        if (idx < 0) {
            fail_at_line(ident.line, "assignment to unknown identifier");
        }

        parse_expression(c);
        expect(c, TOK_SEMI, "expected ';' after assignment");
        emit_u8(c, BCVM_OP_STORE_LOCAL);
        emit_u8(c, (uint8_t)idx);
        return;
    }

    fail_at_line(c->lexer.current.line, "unsupported statement");
}

static void parse_program(Compiler* c) {
    int saw_return = 0;

    expect(c, TOK_INT, "expected 'int' at program start");
    expect(c, TOK_MAIN, "expected 'main' function");
    expect(c, TOK_LPAREN, "expected '(' after main");
    expect(c, TOK_RPAREN, "expected ')' after main");
    expect(c, TOK_LBRACE, "expected '{' to start function body");

    while (c->lexer.current.type != TOK_RBRACE && c->lexer.current.type != TOK_EOF) {
        parse_statement(c, &saw_return);
    }

    expect(c, TOK_RBRACE, "expected '}' to close function body");
    expect(c, TOK_EOF, "unexpected tokens after function");

    if (!saw_return) {
        emit_u8(c, BCVM_OP_PUSH_I32);
        emit_u32(c, 0U);
        emit_u8(c, BCVM_OP_RET);
    }
}

static uint8_t* read_file_all(const char* path, uint32_t* out_size) {
    FILE* f;
    long size;
    uint8_t* data;

    f = fopen(path, "rb");
    if (!f) {
        fail("failed to open input file");
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        fail("failed to seek input file");
    }

    size = ftell(f);
    if (size < 0 || (uint32_t)size > MAX_SOURCE_SIZE) {
        fclose(f);
        fail("input too large");
    }

    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        fail("failed to rewind input file");
    }

    data = (uint8_t*)malloc((size_t)size + 1U);
    if (!data) {
        fclose(f);
        fail("out of memory");
    }

    if (fread(data, 1, (size_t)size, f) != (size_t)size) {
        free(data);
        fclose(f);
        fail("failed to read input file");
    }

    fclose(f);
    data[size] = '\0';
    *out_size = (uint32_t)size;
    return data;
}

static void write_u16le(uint8_t* out, uint16_t v) {
    out[0] = (uint8_t)(v & 0xFFU);
    out[1] = (uint8_t)((v >> 8) & 0xFFU);
}

static void write_u32le(uint8_t* out, uint32_t v) {
    out[0] = (uint8_t)(v & 0xFFU);
    out[1] = (uint8_t)((v >> 8) & 0xFFU);
    out[2] = (uint8_t)((v >> 16) & 0xFFU);
    out[3] = (uint8_t)((v >> 24) & 0xFFU);
}

static void write_output(const char* path, const Compiler* c) {
    uint8_t prism_header[PRISM_APP_HEADER_SIZE];
    uint8_t vm_header[BCVM_IMAGE_HEADER_SIZE];
    FILE* f;
    uint32_t vm_image_size = BCVM_IMAGE_HEADER_SIZE + c->code_size + c->data_size;

    memset(prism_header, 0, sizeof(prism_header));
    write_u32le(&prism_header[0], PRISM_APP_MAGIC);
    write_u16le(&prism_header[4], PRISM_APP_FORMAT_VERSION);
    write_u16le(&prism_header[6], 0U);
    write_u32le(&prism_header[8], 0U);
    write_u32le(&prism_header[12], vm_image_size);
    write_u32le(&prism_header[16], 0U);

    memset(vm_header, 0, sizeof(vm_header));
    write_u32le(&vm_header[0], BCVM_MAGIC);
    write_u16le(&vm_header[4], BCVM_VERSION);
    write_u16le(&vm_header[6], 0U);
    write_u32le(&vm_header[8], c->code_size);
    write_u32le(&vm_header[12], c->data_size);
    write_u32le(&vm_header[16], 0U);

    f = fopen(path, "wb");
    if (!f) {
        fail("failed to open output file");
    }

    fwrite(prism_header, 1, sizeof(prism_header), f);
    fwrite(vm_header, 1, sizeof(vm_header), f);
    fwrite(c->code, 1, c->code_size, f);
    fwrite(c->data, 1, c->data_size, f);
    fclose(f);
}

int main(int argc, char** argv) {
    Compiler c;
    uint8_t* source;
    uint32_t source_size = 0;

    if (argc != 3) {
        fprintf(stderr, "Usage: prismcc <input.c> <output.app>\n");
        return 1;
    }

    memset(&c, 0, sizeof(c));
    source = read_file_all(argv[1], &source_size);

    c.lexer.src = (const char*)source;
    c.lexer.len = source_size;
    c.lexer.pos = 0;
    c.lexer.line = 1;
    next_token(&c);

    parse_program(&c);
    write_output(argv[2], &c);

    free(source);
    printf("prismcc: wrote %s (code=%u bytes, data=%u bytes)\n", argv[2], c.code_size, c.data_size);
    return 0;
}
