CC=gcc
LD=gcc

CPPFLAGS=-Isrc
CFLAGS=-m32 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector -fno-pic -fno-pie
LDFLAGS=-m32 -T boot/linker.ld -ffreestanding -nostdlib -no-pie -Wl,--build-id=none

BUILD=build
ISO_DIR=$(BUILD)/isodir

QEMU=qemu-system-x86_64
QEMU_FLAGS=-boot order=d -cdrom $(BUILD)/os.iso \
	-drive file=$(BUILD)/disk.img,format=raw,if=ide,index=0,media=disk \
	-monitor none

.PHONY: all run run-serial run-serial-log prismcc clean

all: os.iso

$(BUILD):
	mkdir -p $(BUILD)

# -------------------------
# Core objects
# -------------------------
CORE_OBJS = \
$(BUILD)/boot.o \
$(BUILD)/kernel.o \
$(BUILD)/console.o \
$(BUILD)/psf_font.o \
$(BUILD)/keyboard.o \
$(BUILD)/serial.o \
$(BUILD)/comport.o \
$(BUILD)/log.o \
$(BUILD)/shell.o \
$(BUILD)/command.o \
$(BUILD)/font_psf.o \
$(BUILD)/string.o

# -------------------------
# Systems
# -------------------------
INTERRUPT_OBJS = \
$(BUILD)/gdt.o \
$(BUILD)/pic.o \
$(BUILD)/isr_stubs.o \
$(BUILD)/idt.o \
$(BUILD)/irq.o \
$(BUILD)/interrupts.o \
$(BUILD)/ringbuf.o

MEMORY_OBJS = \
$(BUILD)/pmm.o \
$(BUILD)/paging.o

FS_OBJS = \
$(BUILD)/blockdev.o \
$(BUILD)/fat32.o \
$(BUILD)/vfs.o

APP_OBJS = \
$(BUILD)/app_loader.o \
$(BUILD)/app_runtime.o \
$(BUILD)/bytecode_vm.o \
$(BUILD)/prismcc_runtime.o \
$(BUILD)/editor_app.o \
$(BUILD)/ide_app.o \
$(BUILD)/app_manager.o

# -------------------------
# Compile rules
# -------------------------
$(BUILD)/%.o: boot/%.s | $(BUILD)
	gcc -m32 -c $< -o $@

$(BUILD)/%.o: src/kernel.c | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/debug/%.c | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/display/%.c | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/input/%.c | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/comport/%.c | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/shell/%.c | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/commands/%.c | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/filesystem/%.c | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/filesystem/fat32/%.c | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/apps/%.c | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/platform/%.c | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/interrupts/%.c | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/util/%.c | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/string.o: src/util/string.c | $(BUILD)
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/font_psf.o: assets/fonts/cp850-8x16.psf | $(BUILD)
	objcopy -I binary -O elf32-i386 -B i386 $< $@

# -------------------------
# Link kernel
# -------------------------
KERNEL_OBJS = \
$(CORE_OBJS) \
$(INTERRUPT_OBJS) \
$(MEMORY_OBJS) \
$(FS_OBJS) \
$(APP_OBJS)

kernel.elf: $(KERNEL_OBJS) boot/linker.ld
	gcc $(LDFLAGS) $(KERNEL_OBJS) -o $(BUILD)/kernel.elf

# -------------------------
# ISO
# -------------------------
os.iso: kernel.elf boot/grub.cfg
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(BUILD)/kernel.elf $(ISO_DIR)/boot/kernel.elf
	cp boot/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD)/os.iso $(ISO_DIR)

# -------------------------
# Disk
# -------------------------
$(BUILD)/disk.img:
	truncate -s 64M $@

# -------------------------
# RUN (NO KVM)
# -------------------------
run: os.iso $(BUILD)/disk.img
	$(QEMU) $(QEMU_FLAGS)

run-serial: os.iso $(BUILD)/disk.img
	$(QEMU) $(QEMU_FLAGS) -serial stdio

run-serial-log: os.iso $(BUILD)/disk.img
	$(QEMU) $(QEMU_FLAGS) -serial file:$(BUILD)/serial.log

# -------------------------
# RUN (KVM - FAST)
# -------------------------
run-kvm: os.iso $(BUILD)/disk.img
	$(QEMU) $(QEMU_FLAGS) -enable-kvm -cpu host -m 512M

run-kvm-serial: os.iso $(BUILD)/disk.img
	$(QEMU) $(QEMU_FLAGS) -enable-kvm -cpu host -m 512M -serial stdio

# -------------------------
# Tools
# -------------------------
prismcc: tools/prismcc.c src/apps/app_format.h | $(BUILD)
	gcc -O2 -Wall -Wextra -Isrc $< -o $(BUILD)/prismcc

clean:
	rm -rf $(BUILD)