#ifndef PRISMOS_FILESYSTEM_BLOCKDEV_H
#define PRISMOS_FILESYSTEM_BLOCKDEV_H

#include <stdint.h>

#define BLOCKDEV_SECTOR_SIZE 512U

/* The filesystem layer talks to a single persistent IDE disk image.
 * The sector interface stays small so FAT32 can stay focused on on-disk logic. */
int blockdev_init_disk(void);
int blockdev_read_sector(uint32_t sector, void* buffer);
int blockdev_write_sector(uint32_t sector, const void* buffer);
uint32_t blockdev_sector_count(void);

#endif
