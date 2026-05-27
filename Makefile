CC=gcc
LD=gcc

CPPFLAGS=-Isrc
CFLAGS=-m32 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector -fno-pic -fno-pie
LDFLAGS=-m32 -T boot/linker.ld -ffreestanding -nostdlib -no-pie -Wl,--build-id=none

BUILD=build
ISO_DIR=$(BUILD)/isodir

.PHONY: all run run-serial run-serial-log clean

all: os.iso

$(BUILD):
	mkdir -p $(BUILD)

boot.o: boot/boot.s | $(BUILD)
	gcc -m32 -c boot/boot.s -o $(BUILD)/boot.o

kernel.o: src/kernel.c src/shell/shell.h src/display/console.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/kernel.c -o $(BUILD)/kernel.o

log.o: src/debug/log.c src/debug/log.h src/comport/comport.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/debug/log.c -o $(BUILD)/log.o

console.o: src/display/console.c src/display/console.h src/display/psf_font.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/display/console.c -o $(BUILD)/console.o

psf_font.o: src/display/psf_font.c src/display/psf_font.h src/display/font8x8.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/display/psf_font.c -o $(BUILD)/psf_font.o

font_psf.o: assets/fonts/cp850-8x16.psf | $(BUILD)
	objcopy -I binary -O elf32-i386 -B i386 assets/fonts/cp850-8x16.psf $(BUILD)/font_psf.o

keyboard.o: src/input/keyboard.c src/input/keyboard.h src/platform/io.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/input/keyboard.c -o $(BUILD)/keyboard.o

serial.o: src/display/serial.c src/display/serial.h src/platform/io.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/display/serial.c -o $(BUILD)/serial.o

comport.o: src/comport/comport.c src/comport/comport.h src/platform/io.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/comport/comport.c -o $(BUILD)/comport.o

shell.o: src/shell/shell.c src/shell/shell.h src/debug/log.h src/commands/command.h src/display/console.h src/input/keyboard.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/shell/shell.c -o $(BUILD)/shell.o

command.o: src/commands/command.c src/commands/command.h src/debug/log.h src/comport/comport.h src/display/console.h src/platform/io.h src/platform/system.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/commands/command.c -o $(BUILD)/command.o

kernel.elf: boot.o kernel.o console.o psf_font.o keyboard.o serial.o comport.o log.o shell.o command.o font_psf.o boot/linker.ld
	gcc $(LDFLAGS) $(BUILD)/boot.o $(BUILD)/kernel.o $(BUILD)/console.o $(BUILD)/psf_font.o $(BUILD)/keyboard.o $(BUILD)/serial.o $(BUILD)/comport.o $(BUILD)/log.o $(BUILD)/shell.o $(BUILD)/command.o $(BUILD)/font_psf.o -o $(BUILD)/kernel.elf

os.iso: kernel.elf boot/grub.cfg
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(BUILD)/kernel.elf $(ISO_DIR)/boot/kernel.elf
	cp boot/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD)/os.iso $(ISO_DIR)

run: os.iso
	qemu-system-x86_64 -cdrom $(BUILD)/os.iso

run-serial: os.iso
	qemu-system-x86_64 -cdrom $(BUILD)/os.iso -serial stdio -monitor none

run-serial-log: os.iso
	qemu-system-x86_64 -cdrom $(BUILD)/os.iso -serial file:$(BUILD)/serial.log

clean:
	rm -rf $(BUILD)