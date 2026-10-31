/*
 * mx25l.h
 *
 * Driver for the Macronix MX25L3233F (32 Mbit / 4 MByte SPI NOR flash)
 * on OCTOSPI1 in single line SPI mode.
 */

#ifndef MX25L_H_
#define MX25L_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MX25L_SIZE			(4U * 1024U * 1024U)
#define MX25L_PAGE_SIZE		256U
#define MX25L_SECTOR_SIZE	4096U

bool mx25l_init(void);
bool mx25l_read(uint32_t addr, uint8_t *data, uint32_t len);
bool mx25l_write(uint32_t addr, const uint8_t *data, uint32_t len);	// area must be erased
bool mx25l_erase_sector(uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif /* MX25L_H_ */
