BUILD = build
ISO_ROOT = $(BUILD)/iso-root
ISO_IMG = $(ISO_ROOT)/boot.img

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
LDFLAGS = -m elf_i386 -T boot/linker.ld --oformat binary

ifeq ($(HOST_OS),Linux)
	CFLAGS += -fno-pic -fno-pie -fno-stack-protector
endif

all: os.bin

$(BUILD):
	mkdir -p $(BUILD)

boot.bin: boot/boot.asm | $(BUILD)
	$(AS) -f bin boot/boot.asm -o $(BUILD)/boot.bin

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

kernel.bin: entry.o kernel.o console.o keyboard.o shell.o command.o boot/linker.ld
	$(LD) $(LDFLAGS) $(BUILD)/entry.o $(BUILD)/kernel.o $(BUILD)/console.o $(BUILD)/keyboard.o $(BUILD)/shell.o $(BUILD)/command.o -o $(BUILD)/kernel.bin

os.bin: boot.bin kernel.bin
	cat $(BUILD)/boot.bin $(BUILD)/kernel.bin > $(BUILD)/os.bin

$(ISO_ROOT): | $(BUILD)
	mkdir -p $@

$(ISO_IMG): os.img | $(ISO_ROOT)
	cp $(BUILD)/os.img $@

os.iso: $(ISO_IMG)
	xorriso -as mkisofs -o $(BUILD)/os.iso -b boot.img -c boot.catalog $(ISO_ROOT)

os.img: boot.bin kernel.bin
	dd if=/dev/zero of=$(BUILD)/os.img bs=512 count=2880
	dd if=$(BUILD)/boot.bin of=$(BUILD)/os.img conv=notrunc
	dd if=$(BUILD)/kernel.bin of=$(BUILD)/os.img bs=512 seek=1 conv=notrunc

run: os.img
	qemu-system-x86_64 $(QEMU_ACCEL) -drive format=raw,file=$(BUILD)/os.img -display sdl

run-iso: os.iso
	qemu-system-x86_64 $(QEMU_ACCEL) -boot order=d -cdrom $(BUILD)/os.iso -display sdl

clean:
	rm -rf $(BUILD)