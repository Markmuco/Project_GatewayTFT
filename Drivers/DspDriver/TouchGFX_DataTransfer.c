/*
 * TouchGFX_DataTransfer.c
 *
 *  Created on: 27 груд. 2021 р.
 *      Author: tabur
 */

#include "TouchGFX_DataTransfer.h"
#include "ssd1963.h"

static uint8_t isTransmittingData = 0;

uint32_t touchgfxDisplayDriverTransmitActive(void)
{
	return isTransmittingData;
}

/*
 * Interface to TouchGFX generated code
 */
void touchgfxDisplayDriverTransmitBlock(uint8_t* pixels, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	uint8_t lo = 0, hi = 0;
	uint32_t n_bytes = w * h;

	isTransmittingData = 1;

	SSD1963_Set_Work_Area(x, y, w, h);

	while (n_bytes)
	{
		lo = *pixels++;
		hi = *pixels++;
		LCD_DataWrite(hi << 8 | lo);
		n_bytes--;
	}

	isTransmittingData = 0;
	DisplayDriver_TransferCompleteCallback();
}

