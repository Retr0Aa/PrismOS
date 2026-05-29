#include "filesystem/blockdev.h"

#include <stddef.h>

#include "debug/log.h"
#include "platform/io.h"

#define ATA_PRIMARY_IO_BASE 0x1F0U
#define ATA_PRIMARY_CTRL    0x3F6U

#define ATA_REG_DATA     (ATA_PRIMARY_IO_BASE + 0U)
#define ATA_REG_ERROR    (ATA_PRIMARY_IO_BASE + 1U)
#define ATA_REG_SECCOUNT (ATA_PRIMARY_IO_BASE + 2U)
#define ATA_REG_LBA0     (ATA_PRIMARY_IO_BASE + 3U)
#define ATA_REG_LBA1     (ATA_PRIMARY_IO_BASE + 4U)
#define ATA_REG_LBA2     (ATA_PRIMARY_IO_BASE + 5U)
#define ATA_REG_HDDEVSEL (ATA_PRIMARY_IO_BASE + 6U)
#define ATA_REG_COMMAND  (ATA_PRIMARY_IO_BASE + 7U)
#define ATA_REG_STATUS   (ATA_PRIMARY_IO_BASE + 7U)
#define ATA_REG_ALTSTATUS ATA_PRIMARY_CTRL

#define ATA_CMD_READ_SECTORS  0x20U
#define ATA_CMD_WRITE_SECTORS 0x30U
#define ATA_CMD_IDENTIFY      0xECU

#define ATA_STATUS_ERR  0x01U
#define ATA_STATUS_DRQ  0x08U
#define ATA_STATUS_DF   0x20U
#define ATA_STATUS_BSY  0x80U

#define IDE_DEFAULT_SECTOR_COUNT 131072U
#define FAT32_RESERVED_SECTORS 32U
#define FAT32_FAT_SECTOR 32U
#define FAT32_DATA_START_SECTOR 33U
#define FAT32_ROOT_CLUSTER 2U
#define FAT32_FILE_CLUSTER 3U

static int disk_ready = 0;
static uint32_t disk_sector_count = IDE_DEFAULT_SECTOR_COUNT;

static void memory_set(uint8_t* destination, uint8_t value, uint32_t count) {
    for (uint32_t index = 0; index < count; index++) {
        destination[index] = value;
    }
}

static void memory_copy(uint8_t* destination, const uint8_t* source, uint32_t count) {
    for (uint32_t index = 0; index < count; index++) {
        destination[index] = source[index];
    }
}

static void write_u16le(uint8_t* out, uint16_t value) {
    out[0] = (uint8_t)(value & 0xFFU);
    out[1] = (uint8_t)((value >> 8) & 0xFFU);
}

static void write_u32le(uint8_t* out, uint32_t value) {
    out[0] = (uint8_t)(value & 0xFFU);
    out[1] = (uint8_t)((value >> 8) & 0xFFU);
    out[2] = (uint8_t)((value >> 16) & 0xFFU);
    out[3] = (uint8_t)((value >> 24) & 0xFFU);
}

static void write_string(uint8_t* destination, const char* source, uint32_t count) {
    uint32_t index = 0;

    while (index < count && source[index] != '\0') {
        destination[index] = (uint8_t)source[index];
        index++;
    }

    while (index < count) {
        destination[index] = ' ';
        index++;
    }
}

static uint8_t ata_status(void) {
    return inb(ATA_REG_STATUS);
}

static uint8_t ata_alt_status(void) {
    return inb(ATA_REG_ALTSTATUS);
}

static int ata_wait_ready(void) {
    uint32_t guard = 0;

    while (guard < 1000000U) {
        uint8_t status = ata_alt_status();
        if ((status & ATA_STATUS_BSY) == 0U) {
            return 0;
        }

        guard++;
    }

    return -1;
}

static int ata_wait_drq(void) {
    uint32_t guard = 0;

    while (guard < 1000000U) {
        uint8_t status = ata_status();
        if ((status & ATA_STATUS_BSY) != 0U) {
            guard++;
            continue;
        }

        if ((status & (ATA_STATUS_ERR | ATA_STATUS_DF)) != 0U) {
            return -1;
        }

        if ((status & ATA_STATUS_DRQ) != 0U) {
            return 0;
        }

        guard++;
    }

    return -1;
}

static int ata_select_lba(uint32_t lba) {
    if (ata_wait_ready() != 0) {
        return -1;
    }

    outb(ATA_REG_HDDEVSEL, (uint8_t)(0xE0U | ((lba >> 24) & 0x0FU)));
    outb(ATA_REG_SECCOUNT, 1U);
    outb(ATA_REG_LBA0, (uint8_t)(lba & 0xFFU));
    outb(ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFFU));
    outb(ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFFU));
    return 0;
}

static int ata_identify(void) {
    uint16_t words[256];

    if (ata_wait_ready() != 0) {
        return -1;
    }

    outb(ATA_REG_HDDEVSEL, 0xE0U);
    outb(ATA_REG_SECCOUNT, 0U);
    outb(ATA_REG_LBA0, 0U);
    outb(ATA_REG_LBA1, 0U);
    outb(ATA_REG_LBA2, 0U);
    outb(ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    if (ata_status() == 0U) {
        return -1;
    }

    if (ata_wait_drq() != 0) {
        return -1;
    }

    for (uint32_t index = 0; index < 256U; index++) {
        words[index] = inw(ATA_REG_DATA);
    }

    if ((words[83] & (1U << 10)) != 0U || (words[48] & (1U << 9)) != 0U) {
        uint32_t lba48 = ((uint32_t)words[100]) | ((uint32_t)words[101] << 16);
        if (lba48 != 0U) {
            disk_sector_count = lba48;
        }
    }

    return 0;
}

static int ata_read_sector_raw(uint32_t sector, void* buffer) {
    uint16_t* destination = (uint16_t*)buffer;

    if (sector >= disk_sector_count) {
        return -1;
    }

    if (ata_select_lba(sector) != 0) {
        return -1;
    }

    outb(ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);

    if (ata_wait_drq() != 0) {
        return -1;
    }

    for (uint32_t index = 0; index < 256U; index++) {
        destination[index] = inw(ATA_REG_DATA);
    }

    return 0;
}

static int ata_write_sector_raw(uint32_t sector, const void* buffer) {
    const uint16_t* source = (const uint16_t*)buffer;

    if (sector >= disk_sector_count) {
        return -1;
    }

    if (ata_select_lba(sector) != 0) {
        return -1;
    }

    outb(ATA_REG_COMMAND, ATA_CMD_WRITE_SECTORS);

    if (ata_wait_drq() != 0) {
        return -1;
    }

    for (uint32_t index = 0; index < 256U; index++) {
        outw(ATA_REG_DATA, source[index]);
    }

    outb(ATA_REG_COMMAND, 0xE7U);
    if (ata_wait_ready() != 0) {
        return -1;
    }

    return 0;
}

static void format_boot_sector(uint8_t* sector) {
    memory_set(sector, 0, BLOCKDEV_SECTOR_SIZE);

    sector[0] = 0xEB;
    sector[1] = 0x58;
    sector[2] = 0x90;
    write_string(&sector[3], "PRISMOS", 8);
    write_u16le(&sector[11], (uint16_t)BLOCKDEV_SECTOR_SIZE);
    sector[13] = 1;
    write_u16le(&sector[14], FAT32_RESERVED_SECTORS);
    sector[16] = 1;
    write_u16le(&sector[17], 0);
    write_u16le(&sector[19], 0);
    sector[21] = 0xF8;
    write_u16le(&sector[22], 0);
    write_u16le(&sector[24], 63);
    write_u16le(&sector[26], 255);
    write_u32le(&sector[28], 0);
    write_u32le(&sector[32], disk_sector_count);
    write_u32le(&sector[36], 1);
    write_u16le(&sector[40], 0);
    write_u16le(&sector[42], 0);
    write_u32le(&sector[44], FAT32_ROOT_CLUSTER);
    write_u16le(&sector[48], 1);
    write_u16le(&sector[50], 6);
    sector[64] = 0x80;
    sector[66] = 0x29;
    write_u32le(&sector[67], 0x26052026U);
    write_string(&sector[71], "PRISMOSVOL", 11);
    write_string(&sector[82], "FAT32", 8);
    sector[510] = 0x55;
    sector[511] = 0xAA;
}

static void format_fsinfo_sector(uint8_t* sector) {
    memory_set(sector, 0, BLOCKDEV_SECTOR_SIZE);

    write_u32le(&sector[0], 0x41615252U);
    write_u32le(&sector[484], 0x61417272U);
    write_u32le(&sector[488], 0xFFFFFFFFU);
    write_u32le(&sector[492], 0xFFFFFFFFU);
    sector[510] = 0x55;
    sector[511] = 0xAA;
}

static void format_fat_sector(uint8_t* sector) {
    memory_set(sector, 0, BLOCKDEV_SECTOR_SIZE);

    write_u32le(&sector[0], 0x0FFFFFF8U);
    write_u32le(&sector[4], 0xFFFFFFFFU);
    write_u32le(&sector[FAT32_ROOT_CLUSTER * 4U], 0x0FFFFFFFU);
    write_u32le(&sector[FAT32_FILE_CLUSTER * 4U], 0x0FFFFFFFU);
}

static void format_root_directory(uint8_t* sector) {
    uint8_t* entry = &sector[0];
    const char* payload = "Hello from FAT32 on PrismOS\n";
    uint32_t payload_size = 0;

    memory_set(sector, 0, BLOCKDEV_SECTOR_SIZE);

    write_string(&entry[0], "HELLO", 8);
    write_string(&entry[8], "TXT", 3);
    entry[11] = 0x20;
    write_u16le(&entry[20], 0);
    write_u16le(&entry[26], FAT32_FILE_CLUSTER);

    while (payload[payload_size] != '\0') {
        payload_size++;
    }

    write_u32le(&entry[28], payload_size);
}

static int disk_is_blank(void) {
    uint8_t sector[BLOCKDEV_SECTOR_SIZE];

    if (ata_read_sector_raw(0, sector) != 0) {
        return 1;
    }

    return sector[510] != 0x55 || sector[511] != 0xAA;
}

static int initialize_blank_fat32_image(void) {
    uint8_t sector[BLOCKDEV_SECTOR_SIZE];
    uint8_t fat_sector[BLOCKDEV_SECTOR_SIZE];
    uint8_t root_sector[BLOCKDEV_SECTOR_SIZE];
    uint8_t file_cluster[BLOCKDEV_SECTOR_SIZE];

    format_boot_sector(sector);
    if (ata_write_sector_raw(0, sector) != 0) {
        return -1;
    }

    format_fsinfo_sector(sector);
    if (ata_write_sector_raw(1, sector) != 0) {
        return -1;
    }

    format_fat_sector(fat_sector);
    if (ata_write_sector_raw(FAT32_FAT_SECTOR, fat_sector) != 0) {
        return -1;
    }

    memory_set(root_sector, 0, BLOCKDEV_SECTOR_SIZE);
    format_root_directory(root_sector);
    if (ata_write_sector_raw(FAT32_DATA_START_SECTOR, root_sector) != 0) {
        return -1;
    }

    memory_set(file_cluster, 0, BLOCKDEV_SECTOR_SIZE);
    {
        const char* payload = "Hello from FAT32 on PrismOS\n";
        uint32_t payload_size = 0;

        while (payload[payload_size] != '\0') {
            payload_size++;
        }

        memory_copy(file_cluster, (const uint8_t*)payload, payload_size);
    }

    if (ata_write_sector_raw(FAT32_DATA_START_SECTOR + 1U, file_cluster) != 0) {
        return -1;
    }

    return 0;
}

int blockdev_init_disk(void) {
    if (ata_identify() != 0) {
        ERROR_LOG("ATA identify failed");
        return -1;
    }

    if (disk_is_blank()) {
        if (initialize_blank_fat32_image() != 0) {
            ERROR_LOG("failed to format blank IDE disk");
            return -1;
        }
    }

    disk_ready = 1;
    DEBUG_LOG("IDE block device initialized");
    return 0;
}

int blockdev_read_sector(uint32_t sector, void* buffer) {
    uint8_t* destination = (uint8_t*)buffer;

    if (!disk_ready || destination == NULL) {
        return -1;
    }

    if (ata_read_sector_raw(sector, destination) != 0) {
        return -1;
    }

    return 0;
}

int blockdev_write_sector(uint32_t sector, const void* buffer) {
    if (!disk_ready || buffer == NULL) {
        return -1;
    }

    if (ata_write_sector_raw(sector, buffer) != 0) {
        return -1;
    }

    return 0;
}

uint32_t blockdev_sector_count(void) {
    return disk_sector_count;
}
