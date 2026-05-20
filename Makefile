CC=gcc
LD=gcc

CPPFLAGS=-Isrc
CFLAGS=-m32 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector -fno-pic -fno-pie
LDFLAGS=-m32 -T boot/linker.ld -ffreestanding -nostdlib -no-pie -Wl,--build-id=none

BUILD=build
ISO_DIR=$(BUILD)/isodir

all: os.iso

$(BUILD):
	mkdir -p $(BUILD)

boot.o: boot/boot.s | $(BUILD)
	gcc -m32 -c boot/boot.s -o $(BUILD)/boot.o

kernel.o: src/kernel.c src/shell/shell.h src/display/console.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/kernel.c -o $(BUILD)/kernel.o

console.o: src/display/console.c src/display/console.h src/display/font8x8.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/display/console.c -o $(BUILD)/console.o

keyboard.o: src/input/keyboard.c src/input/keyboard.h src/platform/io.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/input/keyboard.c -o $(BUILD)/keyboard.o

serial.o: src/display/serial.c src/display/serial.h src/platform/io.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/display/serial.c -o $(BUILD)/serial.o

shell.o: src/shell/shell.c src/shell/shell.h src/commands/command.h src/display/console.h src/input/keyboard.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/shell/shell.c -o $(BUILD)/shell.o

command.o: src/commands/command.c src/commands/command.h src/display/console.h src/platform/io.h src/platform/system.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/commands/command.c -o $(BUILD)/command.o

kernel.elf: boot.o kernel.o console.o keyboard.o serial.o shell.o command.o boot/linker.ld
	gcc $(LDFLAGS) $(BUILD)/boot.o $(BUILD)/kernel.o $(BUILD)/console.o $(BUILD)/keyboard.o $(BUILD)/serial.o $(BUILD)/shell.o $(BUILD)/command.o -o $(BUILD)/kernel.elf

os.iso: kernel.elf boot/grub.cfg
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(BUILD)/kernel.elf $(ISO_DIR)/boot/kernel.elf
	cp boot/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD)/os.iso $(ISO_DIR)

run: os.iso
	qemu-system-x86_64 -cdrom $(BUILD)/os.iso

clean:
	rm -rf $(BUILD)