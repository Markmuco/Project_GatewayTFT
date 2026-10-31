/*
 * flash_disk.c
 *
 * MX25L3233F as a disk of 512 byte blocks, used by the USB mass storage (ux_device_msc.c)
 * and by FileX (fx_stm32_custom_driver.c), so both see the same FAT file system.
 *
 * Both callers run in the main loop (USBX standalone, FileX single thread),
 * so the sector buffer is never used twice at the same time.
 */

#include <string.h>
#include "vars.h"
#include "flash_disk.h"

#define BLOCKS_PER_SECTOR	(MX25L_SECTOR_SIZE / FLASH_DISK_BLOCK_SIZE)

static uint8_t sector_buf[MX25L_SECTOR_SIZE];
static volatile bool disk_changed;

/*
 * Flash found and the request within the disk
 */
static bool in_range(uint32_t lba, uint32_t blocks)
{
	return lba < FLASH_DISK_BLOCKS && blocks <= FLASH_DISK_BLOCKS - lba;
}

/*
 * The PC caches the FAT and directories: after a change by FileX (log file) the USB mass storage
 * reports "medium may have changed", then the PC reads the disk again.
 */
void flash_disk_changed(void)
{
	disk_changed = true;
}

bool flash_disk_take_changed(void)
{
	bool changed = disk_changed;

	disk_changed = false;
	return changed;
}

bool flash_disk_ready(void)
{
	return gvars.mx25l_ok;
}

bool flash_disk_read(uint32_t lba, uint8_t *data, uint32_t blocks)
{
	if (!flash_disk_ready() || !in_range(lba, blocks))
		return false;

	return mx25l_read(lba * FLASH_DISK_BLOCK_SIZE, data, blocks * FLASH_DISK_BLOCK_SIZE);
}

/*
 * Write through: per 4 kB flash sector read the old contents (only when the sector
 * is not completely overwritten), merge the new blocks, erase and program.
 */
bool flash_disk_write(uint32_t lba, const uint8_t *data, uint32_t blocks)
{
	if (!flash_disk_ready() || !in_range(lba, blocks))
		return false;

	while (blocks > 0)
	{
		uint32_t first = lba % BLOCKS_PER_SECTOR;
		uint32_t count = BLOCKS_PER_SECTOR - first;
		uint32_t addr = (lba / BLOCKS_PER_SECTOR) * MX25L_SECTOR_SIZE;

		if (count > blocks)
			count = blocks;

		if (count < BLOCKS_PER_SECTOR && !mx25l_read(addr, sector_buf, MX25L_SECTOR_SIZE))
			return false;

		memcpy(&sector_buf[first * FLASH_DISK_BLOCK_SIZE], data, count * FLASH_DISK_BLOCK_SIZE);

		if (!mx25l_erase_sector(addr) || !mx25l_write(addr, sector_buf, MX25L_SECTOR_SIZE))
			return false;

		lba += count;
		blocks -= count;
		data += count * FLASH_DISK_BLOCK_SIZE;
	}
	return true;
}
