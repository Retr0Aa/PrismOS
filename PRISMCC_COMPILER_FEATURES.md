# PrismCC Compiler Features

This document describes the current feature set of the in-OS PrismCC compiler and BCVM runtime.

## Overview

- Compiler: `src/apps/prismcc_runtime.c`
- VM runtime: `src/apps/bytecode_vm.c`
- Bytecode format/opcodes: `src/apps/app_format.h`
- Shell integration command: `cc <input.c> <output.app>`

PrismCC compiles a C-like subset into BCVM bytecode packaged in the Prism app format.

## Language: Supported Types

- `int`
- `string`
- `int[N]` fixed-size arrays
- `struct` types with `int`/`string` fields

Notes:
- Arrays are currently int-only.
- String values are first-class VM values (descriptors).
- Local variable limit per function: 64.

## Language: Functions and Calls

- User-defined functions are supported.
- Function parameters are supported (`int` and `string`).
- Return values are supported.
- Method overloading is supported by signature:
  - same name
  - different parameter type list
- Overload resolution is performed during compile/patch stage.

## Preprocessor and Includes

- Supports textual includes via:
  - `#include "relative/path.h"`
  - `#include <relative/or/absolute/path.h>`
- Include paths are resolved relative to the including file when not absolute.
- Header files (`.h`) are supported as normal include targets.
- Includes are expanded before lexing/parsing (single translation unit).
- Cyclic includes are rejected.

## Language: Statements and Control Flow

- Variable declarations:
  - `int x;`
  - `int x = 5;`
  - `string s;`
  - `string s = "hi";`
  - `int a[8];`
  - `struct Person p;`
- Assignment:
  - scalar assignment (`x = expr;`)
  - array element assignment (`a[i] = expr;`)
  - struct field assignment (`p.name = "neo";`, `p.age = 7;`)
- `if` / `else`
- `while`
- `for (init; condition; increment)`
- `return expr;`

## Language: Expressions and Operators

- Integer arithmetic:
  - `+`, `-`, `*`, `/`, `%`
  - unary `+`, unary `-`
  - `++a`, `--a`, `a++`, `a--` for `int` locals
- Integer comparisons:
  - `==`, `!=`, `<`, `<=`, `>`, `>=`
- Function call expressions
- Parenthesized expressions
- String literals
- Array element reads (`a[i]`)
- Struct field reads (`p.name`, `p.age`)

## Structs

- Type definition syntax:
  - `struct Person { int age; string name; };`
- Field types currently supported in structs:
  - `int`
  - `string`
- Struct variables are value containers backed by VM heap arrays.
- Struct initializers are not supported yet (declare first, then assign fields).

## Built-ins: Console and Input

- `print(expr)`
  - overloaded behavior:
    - prints int when expression is int
    - prints string when expression is string
  - prints newline after output
- `print_int(expr)`
- `print_color(colorExpr, stringExpr)`
- `read_text()`
- `read_text(promptStringExpr)`
- `input_int()`
- `print_input()`
- `input_len()`
- `input_eq("literal")`

## Built-ins: String

- `string_len(stringExpr)`
- `string_eq(stringExprA, stringExprB)`

## Built-ins: Filesystem

- `file_read(pathStringExpr)` -> returns string
- `file_write(pathStringExpr, textStringExpr);` -> status is produced and discarded in statement form
- `file_append(pathStringExpr, textStringExpr);` -> status is produced and discarded in statement form
- `file_exists(pathStringExpr)` -> returns int (`1` exists, `0` does not exist)

Path strings are passed directly to VFS operations in the VM.

## Arrays

- Fixed-size declaration with constant positive size (`int a[16];`)
- Zero-initialized when created
- Indexed read/write:
  - `x = a[i];`
  - `a[i] = x;`

Runtime behavior:
- Array storage is VM heap backed (handle-based)
- Out-of-range access fails VM execution

## Compiler and Runtime Limits

Compiler-side limits:
- Source size: 32 KiB
- Code size: 48 KiB
- Data size: 16 KiB
- Max locals: 512
- Max functions: 64
- Max call patch sites: 256

VM-side limits:
- Stack entries: 256
- Call depth: 32
- Max VM steps per run: 200000
- Input buffer: 127 chars
- Heap string slots: 64 (data pool 4096 bytes)
- Heap array slots: 64 (cell pool 1024 int cells)

## Shell Workflow

1. Write source file (for example under `/examples`).
  - You can split reusable code into headers and include them.
2. Compile in PrismOS shell:
   - `cc /path/program.c /path/program.app`
3. Run app:
   - `app-run /path/program.app`

### Small Standard Library (Headers)

The repository now includes a small header-only standard library under `/examples/std`:

- `/examples/std/prism_string.h`
  - `std_str_len`, `std_str_eq`, emptiness and length checks
- `/examples/std/prism_math.h`
  - arithmetic wrappers, `std_abs`, `std_min`, `std_max`, `std_clamp`, `std_pow2`
- `/examples/std/prism_io.h`
  - print/read helpers and filesystem wrappers

Usage example:

- `#include "std/prism_string.h"`
- `#include "std/prism_math.h"`
- `#include "std/prism_io.h"`

## Current Known Gaps

- No unions/enums
- No pointers
- No floating-point types
- No global variables
- No module/import system
- No separate object/link pipeline yet (includes are textual expansion)
- No dynamic array type beyond fixed `int[N]`
- No struct fields of type array or nested struct yet
- No string interpolation/format library yet
- Filesystem built-ins currently use simple status returns; richer error model is pending
