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
```

Notes:
- The build uses 32-bit compilation flags (gcc -m32). Ensure you have multilib support installed.
- `make run` launches QEMU for quick testing; you can also boot `build/os.iso` in a BIOS-mode VM.

If you want a reproducible cross-toolchain build, replace the host `gcc` invocations with an i686-elf cross-compiler and adjust the Makefile accordingly.

# Developing and Contributing to PrismOS
> [!IMPORTANT]
> All code you commit must be clean and have comments.
> For development, edit sources under `src/`, then re-run `make run`.
