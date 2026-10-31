/*
 * logging.c
 *
 *  Created on: 29 Oct 2026
 *      Author: marku
 *
 * Every hour one line with the values in tft_<year>.csv on the flash disk (same disk as the USB drive).
 * A new file starts with a header line. The time is the local time of the gateway (ID_202).
 *
 * Adding a value: add a line to columns[] (at the end, then older lines stay correct in Excel).
 * When the header of an existing file differs, the new header is added as a line, so the change is visible.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"
#include "app_filex.h"
#include "uart_sci.h"
#include "ms_timers.h"
#include "vars.h"
#include "fat_routines.h"
#include "logging.h"

static bool local_time(log_time_t *t);
static void make_header(char *buf, size_t size);
static bool write_log(const log_time_t *t);


/*
 * The columns of the log file, after the date. Add new values at the end.
 */
static const log_column_t columns[] =
{
	// no value columns yet, e.g. { "Name (unit)", col_function },
};

#define NR_COLUMNS	(sizeof(columns) / sizeof(columns[0]))

/*
 * Call from the main loop: every new hour one line. Not in the first (incomplete) hour after a start.
 */
void process_logging(void)
{
	static uint32_t sec_tick;
	static uint8_t old_hour = 0xFF;
	log_time_t t;

	if ((HAL_GetTick() - sec_tick) < 1000)
		return;
	sec_tick = HAL_GetTick();

	if (!local_time(&t))
		return;

	// first time
	if (old_hour == 0xFF)
	{
		old_hour = t.hour;
		return;
	}

	// new hour
	if (t.hour != old_hour)
	{
		old_hour = t.hour;
		write_log(&t);
	}
}

/*
 * Write a line now (shell command "log", for testing)
 */
void log_now(void)
{
	log_time_t t;

	if (!local_time(&t))
	{
		tty_printf("LOG: no time received (ID_202)\r\n");
		return;
	}
	write_log(&t);
}
/*
 * ***************
 */




/*
 * Local time from the gateway (ID_202), false when not (yet) received
 */
static bool local_time(log_time_t *t)
{
	const volatile home_data_t *d = &gvars.home_data;

	if (d->epoch_rx_ms == 0 || d->epoch < 1600000000u)
		return false;

	uint32_t epoch = d->epoch + (home_millis() - d->epoch_rx_ms) / 1000u;
	uint32_t secs = epoch % 86400u;

	// civil_from_days, http://howardhinnant.github.io/date_algorithms.html
	int32_t z = (int32_t) (epoch / 86400u) + 719468;
	int32_t era = z / 146097;
	uint32_t doe = (uint32_t) (z - era * 146097);
	uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	uint32_t mp = (5 * doy + 2) / 153;
	uint32_t month = mp < 10 ? mp + 3 : mp - 9;

	t->year = (uint16_t) (yoe + era * 400 + (month <= 2));
	t->month = (uint8_t) month;
	t->day = (uint8_t) (doy - (153 * mp + 2) / 5 + 1);
	t->hour = (uint8_t) (secs / 3600);
	t->minute = (uint8_t) (secs % 3600 / 60);
	t->second = (uint8_t) (secs % 60);
	return true;
}

/*
 * Header line, without line end
 */
static void make_header(char *buf, size_t size)
{
	size_t len;

	snprintf(buf, size, "Date");
	for (uint32_t i = 0; i < NR_COLUMNS; i++)
	{
		len = strlen(buf);
		snprintf(&buf[len], size - len, ",%s", columns[i].name);
	}
}

/*
 * Write one line with the actual values, makes the file (with header) when needed
 */
static bool write_log(const log_time_t *t)
{
	static char filename[20];
	static char line[LOG_LINE_SIZE];
	static char header[LOG_LINE_SIZE];
	static char first[LOG_LINE_SIZE];
	char value[16];
	size_t len;

	// date and time of the file in the directory
	fx_system_date_set(t->year, t->month, t->day);
	fx_system_time_set(t->hour, t->minute, t->second);

	snprintf(filename, sizeof(filename), "tft_%u.csv", t->year);

	make_header(header, sizeof(header));
	if (!file_first_line(filename, first, sizeof(first)))
	{
		// new file
		len = strlen(header);
		snprintf(&header[len], sizeof(header) - len, "\r\n");
		if (!file_add(filename, header))
			return false;
	}
	else if (strcmp(first, header) != 0)
	{
		// columns changed (new firmware): new header, older lines keep their columns
		tty_printf("LOG: header of %s changed, header added\r\n", filename);
		len = strlen(header);
		snprintf(&header[len], sizeof(header) - len, "\r\n");
		if (!file_add(filename, header))
			return false;
	}

	snprintf(line, sizeof(line), "%u-%u-%04u %02u:%02u", t->day, t->month, t->year, t->hour, t->minute);
	for (uint32_t i = 0; i < NR_COLUMNS; i++)
	{
		value[0] = 0;
		columns[i].value(value, sizeof(value));
		len = strlen(line);
		snprintf(&line[len], sizeof(line) - len, ",%s", value);
	}
	len = strlen(line);
	snprintf(&line[len], sizeof(line) - len, "\r\n");

	if (file_add(filename, line))
	{
		tty_printf("LOG: %s %s", filename, line);
		return true;
	}

	tty_printf("LOG: Err save %s %s", filename, line);
	return false;
}
