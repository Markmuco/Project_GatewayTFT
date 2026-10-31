/*
 * flash_disk.h
 *
 * MX25L3233F as a disk of 512 byte blocks, used by the USB mass storage (ux_device_msc.c)
 * and by FileX (fx_stm32_custom_driver.c), so both see the same FAT file system.
 */

#ifndef FLASH_DISK_H_
#define FLASH_DISK_H_

#include <stdint.h>
#include <stdbool.h>
#include "mx25l.h"

#define FLASH_DISK_BLOCK_SIZE	512U
#define FLASH_DISK_BLOCKS		(MX25L_SIZE / FLASH_DISK_BLOCK_SIZE)

bool flash_disk_ready(void);
bool flash_disk_read(uint32_t lba, uint8_t *data, uint32_t blocks);
bool flash_disk_write(uint32_t lba, const uint8_t *data, uint32_t blocks);

void flash_disk_changed(void);		// FileX changed the disk: the PC must read it again
bool flash_disk_take_changed(void);	// true once after flash_disk_changed(), used by the USB mass storage

#endif /* FLASH_DISK_H_ */
