/*
 * m24xx.c
 *
 *  See m24xx.h - universal driver for the 24AA00..24AA512 I2C EEPROM
 *  family, configured entirely by E2P_VARIANT in the header.
 *
 *  Created on: Nov 1, 2023
 *      Author: mark
 */
#include "main.h"
#include "eerom.h"
#include "m24xx.h"
#include "vars.h"
#include "i2c.h"

static I2C_HandleTypeDef * i2c;

#if E2P_ADDR_BITS == 16U
#define E2P_MEMADD_SIZE   I2C_MEMADD_SIZE_16BIT
#else
#define E2P_MEMADD_SIZE   I2C_MEMADD_SIZE_8BIT
#endif

static bool m24xx_ready(uint16_t device_addr);
static uint16_t e2p_device_addr(uint32_t address);
static uint16_t e2p_mem_addr(uint32_t address);



/*!
 * \brief
 *
 * \param pointer to I2C handler
 *
 * \return - HAL_OK
 */
HAL_StatusTypeDef eeprom_init(I2C_HandleTypeDef * hi2c)
{
	i2c = hi2c;
    return HAL_I2C_IsDeviceReady(i2c, E2P_RIIC_ADDRESS, 1, 100);
}


/*!
 * \brief This function reads the 48-bit node address from a factory-ID
 *        ("E48"/"E64") variant EEPROM. Not all 24AAxx parts have this -
 *        unrelated to E2P_VARIANT, only meaningful if the connected chip
 *        is actually one of those factory-programmed variants.
 *
 * \param p_dst Pointer to the buffer where the 48-bit node address is stored (6 bytes).
 *              The buffer contains:
 *              uint8_t node_addr[0]
 *              uint8_t node_addr[1]
 *              uint8_t node_addr[2]
 *              uint8_t node_addr[3]
 *              uint8_t node_addr[4]
 *              uint8_t node_addr[5]
 *
 * \return -. true on error.
 */
HAL_StatusTypeDef e2p_get_serial_no(uint8_t *p_dst)
{
	uint8_t address = E2P_NODE_ADDRESS;
	HAL_StatusTypeDef res;

	res = HAL_I2C_Master_Transmit(i2c, (uint16_t) E2P_RIIC_ADDRESS, &address, 1, 50);
	res |= HAL_I2C_Master_Receive(i2c, (uint16_t) E2P_RIIC_ADDRESS, p_dst, 6, 50);

	return res;
}


/*!
 * \brief This function reads data from the EEPROM.
 *
 * \param address The address where to read from (0 .. E2P_SIZE_BYTES-1).
 * \param p_dst Pointer to the buffer where the data is stored.
 * \param len Length of the data to read.
 *
 * example: https://github.com/sinadarvi/SD_HAL_AT24/blob/master/at24_hal_i2c.c
 *
 * \return false if successful, true on error.
 */
HAL_StatusTypeDef m24xx_read(uint8_t *p_dst, uint32_t address, uint16_t len)
{
	// Sanity checks, eeprom_init() not called yet: no I2C handle
	if (i2c == 0)
		return (HAL_ERROR);
	if (!len)
		return (HAL_ERROR);
	if ((address + len) > E2P_SIZE_BYTES)
		return (HAL_ERROR);

	return HAL_I2C_Mem_Read(i2c, e2p_device_addr(address), e2p_mem_addr(address), E2P_MEMADD_SIZE, p_dst, len, 50);
}

/*!
 * \brief This function writes data to the EEPROM, splitting the transfer
 *        on page boundaries as required by the part.
 *
 * \param address The address where to write to (0 .. E2P_SIZE_BYTES-1).
 * \param p_src Pointer to the buffer containig data to be written.
 * \param len Length of the data to write.
 *
 * \return false if successful, true on error.
 */
HAL_StatusTypeDef m24xx_write(uint8_t *p_src, uint32_t address, uint16_t len)
{
	uint32_t remaining;
	uint32_t addr;
	uint16_t writelen;

	// Sanity checks, eeprom_init() not called yet: no I2C handle
	if (i2c == 0)
		return (HAL_ERROR);
	if (!len)
		return (HAL_ERROR);
	if ((address + len) > E2P_SIZE_BYTES)
		return (HAL_ERROR);

	remaining = len;
	addr = address;

	while (remaining > 0U)
	{
		uint32_t byte_in_page = addr % E2P_PAGE_SIZE;
		uint32_t room_in_page = E2P_PAGE_SIZE - byte_in_page;
		writelen = (uint16_t)((room_in_page < remaining) ? room_in_page : remaining);

		uint16_t device_addr = e2p_device_addr(addr);

		if (HAL_I2C_Mem_Write(i2c, device_addr, e2p_mem_addr(addr), E2P_MEMADD_SIZE, p_src, writelen, 50) != HAL_OK)
			return (HAL_ERROR);

		// Wait for the EEPROM's internal write cycle to finish by polling for an I2C ACK
		// (typ. a few ms per the datasheet) instead of a fixed worst-case delay, with a
		// safety timeout in case the device never comes back. Poll the SAME (block-select)
		// device address just written to - on the parts with block-select bits
		// (E2P_BLOCK_BITS > 0) each block acts as its own I2C device address.
		{
			uint32_t wait_start = HAL_GetTick();
			while (!m24xx_ready(device_addr))
			{
				if ((HAL_GetTick() - wait_start) >= 10U)
					return (HAL_ERROR);
			}
		}

		p_src += writelen;
		addr += writelen;
		remaining -= writelen;
	}

	return (HAL_OK);
}

/*!
 * \brief This function checks if the memory is busy writing/erasing.
 *
 * \param -.
 *
 * \return True if successful, false on error.
 */
static bool m24xx_ready(uint16_t device_addr)
{
	return (HAL_I2C_IsDeviceReady(i2c, device_addr, 1, 10) == HAL_OK);
}

/*!
 * \brief Splits a linear byte address into the (device address, memory address) pair the chip actually expects. For the small parts
 *        (24AA04/08/16) that have too little room in one memory-address byte for their full size, the missing top bits are instead
 *        folded into the device address, in the same bit position the A0/A1/A2 chip-select pins would otherwise occupy - that's the
 *        standard "block select" addressing scheme those parts use. Parts with a 16-bit memory address (E2P_ADDR_BITS == 16) never
 *        need this: the whole address always fits in the memory-address field, so E2P_BLOCK_BITS is 0 for all of them.
 */
static uint16_t e2p_device_addr(uint32_t address)
{
#if E2P_BLOCK_BITS > 0U
	uint8_t block = (uint8_t)((address >> 8) & ((1U << E2P_BLOCK_BITS) - 1U));
	return (uint16_t)(E2P_RIIC_ADDRESS | (block << 1));
#else
	(void)address;
	return (uint16_t)E2P_RIIC_ADDRESS;
#endif
}

static uint16_t e2p_mem_addr(uint32_t address)
{
#if E2P_BLOCK_BITS > 0U
	return (uint16_t)(address & 0xFFU);	/* block bits already moved into the device address above */
#else
	return (uint16_t)address;
#endif
}
