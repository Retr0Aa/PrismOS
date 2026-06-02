CC=gcc
LD=gcc

CPPFLAGS=-Isrc
CFLAGS=-m32 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector -fno-pic -fno-pie
LDFLAGS=-m32 -T boot/linker.ld -ffreestanding -nostdlib -no-pie -Wl,--build-id=none

BUILD=build
ISO_DIR=$(BUILD)/isodir

.PHONY: all run run-serial run-serial-log prismcc clean

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

command.o: src/commands/command.c src/commands/command.h src/debug/log.h src/comport/comport.h src/display/console.h src/platform/io.h src/platform/system.h src/filesystem/vfs.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/commands/command.c -o $(BUILD)/command.o

blockdev.o: src/filesystem/blockdev.c src/filesystem/blockdev.h src/debug/log.h src/platform/io.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/filesystem/blockdev.c -o $(BUILD)/blockdev.o

fat32.o: src/filesystem/fat32/fat32.c src/filesystem/fat32/fat32.h src/filesystem/blockdev.h src/debug/log.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/filesystem/fat32/fat32.c -o $(BUILD)/fat32.o

vfs.o: src/filesystem/vfs.c src/filesystem/vfs.h src/filesystem/fat32/fat32.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/filesystem/vfs.c -o $(BUILD)/vfs.o

app_loader.o: src/apps/app_loader.c src/apps/app_loader.h src/apps/app_format.h src/filesystem/vfs.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/apps/app_loader.c -o $(BUILD)/app_loader.o

app_runtime.o: src/apps/app_runtime.c src/apps/app_runtime.h src/apps/editor_app.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/apps/app_runtime.c -o $(BUILD)/app_runtime.o

bytecode_vm.o: src/apps/bytecode_vm.c src/apps/bytecode_vm.h src/apps/app_format.h src/debug/log.h src/display/console.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/apps/bytecode_vm.c -o $(BUILD)/bytecode_vm.o

prismcc_runtime.o: src/apps/prismcc_runtime.c src/apps/prismcc_runtime.h src/apps/app_format.h src/debug/log.h src/filesystem/vfs.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/apps/prismcc_runtime.c -o $(BUILD)/prismcc_runtime.o

editor_app.o: src/apps/editor_app.c src/apps/editor_app.h src/display/console.h src/filesystem/vfs.h src/input/keyboard.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/apps/editor_app.c -o $(BUILD)/editor_app.o

app_manager.o: src/apps/app_manager.c src/apps/app_manager.h src/apps/app_loader.h src/apps/app_runtime.h src/apps/app_format.h src/debug/log.h src/filesystem/vfs.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/apps/app_manager.c -o $(BUILD)/app_manager.o

gdt.o: src/platform/gdt.c src/platform/gdt.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/platform/gdt.c -o $(BUILD)/gdt.o

pic.o: src/platform/pic.c src/platform/pic.h src/platform/io.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/platform/pic.c -o $(BUILD)/pic.o

isr_stubs.o: src/interrupts/isr_stubs.s | $(BUILD)
	gcc -m32 -c src/interrupts/isr_stubs.s -o $(BUILD)/isr_stubs.o

idt.o: src/interrupts/idt.c src/interrupts/idt.h src/interrupts/irq.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/interrupts/idt.c -o $(BUILD)/idt.o

irq.o: src/interrupts/irq.c src/interrupts/irq.h src/platform/pic.h src/debug/log.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/interrupts/irq.c -o $(BUILD)/irq.o

interrupts.o: src/interrupts/interrupts.c src/interrupts/interrupts.h src/interrupts/idt.h src/interrupts/irq.h src/platform/gdt.h src/platform/pic.h src/debug/log.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/interrupts/interrupts.c -o $(BUILD)/interrupts.o

ringbuf.o: src/util/ringbuf.c src/util/ringbuf.h | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/util/ringbuf.c -o $(BUILD)/ringbuf.o

pmm.o: src/memory/pmm.c src/memory/pmm.h src/platform/system.h src/display/console.h src/debug/log.h boot/linker.ld | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/memory/pmm.c -o $(BUILD)/pmm.o

paging.o: src/memory/paging.c src/memory/paging.h src/memory/pmm.h src/platform/io.h src/interrupts/irq.h src/debug/log.h src/display/console.h boot/linker.ld | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c src/memory/paging.c -o $(BUILD)/paging.o

INTERRUPT_OBJS = $(BUILD)/gdt.o $(BUILD)/pic.o $(BUILD)/isr_stubs.o $(BUILD)/idt.o $(BUILD)/irq.o $(BUILD)/interrupts.o $(BUILD)/ringbuf.o
MEMORY_OBJS = $(BUILD)/pmm.o $(BUILD)/paging.o
FS_OBJS = $(BUILD)/blockdev.o $(BUILD)/fat32.o $(BUILD)/vfs.o
APP_OBJS = $(BUILD)/app_loader.o $(BUILD)/app_runtime.o $(BUILD)/bytecode_vm.o $(BUILD)/prismcc_runtime.o $(BUILD)/editor_app.o $(BUILD)/app_manager.o

kernel.elf: boot.o kernel.o console.o psf_font.o keyboard.o serial.o comport.o log.o shell.o command.o blockdev.o fat32.o vfs.o app_loader.o app_runtime.o bytecode_vm.o prismcc_runtime.o editor_app.o app_manager.o font_psf.o gdt.o pic.o isr_stubs.o idt.o irq.o interrupts.o ringbuf.o pmm.o paging.o boot/linker.ld
	gcc $(LDFLAGS) $(BUILD)/boot.o $(BUILD)/kernel.o $(BUILD)/console.o $(BUILD)/psf_font.o $(BUILD)/keyboard.o $(BUILD)/serial.o $(BUILD)/comport.o $(BUILD)/log.o $(BUILD)/shell.o $(BUILD)/command.o $(FS_OBJS) $(APP_OBJS) $(BUILD)/font_psf.o $(INTERRUPT_OBJS) $(MEMORY_OBJS) -o $(BUILD)/kernel.elf

prismcc: tools/prismcc.c src/apps/app_format.h | $(BUILD)
	gcc -O2 -Wall -Wextra -Isrc tools/prismcc.c -o $(BUILD)/prismcc

os.iso: kernel.elf boot/grub.cfg
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(BUILD)/kernel.elf $(ISO_DIR)/boot/kernel.elf
	cp boot/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD)/os.iso $(ISO_DIR)

$(BUILD)/disk.img:
	truncate -s 64M $(BUILD)/disk.img

disk.img: $(BUILD)/disk.img

run: os.iso $(BUILD)/disk.img
	qemu-system-x86_64 -boot order=d -cdrom $(BUILD)/os.iso -drive file=$(BUILD)/disk.img,format=raw,if=ide,index=0,media=disk -monitor none

run-serial: os.iso $(BUILD)/disk.img
	qemu-system-x86_64 -boot order=d -cdrom $(BUILD)/os.iso -drive file=$(BUILD)/disk.img,format=raw,if=ide,index=0,media=disk -serial stdio -monitor none

run-serial-log: os.iso $(BUILD)/disk.img
	qemu-system-x86_64 -boot order=d -cdrom $(BUILD)/os.iso -drive file=$(BUILD)/disk.img,format=raw,if=ide,index=0,media=disk -serial file:$(BUILD)/serial.log -monitor none

clean:
	rm -rf $(BUILD)