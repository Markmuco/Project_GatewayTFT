#include <stdlib.h>
#include <string.h>

#include "shell_usercmd.h"
#include "shell.h"

#include "main.h"
#include "uart_sci.h"
#include "ms_timers.h"
#include "ssd1963.h"
#include "fat_routines.h"
#include "i2c.h"
#include "BH1750.h"
#include "LD2420.h"
#include "eerom.h"
#include "iwdg.h"
#include "logging.h"
#include "dimmer.h"

// -------------------- USER / APPLICATION COMMANDS --------------------

/*!
 * \brief
 *
 * \ Parameter
 *
 * \return -.
 */
static void sh_reboot(char *argv)
{
	uint8_t nr;

	if (shell_next_argv(&argv))
	{
		nr = strtol(argv, 0, 10);
	}
	tty_printf("NVICReset\r\n");
	HAL_Delay(10);
	NVIC_SystemReset();
}

/*!
 * \brief
 *
 * \param
 *
 * \return -.
 */
static void sh_i2cscan(char *param)
{
	for (int var = 0; var < 0xFF; var += 2)
	{
		if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t) var, 1, 100) == HAL_OK)
			tty_printf("I2C2 0x%02X %d\r\n", var, var);
	}

}

/*!
 * \brief
 *
 * \param
 *
 * \return -.
 */
static void sh_lux(char *param)
{
	uint16_t lux;
	if (direct_lux_value(&lux, cont_H_res_mode1, 200) == HAL_OK)
		tty_printf("Lux:            %d\r\n", lux);
	else
		tty_printf(" ERR Lux read: %d\r\n");
}

/*!
 * \brief Show or change one EEPROM setting, e.g. "backlow" shows, "backlow 15" sets and saves
 *
 * \param argv command line, name text, pointer to the value in gvars.eeprom, allowed range
 *
 * \return -.
 */
static void eeprom_setting(char *argv, const char *name, uint16_t *value, uint16_t min, uint16_t max)
{
	if (shell_next_argv(&argv))
	{
		char *end;
		long nr = strtol(argv, &end, 10);

		if (end == argv || nr < min || nr > max)
		{
			tty_printf("ERR %s range %u..%u\r\n", name, min, max);
			return;
		}
		*value = (uint16_t) nr;
		WriteStruct2rom(&gvars.eeprom, sizeof(gvars.eeprom));
	}
	tty_printf("%-16s %u\r\n", name, *value);
}

static void sh_backlow(char *argv)
{
	eeprom_setting(argv, "Backlight low:", &gvars.eeprom.back_low, 1, 100);
}

static void sh_backhigh(char *argv)
{
	eeprom_setting(argv, "Backlight high:", &gvars.eeprom.back_high, 1, 100);
}

static void sh_luxdark(char *argv)
{
	eeprom_setting(argv, "Lux dark:", &gvars.eeprom.lux_dark, 0, 65535);
}

static void sh_luxbright(char *argv)
{
	eeprom_setting(argv, "Lux bright:", &gvars.eeprom.lux_bright, 0, 65535);
}

static void sh_holdshort(char *argv)
{
	eeprom_setting(argv, "Hold short (s):", &gvars.eeprom.back_short_sec, 10, 65535);
}

static void sh_holdlong(char *argv)
{
	eeprom_setting(argv, "Hold long (s):", &gvars.eeprom.back_long_sec, 10, 65535);
}


/*!
 * \brief
 *
 * \ Parameter
 *
 * \return -.
 */
#define APP_HEADER_GAP	0x200u	// application header (MYSEC) is this far in front of the vector table
#define BL_HEADER_SIZE	48u		// MYSEC of the bootloader, directly in front of the application header

void sh_ver(char *argv)
{
	extern const app_info_t c_app_info;				// main.c
	extern uint32_t _main_app_start_address;		// linker script, address of the vector table
	const uint32_t app_header = (uint32_t) &_main_app_start_address - APP_HEADER_GAP;
	const bl_info_t *p_bl_info = (const bl_info_t*) (app_header - BL_HEADER_SIZE);
	const app_info_t *p_app_info = &c_app_info;

	tty_printf(VT100_COLOR_GREEN);
	if (p_bl_info->key == APP_KEY)
		tty_printf("Bootloader  %X.%02X (%s %s)\r\n", ((p_bl_info->version >> 24) & 0xFF), ((p_bl_info->version >> 16) & 0xFF), p_bl_info->build_date, p_bl_info->build_time);
	else
		tty_printf("Bootlaoder  unknown\r\n");
	tty_printf(VT100_COLOR_DEFAULT);
	tty_printf("Application %X.%02X (%s %s)\r\n", ((p_app_info->version >> 24) & 0xFF), ((p_app_info->version >> 16) & 0xFF), p_app_info->build_date, p_app_info->build_time);
}

/*!
 * \brief
 *
 * \ Parameter
 *
 * \return -.
 */
static void show_age(const char *name, uint32_t rx_ms)
{
	if (rx_ms == 0)
		tty_printf("%-12s never received\r\n", name);
	else
		tty_printf("%-12s %lu s ago\r\n", name, (unsigned long) ((home_millis() - rx_ms) / 1000));
}

static void sh_show(char *param)
{
	const volatile home_data_t *d = &gvars.home_data;
	int32_t t = d->outdoor_temp;

	tty_printf(VT100_COLOR_GREEN);
	tty_printf("--- gvars ---\r\n");
	tty_printf(VT100_COLOR_DEFAULT);
	tty_printf("mx25l_ok     %s\r\n", gvars.mx25l_ok ? "yes" : "no");

	tty_printf(VT100_COLOR_GREEN);
	tty_printf("--- eeprom ---\r\n");
	tty_printf(VT100_COLOR_DEFAULT);

	tty_printf("back_low     %u %%\r\n", gvars.eeprom.back_low);
	tty_printf("back_high    %u %%\r\n", gvars.eeprom.back_high);
	tty_printf("lux_dark     %u\r\n", gvars.eeprom.lux_dark);
	tty_printf("lux_bright   %u\r\n", gvars.eeprom.lux_bright);
	tty_printf("short_sec    %u s\r\n", gvars.eeprom.back_short_sec);
	tty_printf("long_sec     %u s\r\n", gvars.eeprom.back_long_sec);
	{
		uint32_t up = gvars.eeprom.uptime_sec;
		uint32_t bl = gvars.eeprom.backlight_sec;
		uint32_t pct10 = up ? (uint32_t) (((uint64_t) bl * 1000u) / up) : 0;

		tty_printf("uptime       %lu s (%lud %luh %lum)\r\n", (unsigned long) up, (unsigned long) (up / 86400u),
				(unsigned long) ((up % 86400u) / 3600u), (unsigned long) ((up % 3600u) / 60u));
		tty_printf("backlight on %lu s (%lud %luh %lum) = %lu.%lu %% of uptime\r\n", (unsigned long) bl,
				(unsigned long) (bl / 86400u), (unsigned long) ((bl % 86400u) / 3600u), (unsigned long) ((bl % 3600u) / 60u),
				(unsigned long) (pct10 / 10u), (unsigned long) (pct10 % 10u));
	}

	tty_printf(VT100_COLOR_GREEN);
	tty_printf("--- energy (ID 300) ---\r\n");
	tty_printf(VT100_COLOR_DEFAULT);

	tty_printf("consume      %ld W\r\n", (long) d->consume_w);
	tty_printf("solar        %ld W\r\n", (long) d->solar_w);
	tty_printf("ev           %ld W\r\n", (long) d->ev_w);
	tty_printf("h2o          %ld L/h\r\n", (long) d->h2o);
	show_age("energy", d->energy_rx_ms);

	tty_printf(VT100_COLOR_GREEN);
	tty_printf("--- grid (ID 320) ---\r\n");
	tty_printf(VT100_COLOR_DEFAULT);

	tty_printf("grid         %d W (%s)\r\n", d->grid_w, d->grid_w < 0 ? "export" : "import");
	tty_printf("tariff       %u (%s)\r\n", d->tariff, d->tariff == 1 ? "low" : "normal");
	show_age("grid", d->grid_rx_ms);

	tty_printf(VT100_COLOR_GREEN);
	tty_printf("--- powerwall (ID 138/139) ---\r\n");
	tty_printf(VT100_COLOR_DEFAULT);

	tty_printf("soc          %u %%\r\n", d->battery_soc);
	show_age("soc", d->soc_rx_ms);
	tty_printf("charge       %d W\r\n", d->battery_charge_w);
	tty_printf("produce      %d W\r\n", d->battery_produce_w);
	show_age("battery", d->battery_rx_ms);

	tty_printf(VT100_COLOR_GREEN);
	tty_printf("--- sensors ---\r\n");
	tty_printf(VT100_COLOR_DEFAULT);

	tty_printf("pressure     %u.%u mbar\r\n", d->pressure_mbar10 / 10, d->pressure_mbar10 % 10);
	show_age("pressure", d->pressure_rx_ms);
	tty_printf("humidity     %u %%\r\n", d->humidity);
	show_age("humidity", d->humidity_rx_ms);
	tty_printf("outdoor      %s%ld.%ld C\r\n", t < 0 ? "-" : "", (long) (labs(t) / 10), (long) (labs(t) % 10));
	show_age("outdoor", d->outdoor_temp_rx_ms);
	tty_printf("wind         %d.%d km/h\r\n", d->wind_hmh / 10, d->wind_hmh % 10);
	show_age("wind", d->wind_rx_ms);
	tty_printf("rain         %u (%s)\r\n", d->rain_adc, d->rain_adc > 700 ? "dry" : (d->rain_adc < 100 ? "rain" : "wet"));
	show_age("rain", d->rain_rx_ms);
	tty_printf("rain count   %lu x 0.3 mm\r\n", (unsigned long) d->rain_count);
	show_age("rain count", d->rain_count_rx_ms);
	tty_printf("ev state     %u\r\n", d->ev_state);
	show_age("ev state", d->ev_state_rx_ms);
	tty_printf("ev sessions  %u (since start)\r\n", d->ev_session_cnt);

	tty_printf(VT100_COLOR_GREEN);
	tty_printf("--- time (ID 202) ---\r\n");
	tty_printf(VT100_COLOR_DEFAULT);

	if (d->epoch_rx_ms)
	{
		uint32_t now = d->epoch + (home_millis() - d->epoch_rx_ms) / 1000;
		tty_printf("epoch        %lu  %02lu:%02lu:%02lu\r\n", (unsigned long) now, (unsigned long) (now % 86400 / 3600), (unsigned long) (now % 3600 / 60), (unsigned long) (now % 60));
	}
	show_age("epoch", d->epoch_rx_ms);
}

/*!
 * \brief
 *
 * \ Parameter
 *
 * \return -.
 */
static void sh_dir(char *argv)
{
	char *path = NULL;

	// optional directory, e.g. "dir LOGS"
	if (shell_next_argv(&argv))
		path = argv;

	print_dir(path);
}

/*
 * File name after the command, trailing spaces removed, NULL when missing
 */
static char *file_argv(char *argv)
{
	if (!shell_next_argv(&argv))
		return NULL;

	size_t len = strlen(argv);
	while (len > 0 && argv[len - 1] == ' ')
		argv[--len] = 0;

	return argv;
}

/*
 * "type tft_2026.csv"
 */
static void sh_type(char *argv)
{
	type_file(file_argv(argv));
}

/*
 * "del tft_2026.csv"
 */
static void sh_del(char *argv)
{
	remove_file(file_argv(argv));
}

/*
 * Write a log line now
 */
static void sh_log(char *argv)
{
	log_now();
}

/*!
 * \brief
 *
 * \ Parameter
 *
 * \return -.
 */
static void sh_dim(char *argv)
{
	dimmer_show();
}

static void sh_back(char *argv)
{
	uint8_t nr;

	if (shell_next_argv(&argv))
	{
		nr = strtol(argv, 0, 10);
		SSD1963_backlight(nr);
		tty_printf("Backlight %d\r\n", nr);

	}
}

/*!
 * \brief
 *
 * \ Parameter
 *
 * \return -.
 */
static void sh_range(char *argv)
{
	uint16_t range = get_ld2420_range();

	switch (range)
	{
	case LD2420_ERROR:
		tty_printf("no module data\r\n");
		break;
	case LD2420_RANGE_OFF:
		tty_printf("no presence\r\n");
		break;
	default:
		tty_printf("range %d\r\n", range);
		break;
	}

}

// -------------------- REGISTRATION --------------------

void shell_register_cmds(void)
{
	// First add core shell commands
	(void) add_cmd("help", "This tekst", sh_help);
	(void) add_cmd("reboot", "reboot CPU", sh_reboot);
	(void) add_cmd("ver", "Software version", sh_ver);
#if LOGIN_ENABLE>0
    (void)add_cmd("login",  "Login shell",       sh_login);
    (void)add_cmd("logout", "Logout shell",      sh_logout);
#endif

	// Then add application/user commands
	(void) add_cmd("show", "Show all globals (gvars)", sh_show);
	(void) add_cmd("back", "Backlight 0..100%", sh_back);
	(void) add_cmd("dim", "Backlight control state", sh_dim);
	(void) add_cmd("dir", "FAT ", sh_dir);
	(void) add_cmd("type", "Show a file: type tft_2026.csv", sh_type);
	(void) add_cmd("del", "Delete a file: del name.txt", sh_del);
	(void) add_cmd("log", "Write a log line now", sh_log);

	(void) add_cmd("scan", "I2C ", sh_i2cscan);
	(void) add_cmd("lux", "BH1750 ", sh_lux);
	(void) add_cmd("range", "BH1750 ", sh_range);

	// EEPROM settings: without value show, with value set and save
	(void) add_cmd("backlow", "Backlight dark room 1..100%", sh_backlow);
	(void) add_cmd("backhigh", "Backlight bright room 1..100%", sh_backhigh);
	(void) add_cmd("luxdark", "Below this lux: dark", sh_luxdark);
	(void) add_cmd("luxbright", "Above this lux: bright", sh_luxbright);
	(void) add_cmd("holdshort", "Backlight hold after movement (s)", sh_holdshort);
	(void) add_cmd("holdlong", "Backlight hold 17:00..22:30 (s)", sh_holdlong);






}

