# PrismOS
PrismOS is hobby operating system.

## Build And Run

PrismOS uses GRUB with Multiboot 1 protocol for modern, standards-compliant boot:

- `make os.iso` builds a BIOS-bootable ISO at `build/os.iso` with GRUB bootloader
- `make run` boots the ISO in QEMU with graphical display
- `make clean` removes all build artifacts

The build system requires:
- `nasm` - assembler for boot code
- `gcc` (or `x86_64-elf-gcc` for cross-compilation) - C compiler
- `grub-mkrescue` - creates bootable ISO with GRUB bootloader
- `qemu-system-i386` - for testing in QEMU

### Boot in QEMU

```bash
make run          # Build and boot in QEMU with display
```

### Boot in VMware

Mount `build/os.iso` in a BIOS-mode VM and boot. GRUB menu will appear with PrismOS entry.

## Adding Commands

PrismOS uses a command registry in [src/command.c](src/command.c) as the single source of truth for built-in shell commands. Each command is represented by one entry in the `commands` array with three pieces of data:

- `name`: the command typed in the shell
- `description`: the text shown by `help`
- `handler`: the function that runs when the command is executed

To add a new command:

1. Add a new handler function in [src/command.c](src/command.c).
2. Add one new entry to the `commands` array with the command name, description, and handler.
3. Rebuild PrismOS.

The `help` command automatically lists every command from the registry, so no separate help text needs to be updated.
