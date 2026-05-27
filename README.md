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

If you want a reproducible cross-toolchain build, replace the host `gcc` invocations with an i686-elf cross-compiler and adjust the Makefile accordingly.

## COM1 Debugging (comport)

PrismOS includes a `comport` shell command that writes debug text to COM1.

1. Start with serial attached:

```shell
make run-serial
```

2. In PrismOS shell, send debug text:

```text
comport hello from kernel shell
```

3. Watch your host terminal for the COM1 output.

For persistent logs instead of terminal output:

```shell
make run-serial-log
tail -f build/serial.log
```

## Logging Layer

PrismOS now provides macro-based serial logging in `src/debug/log.h`:

- `DEBUG_LOG("message")`
- `WARNINIG_LOG("message")`
- `ERROR_LOG("message")`

The logs are written to COM1 and include level, file, and line.

Example:

```c
DEBUG_LOG("entered scheduler tick");
WARNINIG_LOG("unexpected device response");
ERROR_LOG("allocator returned null");
```

# Developing and Contributing to PrismOS
> [!IMPORTANT]
> All code you commit must be clean and have comments.
> For development, edit sources under `src/`, then re-run `make run`.
