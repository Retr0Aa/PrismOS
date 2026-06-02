# PrismOS

PrismOS is a small hobby operating system that boots via GRUB and runs on x86 hardware (or in QEMU). It provides a simple framebuffer console, a basic shell, and a handful of built-in commands.

## Quick build & run

Install the build dependencies (example for Debian/Ubuntu):

```shell
sudo apt-get install build-essential nasm gcc-multilib xorriso qemu-system-x86 grub-common grub-pc-bin mtools
```

Build and run:

```shell
make run           # build (if needed) and boot the ISO in QEMU
```

Other useful targets:

```shell
make clean         # remove build artifacts
make run-serial    # boot and mirror COM1 to your terminal
make run-serial-log # boot and save COM1 output to build/serial.log
```

Notes:
- The build uses 32-bit compilation flags (gcc -m32). Ensure you have multilib support installed.
- `make run` launches QEMU for quick testing; you can also boot `build/os.iso` in a BIOS-mode VM.
- `make run-serial` launches QEMU with debug logs. If you are running bare-metal use serial port.

If you want a reproducible cross-toolchain build, replace the host `gcc` invocations with an i686-elf cross-compiler and adjust the Makefile accordingly.


# Developing and Contributing to PrismOS
> [!IMPORTANT]
> All code you commit must be clean and have comments.
> For development, edit sources under `src/`, then re-run `make run`.

## Subset C Compiler (prismcc)

PrismOS includes a lightweight subset-C pipeline that targets a bytecode VM inside the OS.

Build the compiler:

```shell
make prismcc
```

Compile a program to a Prism app package:

```shell
cat > hello.c <<'EOF'
int main() {
		int a = 2 + 3;
		print("Hello from prismcc");
		print_int(a * 10);
		return 0;
}
EOF

./build/prismcc hello.c hello.app
```

Copy the generated app to PrismOS disk content (8.3 FAT name recommended), then run in PrismOS shell:

```text
app-run /HELLO.APP
```

### Supported Subset

- One function: `int main() { ... }`
- Statements:
	- `int x = expr;`
	- `x = expr;`
	- `print("text");`
	- `print_int(expr);`
	- `return expr;`
- Expressions: integer literals, variables, `+ - * /`, unary `-`, parentheses

The compiler emits Prism app containers with a BCVM bytecode payload, and PrismOS runs them through the in-kernel bytecode VM.

## Native Compiler Inside PrismOS

PrismOS now includes an in-OS subset C compiler command:

```text
cc <input.c> <output.app>
```

Example flow from PrismOS shell:

```text
edit /HELLO.C
cc /HELLO.C /HELLO.APP
app-run /HELLO.APP
```

This uses the same subset language as `prismcc` and compiles directly on the PrismOS filesystem.
