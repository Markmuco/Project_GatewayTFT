/*
 * logging.h
 *
 *  Created on: 29 Oct 2026
 *      Author: marku
 */

#ifndef INC_LOGGING_H_
#define INC_LOGGING_H_


#define LOG_LINE_SIZE	200


typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} log_time_t;

typedef struct
{
	const char *name;							// header text
	void (*value)(char *buf, size_t size);		// value as text, empty when not valid
} log_column_t;


void process_logging(void);	// every hour a line in tft_<year>.csv
void log_now(void);			// a line now (shell command "log")

#endif /* INC_LOGGING_H_ */
