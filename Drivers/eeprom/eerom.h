/*
 * eerom.h
 *
 *  Created on: 1 dec. 2021
 *      Author: Mark
 */

#ifndef EEROM_H_
#define EEROM_H_

#include "m24xx.h"

// The primary copy lives in the first half of the EEPROM, the redundant
// (CRC-checked fallback, see ReadStruct2rom()) copy in the second half -
// derived from E2P_SIZE_BYTES (m24xx.h) so this always fits whichever
// EEPROM is actually selected there, instead of assuming a fixed 256-byte
// part. EE_SIZE is therefore the max size (struct + 4-byte CRC) of ONE
// copy, not the whole chip.
#define EE_START_ADDR       (0x00U)
#define EE_COPY_START_ADDR  (E2P_SIZE_BYTES / 2U)
#define EE_SIZE             (E2P_SIZE_BYTES / 2U)

#define EEROM_UNIT_TEST		(0)		// functional test

typedef enum
{
	EE_ERR_OK,
	EE_ERR_OK_COPY,
	EE_ERR_CRC,
	EE_ERR_ERASE,
	EE_ERR_WRITE,
	EE_ERR_READ,
	EE_ERR_SIZE,
	EE_ERR_MEM
}store_EE_ERR_t;


store_EE_ERR_t ReadStruct2rom(void * p_data, uint16_t data_size);
store_EE_ERR_t WriteStruct2rom(void * data, uint16_t len);
#if EEROM_UNIT_TEST>0
void test_eerom(void);
#endif

#endif /* FLASH_EE_H_ */
