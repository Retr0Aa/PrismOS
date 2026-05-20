UEFI Boot Guide
================

Goal: make the kernel boot reliably on UEFI laptops (GOP framebuffer + serial fallback).

Quick points:
- Most modern laptops use 64-bit UEFI only. This repository currently builds a 32-bit (i386) kernel (`-m32`). To boot natively via UEFI you should build a 64-bit kernel (`-m64`).
- GRUB in EFI mode can still load multiboot2-compliant kernels, but the kernel must match the firmware architecture.

Steps to get UEFI boot working
1. Build a 64-bit kernel
   - Change `CFLAGS`/`LDFLAGS` in `Makefile` from `-m32` to `-m64` and update `boot/boot.s` to include a multiboot2 header compatible with 64-bit if needed.
   - Rebuild the kernel artifacts.

2. Create an EFI ISO using GRUB
   - Ensure you have grub-mkrescue with `x86_64-efi` support installed (grub-efi-amd64-bin on Debian/Ubuntu).
   - Then create the ISO with:

```sh
mkdir -p build/isodir/boot/grub
cp build/kernel.elf build/isodir/boot/kernel.elf
cp boot/grub.cfg build/isodir/boot/grub/grub.cfg
grub-mkrescue --format=x86_64-efi -o build/os.iso build/isodir
```

3. Use GRUB EFI payload options
   - Avoid forcing a particular graphics mode in `boot/grub.cfg` (remove `set gfxmode` and `gfxpayload`) so GRUB will leave the firmware/GOP state intact for the kernel.

4. Use GOP framebuffer or serial for console
   - The kernel should accept generic framebuffer information from the bootloader (GOP via GRUB). We updated the kernel to accept multiboot framebuffer tags even when not RGB.
   - Keep the serial fallback enabled (COM1/0x3F8) so you get output even when framebuffer setup fails.

Notes and troubleshooting
- If your laptop firmware is 64-bit-only and you try to boot a 32-bit kernel, it will not start. Prefer building a 64-bit kernel.
- Enabling serial in firmware or using a USB-to-serial adapter can help diagnose boot-time issues.
- For full UEFI-native kernels you can also write a simple EFI boot stub that invokes your kernel directly; using GRUB (EFI) is faster to iterate.

If you want, I can:
- Update the Makefile and boot.s to build a 64-bit kernel and verify a 64-bit multiboot2 flow.
- Add a simple EFI stub instead of GRUB.
