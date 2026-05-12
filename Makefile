BUILD = build

CC = x86_64-elf-gcc
LD = x86_64-elf-ld
AS = nasm

CFLAGS = -m32 -ffreestanding -O0 -c
LDFLAGS = -m elf_i386 -T linker.ld --oformat binary

all: os.bin

$(BUILD):
	mkdir -p $(BUILD)

boot.bin: boot.asm | $(BUILD)
	$(AS) -f bin boot.asm -o $(BUILD)/boot.bin

entry.o: entry.asm | $(BUILD)
	$(AS) -f elf32 entry.asm -o $(BUILD)/entry.o

kernel.o: kernel.c | $(BUILD)
	$(CC) $(CFLAGS) kernel.c -o $(BUILD)/kernel.o

kernel.bin: entry.o kernel.o linker.ld
	$(LD) $(LDFLAGS) $(BUILD)/entry.o $(BUILD)/kernel.o -o $(BUILD)/kernel.bin

os.bin: boot.bin kernel.bin
	cat $(BUILD)/boot.bin $(BUILD)/kernel.bin > $(BUILD)/os.bin

os.img: boot.bin kernel.bin
	dd if=/dev/zero of=$(BUILD)/os.img bs=512 count=2880
	dd if=$(BUILD)/boot.bin of=$(BUILD)/os.img conv=notrunc
	dd if=$(BUILD)/kernel.bin of=$(BUILD)/os.img bs=512 seek=1 conv=notrunc

run: os.img
	qemu-system-x86_64 -drive format=raw,file=$(BUILD)/os.img -display sdl

clean:
	rm -rf $(BUILD)