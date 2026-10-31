/*
 * vars.c
 *
 *  Created on: 14 Oct 2026
 *      Author: marku
 */

#include "main.h"
#include "rtc.h"
#include "vars.h"
#include "eerom.h"
#include "uart_sci.h"


/*
 * Initialize the gvars struct from EEPROM, or use defaults if not found.
 */
void init_vars(void)
{
	store_EE_ERR_t res = ReadStruct2rom(&gvars.eeprom, sizeof(gvars.eeprom));

	if (res == EE_ERR_OK_COPY)
	{
		tty_printf("EEPROM primary block corrupt\r\n");
		// primary block corrupt, the copy is valid: repair the primary block
		WriteStruct2rom(&gvars.eeprom, sizeof(gvars.eeprom));
	}
	else if (res != EE_ERR_OK)
	{
		// eeprom written by a version without the time counters: keep the settings
		struct
		{
			uint16_t back_low, back_high, lux_dark, lux_bright, back_short_sec, back_long_sec;
		} old;

		if (ReadStruct2rom(&old, sizeof(old)) == EE_ERR_OK)
		{
			gvars.eeprom.back_low = old.back_low;
			gvars.eeprom.back_high = old.back_high;
			gvars.eeprom.lux_dark = old.lux_dark;
			gvars.eeprom.lux_bright = old.lux_bright;
			gvars.eeprom.back_short_sec = old.back_short_sec;
			gvars.eeprom.back_long_sec = old.back_long_sec;
			gvars.eeprom.uptime_sec = 0;
			gvars.eeprom.backlight_sec = 0;
			WriteStruct2rom(&gvars.eeprom, sizeof(gvars.eeprom));
			tty_printf("EEPROM upgraded, settings kept\r\n");
			return;
		}

		gvars.eeprom.uptime_sec = 0;
		gvars.eeprom.backlight_sec = 0;
		gvars.eeprom.back_low = 10;
		gvars.eeprom.back_high = 80;
		gvars.eeprom.lux_dark = 400;
		gvars.eeprom.lux_bright = 800;
		gvars.eeprom.back_short_sec = 300;
		gvars.eeprom.back_long_sec = 900;
		WriteStruct2rom(&gvars.eeprom, sizeof(gvars.eeprom));
		tty_printf("EEPROM CRC error, using defaults and writing to EEPROM\r\n");
	}
}


/*
 * Milliseconds since start, 0 is used as 'never received'
 */
uint32_t home_millis(void)
{
	uint32_t t = HAL_GetTick();

	return t ? t : 1;
}

/*
 * RTC backup registers, keep the day totals over a reset
 */
void home_backup_write(uint32_t index, uint32_t value)
{
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0 + index, value);
}

/*
 *
 */
uint32_t home_backup_read(uint32_t index)
{
	return HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0 + index);
}
