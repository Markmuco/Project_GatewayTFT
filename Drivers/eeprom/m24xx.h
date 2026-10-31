/*
 * m24xx.h
 *
 *  Universal driver for the Microchip 24AAxx / 24LCxx / 24FCxx (and
 *  ST M24Cxx-compatible) I2C EEPROM family, 24AA00 through 24AA512.
 *  Select the part actually on the board by changing the single
 *  E2P_VARIANT define below - everything else (address width, page size,
 *  block-select bits) follows automatically from that one choice.
 *
 *  Created on: Nov 1, 2023
 *      Author: mark
 */

#ifndef INC_M24XX_H_
#define INC_M24XX_H_

// E2P device base address (7-bit address 0x50, shifted for the HAL's <<1 convention).
#define E2P_RIIC_ADDRESS   (0xA0) //50

// UUID 48-bit (only meaningful if the connected chip is a factory-ID variant
// such as 24AA02E48/24AA025E48/24AA64E48 - unrelated to E2P_VARIANT below).
#define E2P_NODE_ADDRESS   (0xFA)

/* --- Available parts - pick exactly one as E2P_VARIANT below --- */
#define E2P_24AA00    0
#define E2P_24AA01    1
#define E2P_24AA02    2
#define E2P_24AA04    3
#define E2P_24AA08    4
#define E2P_24AA16    5
#define E2P_24AA32    6
#define E2P_24AA64    7
#define E2P_24AA128   8
#define E2P_24AA256   9
#define E2P_24AA512   10

// *** Select the EEPROM actually on this board here. ***
#define E2P_VARIANT   E2P_24AA02

#if E2P_VARIANT == E2P_24AA00
// 24AA00 - 128 bit = 16 bytes, no real page write (page size 1 byte), single-byte address
#define E2P_SIZE_BYTES     (16U)
#define E2P_PAGE_SIZE      (1U)
#define E2P_ADDR_BITS      (8U)
#define E2P_BLOCK_BITS     (0U)
#elif E2P_VARIANT == E2P_24AA01
// 24AA01 - 1Kbit = 128 bytes = 16 pages x 8 bytes, single-byte address
#define E2P_SIZE_BYTES     (128U)
#define E2P_PAGE_SIZE      (8U)
#define E2P_ADDR_BITS      (8U)
#define E2P_BLOCK_BITS     (0U)
#elif E2P_VARIANT == E2P_24AA02
// 24AA02 / 24AA02E48T - 2Kbit = 256 bytes = 32 pages x 8 bytes, single-byte address
#define E2P_SIZE_BYTES     (256U)
#define E2P_PAGE_SIZE      (8U)
#define E2P_ADDR_BITS      (8U)
#define E2P_BLOCK_BITS     (0U)
#elif E2P_VARIANT == E2P_24AA04
// 24AA04 - 4Kbit = 512 bytes = 32 pages x 16 bytes, single-byte address + 1 block-select bit in the device address
#define E2P_SIZE_BYTES     (512U)
#define E2P_PAGE_SIZE      (16U)
#define E2P_ADDR_BITS      (8U)
#define E2P_BLOCK_BITS     (1U)
#elif E2P_VARIANT == E2P_24AA08
// 24AA08 - 8Kbit = 1024 bytes = 64 pages x 16 bytes, single-byte address + 2 block-select bits in the device address
#define E2P_SIZE_BYTES     (1024U)
#define E2P_PAGE_SIZE      (16U)
#define E2P_ADDR_BITS      (8U)
#define E2P_BLOCK_BITS     (2U)
#elif E2P_VARIANT == E2P_24AA16
// 24AA16 - 16Kbit = 2048 bytes = 128 pages x 16 bytes, single-byte address + 3 block-select bits in the device address
#define E2P_SIZE_BYTES     (2048U)
#define E2P_PAGE_SIZE      (16U)
#define E2P_ADDR_BITS      (8U)
#define E2P_BLOCK_BITS     (3U)
#elif E2P_VARIANT == E2P_24AA32
// 24AA32A - 32Kbit = 4096 bytes = 128 pages x 32 bytes, two-byte address
#define E2P_SIZE_BYTES     (4096U)
#define E2P_PAGE_SIZE      (32U)
#define E2P_ADDR_BITS      (16U)
#define E2P_BLOCK_BITS     (0U)
#elif E2P_VARIANT == E2P_24AA64
// 24AA64 - 64Kbit = 8192 bytes = 256 pages x 32 bytes, two-byte address
#define E2P_SIZE_BYTES     (8192U)
#define E2P_PAGE_SIZE      (32U)
#define E2P_ADDR_BITS      (16U)
#define E2P_BLOCK_BITS     (0U)
#elif E2P_VARIANT == E2P_24AA128
// 24AA128 - 128Kbit = 16384 bytes = 256 pages x 64 bytes, two-byte address
#define E2P_SIZE_BYTES     (16384U)
#define E2P_PAGE_SIZE      (64U)
#define E2P_ADDR_BITS      (16U)
#define E2P_BLOCK_BITS     (0U)
#elif E2P_VARIANT == E2P_24AA256
// 24AA256 - 256Kbit = 32768 bytes = 512 pages x 64 bytes, two-byte address
#define E2P_SIZE_BYTES     (32768U)
#define E2P_PAGE_SIZE      (64U)
#define E2P_ADDR_BITS      (16U)
#define E2P_BLOCK_BITS     (0U)
#elif E2P_VARIANT == E2P_24AA512
// 24AA512 - 512Kbit = 65536 bytes = 512 pages x 128 bytes, two-byte address
#define E2P_SIZE_BYTES     (65536U)
#define E2P_PAGE_SIZE      (128U)
#define E2P_ADDR_BITS      (16U)
#define E2P_BLOCK_BITS     (0U)
#else
#error "m24xx.h: set E2P_VARIANT to one of the E2P_24AAxx values above"
#endif

HAL_StatusTypeDef eeprom_init(I2C_HandleTypeDef * hi2c);
HAL_StatusTypeDef e2p_get_serial_no(uint8_t *p_dst);
HAL_StatusTypeDef m24xx_write(uint8_t *p_src, uint32_t address, uint16_t len);
HAL_StatusTypeDef m24xx_read(uint8_t *p_dst, uint32_t address, uint16_t len);

#endif /* INC_M24XX_H_ */
