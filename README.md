# PrismOS
PrismOS is hobby operating system.

## Build And Run

The project supports two boot paths:

- `make run` builds the raw floppy-style image at `build/os.img` and boots it in QEMU.
- `make run-iso` builds a BIOS-bootable El Torito ISO at `build/os.iso` and boots the ISO in QEMU.

The ISO path requires `xorriso`, which is commonly available on Linux distributions.

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
