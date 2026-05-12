BUILD = build

AS = nasm
CPPFLAGS = -Isrc

HOST_ARCH := $(shell uname -m)
HOST_OS := $(shell uname -s)

# Toolchain and QEMU accel selection by host OS/arch
ifeq ($(HOST_OS),Darwin)
	CC = x86_64-elf-gcc
	LD = x86_64-elf-ld
	QEMU_ACCEL = -accel tcg,thread=multi
else ifeq ($(HOST_OS),Linux)
	ifeq ($(HOST_ARCH),x86_64)
		CC = gcc
		LD = ld
		QEMU_ACCEL = -accel kvm
	else
		CC = x86_64-elf-gcc
		LD = x86_64-elf-ld
		QEMU_ACCEL = -accel tcg,thread=multi
	endif
else
	CC = x86_64-elf-gcc
	LD = x86_64-elf-ld
	QEMU_ACCEL = -accel tcg,thread=multi
endif

CFLAGS = -m32 -ffreestanding -O0 -c
LDFLAGS = -m elf_i386 -T boot/linker.ld

ifeq ($(HOST_OS),Linux)
	CFLAGS += -fno-pic -fno-pie -fno-stack-protector
endif

all: os.iso

$(BUILD):
	mkdir -p $(BUILD)



entry.o: boot/entry.asm | $(BUILD)
	$(AS) -f elf32 boot/entry.asm -o $(BUILD)/entry.o

kernel.o: src/kernel.c src/shell/shell.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/kernel.c -o $(BUILD)/kernel.o

console.o: src/display/console.c src/display/console.h src/platform/io.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/display/console.c -o $(BUILD)/console.o

keyboard.o: src/input/keyboard.c src/input/keyboard.h src/platform/io.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/input/keyboard.c -o $(BUILD)/keyboard.o

shell.o: src/shell/shell.c src/shell/shell.h src/commands/command.h src/display/console.h src/input/keyboard.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/shell/shell.c -o $(BUILD)/shell.o

command.o: src/commands/command.c src/commands/command.h src/display/console.h src/platform/io.h src/platform/system.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) src/commands/command.c -o $(BUILD)/command.o

# Link as ELF kernel with Multiboot header
kernel.elf: entry.o kernel.o console.o keyboard.o shell.o command.o boot/linker.ld
	$(LD) $(LDFLAGS) $(BUILD)/entry.o $(BUILD)/kernel.o $(BUILD)/console.o $(BUILD)/keyboard.o $(BUILD)/shell.o $(BUILD)/command.o -o $(BUILD)/kernel.elf

# Create ISO with GRUB bootloader
os.iso: kernel.elf boot/grub.cfg
	@mkdir -p $(BUILD)/iso-root/boot/grub
	cp $(BUILD)/kernel.elf $(BUILD)/iso-root/boot/
	cp boot/grub.cfg $(BUILD)/iso-root/boot/grub/
	grub-mkrescue -o $(BUILD)/os.iso $(BUILD)/iso-root

run: os.iso
	qemu-system-i386 $(QEMU_ACCEL) -drive file=$(BUILD)/os.iso,index=0,media=cdrom -m 256 -display sdl

clean:
	rm -rf $(BUILD)