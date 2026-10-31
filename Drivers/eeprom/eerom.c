/*
 * flash_ee.c
 *
 *  Created on: 25 sep. 2016
 *      Author: Mark
 */

#include "main.h"
#include "eerom.h"
#include "crc.h"
#include "m24xx.h"
#if EEROM_UNIT_TEST>0
#include "uart_sci.h"
#endif


static store_EE_ERR_t do_WriteStruct2rom(void *data, uint32_t start_addr, uint16_t block_size, uint16_t data_size);

/*!
 * \brief This function Reads a struct from internal Flash
 *
 * \param - p_data pointer to structure to save
 *      - sizeof structure
 *
 * \return EE_ERR_OK / EE_ERR_CRC
 */
store_EE_ERR_t ReadStruct2rom(void *p_data, uint16_t data_size)
{
  uint32_t *p_heap;
  // round 4 bytes
  data_size = ((data_size + 3) / 4) * 4;

  if (data_size == 0)
	  return EE_ERR_SIZE;

  // allocate datablock+crc32
  p_heap = malloc(data_size + 4);
  if (p_heap == 0)
    return EE_ERR_MEM;

  // round 4 bytes
  uint16_t aligned_data_size = ((data_size + 3) / 4) * 4;

  if (m24xx_read((uint8_t *) p_heap, EE_START_ADDR, aligned_data_size + 4) != HAL_OK)
  {
	    free(p_heap);
		return EE_ERR_READ;
  }

  // CRC check the block including the CRC itself will return 0
  if (HAL_CRC_Calculate(&hcrc, p_heap, (aligned_data_size + 4) / 4) == 0)
  {
    memcpy(p_data, p_heap, data_size);
    free(p_heap);
    return EE_ERR_OK;
  }

  // checksum fail try block 2
  if (m24xx_read((uint8_t *) p_heap, EE_COPY_START_ADDR, aligned_data_size + 4) != HAL_OK)
  {
	    free(p_heap);
		return EE_ERR_READ;
  }

  // CRC check the block including the CRC itself will return 0
  if (HAL_CRC_Calculate(&hcrc, p_heap, (aligned_data_size + 4) / 4) == 0)
  {
    memcpy(p_data, p_heap, data_size);
    free(p_heap);
    return EE_ERR_OK_COPY;
  }

  free(p_heap);
  return EE_ERR_CRC;
}

/*!
 * \brief This function Saves a struct to internal Flash
 * 		Set the CRC module to WORD
 *
 * \param	- p_data pointer to structure to save
 * 			- sizeof structure
 *
 * \return		EE_ERR_OK / EE_ERR_CRC / EE_ERR_WRITE / EE_ERR_ERASE / EE_ERR_SIZE
 */
store_EE_ERR_t WriteStruct2rom(void *p_data, uint16_t data_size)
{
	store_EE_ERR_t res = EE_ERR_OK;

	if (data_size + 4 > EE_SIZE)
		return EE_ERR_SIZE;
	
	if (data_size == 0)
		  return EE_ERR_SIZE;

	// write block 1
	res = do_WriteStruct2rom(p_data, EE_START_ADDR, EE_SIZE, data_size);
	// write block 2
	res |= do_WriteStruct2rom(p_data, EE_COPY_START_ADDR, EE_SIZE, data_size);

	return res;
}

/*!
 * \brief This function Saves a struct to internal Flash
 *
 * \param	- p_data pointer to structure to save
 *			- start_addr Flash save adress
 *			- block_size Flash erase block size
 * 			- data_size sizeof structure
 *
 * \return EE_ERR_OK / EE_ERR_CRC / EE_ERR_WRITE / EE_ERR_ERASE
 */
static store_EE_ERR_t do_WriteStruct2rom(void *data, uint32_t start_addr, uint16_t block_size, uint16_t data_size)
{
	uint32_t *p_heap;
	// round 4 bytes
	data_size = ((data_size + 3) / 4) * 4;

	// allocate datablock+crc32
	p_heap = malloc(data_size + 4);
	if (p_heap == 0)
		return EE_ERR_MEM;

	//tty_printf("Heap Memory 0x%08X\r\n", p_heap);

	// copy the datablock to the heap to add the crc32 on top
	memcpy(p_heap, data, data_size);

	/*##-2- Compute the CRC in Words ###################################*/

  //uint32_t crc;
	*(p_heap + data_size / 4) = HAL_CRC_Calculate(&hcrc, p_heap, data_size / 4);

	//tty_printf("check crc 0x%08X heap\r\n", calc_crc32(p_heap, (data_size + 4) / 4));
  //tty_printf("add crc 0x%08X heap\r\n", crc);

	if (m24xx_write((uint8_t*) p_heap, start_addr, data_size + 4) != HAL_OK)
	{
		free(p_heap);
		return EE_ERR_WRITE;
	}

	/*##-3- Check written data ###################################*/
	m24xx_read((uint8_t *) p_heap, start_addr, data_size + 4);

  // CRC check the block including the CRC itself will return 0
  if (HAL_CRC_Calculate(&hcrc, p_heap, (data_size + 4) / 4) == 0)
  {
    free(p_heap);
    return EE_ERR_OK;
  }

	free(p_heap);
	return EE_ERR_CRC;
}

#if EEROM_UNIT_TEST>0
static char * err2str(store_EE_ERR_t key)
{
	switch (key)
	{
	case EE_ERR_OK:
		return "EE_ERR_OK";
	case EE_ERR_OK_COPY:
		return "EE_ERR_OK_COPY";
	case EE_ERR_CRC:
		return "EE_ERR_CRC";
	case EE_ERR_ERASE:
		return "EE_ERR_ERASE";
	case EE_ERR_WRITE:
		return "EE_ERR_WRITE";
	case EE_ERR_SIZE:
		return "EE_ERR_SIZE";
	case EE_ERR_MEM:
		return "EE_ERR_MEM";
	case EE_ERR_READ:
		return "EE_ERR_READ";
	default:
		return "??";
		break;
	}
}

/*
 *
 */
void test_eerom(void)
{
	store_EE_ERR_t err;

	char testdata[] = "The quick brown fox jumps over the lazy dog";

	err = WriteStruct2rom(testdata, strlen(testdata));
	tty_printf("Write %s\r\n", err2str(err));

	uint16_t len = strlen(testdata);

	memset(testdata, 0x00, strlen(testdata));

	err = ReadStruct2rom(testdata, len);
	tty_printf("Read %s = %s\r\n", err2str(err), testdata);

	tty_printf("corrupt primairy block\r\n");

	memset(testdata, 0x00, strlen(testdata));
	if (m24xx_write((uint8_t*) testdata, 0x00, 4) != HAL_OK)
		tty_printf("Err write eerom\r\n");

	err = ReadStruct2rom(testdata, len);
	tty_printf("Read %s = %s\r\n", err2str(err), testdata);
}
#endif

