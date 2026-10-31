/*
 * dimmer.c
 *
 *  Created on: 15 Oct 2026
 *      Author: marku
 */


#include "main.h"
#include "uart_sci.h"
#include "LD2420.h"
#include "BH1750.h"
#include "vars.h"
#include "ssd1963.h"
#include "eerom.h"

#define COUNTER_SAVE_SEC	3600	// store the time counters in the EEPROM once an hour

#define DIM_STEP_MS		50	// time per PWM step, 0..80% takes about 0.8 s
#define LUX_CONV_MS		200	// measuring time BH1750 (H-resolution mode max 180 ms)

#define LONG_START_SEC	(17 * 3600)			// 17:00 local time: BACK_LONG_SEC
#define LONG_END_SEC	(22 * 3600 + 1800)	// 22:30 local time: BACK_SHORT_SEC again

// state of the dimmer, file scope for dimmer_show()
static uint16_t hold_sec;
static bool dark;
static uint8_t level = 20;					// current PWM, same as the init value of the display
static uint8_t target;
static uint16_t last_lux = 0xFFFF;			// 0xFFFF = no measurement
static bool panel_on = true;				// SSD1963_Init() switches the panel on

/*
 * Hold time after movement: BACK_LONG_SEC between 17:00 and 22:30 local time, else BACK_SHORT_SEC.
 * Without a valid time (ID_202) BACK_SHORT_SEC.
 */
static uint16_t hold_time(void)
{
	const volatile home_data_t *d = &gvars.home_data;

	if (d->epoch_rx_ms == 0 || d->epoch < 1600000000u)
		return BACK_SHORT_SEC;

	uint32_t epoch = d->epoch + (home_millis() - d->epoch_rx_ms) / 1000u;
	uint32_t sec_of_day = epoch % 86400u;

	if (sec_of_day >= LONG_START_SEC && sec_of_day < LONG_END_SEC)
		return BACK_LONG_SEC;

	return BACK_SHORT_SEC;
}

/*
 * Backlight control. Call from the main loop.
 * - every second: movement (re)starts hold_time(), BACK_LONG_SEC 17:00..22:30 else BACK_SHORT_SEC, movement is:
 *     LD2420 range is a distance (not LD2420_RANGE_OFF and not LD2420_ERROR), or
 *     PIR (ID_404, sent on change) is set, only on UNIT_BG
 * - while holding the backlight is BACK_LOW (dark room) or BACK_HIGH, else off
 * - dark when lux < LUX_DARK, bright when lux > LUX_BRIGHT, in between the state is kept
 * - the backlight volume (BACK_LOW / BACK_HIGH) is determined every second, not only while holding
 * - lux is measured non-blocking: start_lux_conversion() on the second tick, get_lux_value() after LUX_CONV_MS
 * - the PWM changes one step at a time towards the target
 * - backlight 0: the panel is switched off too (no static image on the liquid crystal, less image retention),
 *   before the backlight goes up again the panel is switched on
 * - every second the operating time counter and, while the backlight is on, the backlight counter
 *   (gvars.eeprom.uptime_sec / backlight_sec) are incremented, stored in the EEPROM once an hour
 */
void process_dimmer(void)
{
	static uint32_t sec_tick;
	static uint32_t save_sec;
	static uint32_t step_tick;
	static uint32_t conv_tick;
	static bool converting;
	static bool init;

	uint32_t now = HAL_GetTick();

	// first call: the settings are read from the EEPROM by init_vars(), not known at compile time
	if (!init)
	{
		init = true;
		hold_sec = hold_time();					// display on after start
		target = BACK_HIGH;
	}

	if ((now - sec_tick) >= 1000)
	{
		sec_tick += 1000;
		if ((now - sec_tick) >= 1000)	// fell behind (blocked for a long time)
			sec_tick = now;

		gvars.eeprom.uptime_sec++;
		if (level > 0)
			gvars.eeprom.backlight_sec++;
		if (++save_sec >= COUNTER_SAVE_SEC)
		{
			save_sec = 0;
			if (WriteStruct2rom(&gvars.eeprom, sizeof(gvars.eeprom)) != EE_ERR_OK)
				tty_printf("ERR store EEPROM counters\r\n");
		}

		uint16_t range = get_ld2420_range();

		// Movement is the LD2420 range only, the PIR is elsewhere
		if (range != LD2420_RANGE_OFF && range != LD2420_ERROR)
			hold_sec = hold_time();
		else if (hold_sec)
			hold_sec--;

		// lux every second, also while the display is off
		converting = (start_lux_conversion(one_time_H_res_mode1) == HAL_OK);
		conv_tick = now;

		// level from the last known lux, corrected after the measurement
		if (hold_sec)
			target = dark ? BACK_LOW : BACK_HIGH;
		else
			target = 0;
	}

	// conversion ready: read the result without waiting
	if (converting && (now - conv_tick) >= LUX_CONV_MS)
	{
		uint16_t lux;

		converting = false;
		if (get_lux_value(&lux) == HAL_OK)
		{
			last_lux = lux;
			if (lux < LUX_DARK)
				dark = true;
			else if (lux > LUX_BRIGHT)
				dark = false;
		}
		if (hold_sec)
			target = dark ? BACK_LOW : BACK_HIGH;
	}

	// panel on before the backlight goes up
	if (!panel_on && target > 0)
	{
		SSD1963_display(1);
		panel_on = true;
	}

	// gradual change of the PWM
	if (level != target && (now - step_tick) >= DIM_STEP_MS)
	{
		step_tick = now;
		if (level < target)
			level++;
		else
			level--;
		SSD1963_backlight(level);
	}

	// backlight off: panel off
	if (panel_on && level == 0 && target == 0)
	{
		SSD1963_display(0);
		panel_on = false;
	}
}

/*
 * Shell "dim": why the backlight is on or off
 */
void dimmer_show(void)
{
	uint16_t range = get_ld2420_range();

	tty_printf("Backlight    %u %% (target %u %%)\r\n", level, target);
	tty_printf("Panel        %s\r\n", panel_on ? "on" : "off");
	tty_printf("Hold         %u s left (movement: %u s)\r\n", hold_sec, hold_time());
	if (range == LD2420_ERROR)
		tty_printf("LD2420       no data\r\n");
	else if (range == LD2420_RANGE_OFF)
		tty_printf("LD2420       no presence\r\n");
	else
		tty_printf("LD2420       presence, range %u cm\r\n", range);
	tty_printf("PIR (ID 404) %s (not used)\r\n", gvars.home_data.pir ? "movement" : "no movement");

	if (last_lux == 0xFFFF)
		tty_printf("Lux          no measurement\r\n");
	else
		tty_printf("Lux          %u (%s, dark < %u, bright > %u)\r\n", last_lux, dark ? "dark" : "bright", LUX_DARK, LUX_BRIGHT);
}
