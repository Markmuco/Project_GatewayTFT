/*
 * LD2420.c
 *
 *  Created on: 15 Oct 2026
 *      Author: marku
 *
 * Simple output mode of the LD2420 (ASCII lines):
 *   "ON" / "OFF" and "Range <cm>"
 * Commands use the binary frame:
 *   FD FC FB FA | len(2,LE) | cmd(2,LE) | value | 04 03 02 01
 * The ack of a command (cmd | 0x0100) is received in the same stream.
 */
#include "main.h"
#include "uart_sci.h"
#include "LD2420.h"


#define LINE_MAX_LEN	16
#define ACK_MAX_LEN		32

static uint16_t range = LD2420_ERROR;	// no data yet

#define RX_TIMEOUT_MS	2000	// no data from the sensor for this time: LD2420_ERROR

static const uint8_t hdr[4] = { 0xFD, 0xFC, 0xFB, 0xFA };
static const uint8_t ftr[4] = { 0x04, 0x03, 0x02, 0x01 };

// last command ack (shared by ld2420_send_cmd and ld2420_ack_received)
static bool ack_valid;
static uint16_t ack_cmd, ack_status;

/*
 * Call regularly from the main loop: ASCII lines (ON / OFF / Range n) and binary command acks
 */
void process_ld2420_data(void)
{
	// ASCII line state
	static char line[LINE_MAX_LEN];
	static uint8_t line_len;
	// binary ack state
	static uint8_t hdr_idx;			// matched header bytes
	static bool in_frame;
	static uint8_t frame[ACK_MAX_LEN];
	static uint16_t frame_len;		// bytes in frame buffer (after header)
	static uint16_t frame_expect;	// total expected: 2 + len + 4
	static uint32_t last_rx_ms;		// time of the last received byte

	char ch;

	while (sci2_getch(&ch))
	{
		last_rx_ms = HAL_GetTick();
		if (in_frame)
		{
			frame[frame_len++] = ch;
			if (frame_len == 2)
			{
				uint16_t len = frame[0] | (frame[1] << 8);
				if (len + 6U > ACK_MAX_LEN)
					in_frame = false;	// too long, drop
				else
					frame_expect = len + 6U;
			}
			else if (frame_len > 2 && frame_len == frame_expect)
			{
				// complete frame: len(2) data... footer(4)
				uint16_t len = frame[0] | (frame[1] << 8);

				if (len >= 4 && memcmp(&frame[2 + len], ftr, 4) == 0)
				{
					ack_cmd = frame[2] | (frame[3] << 8);
					ack_status = frame[4] | (frame[5] << 8);
					ack_valid = true;
				}
				in_frame = false;
			}
			continue;
		}

		// look for frame header
		if ((uint8_t) ch == hdr[hdr_idx])
		{
			if (++hdr_idx == sizeof(hdr))
			{
				hdr_idx = 0;
				in_frame = true;
				frame_len = 0;
				frame_expect = 0;
				line_len = 0;
			}
			continue;
		}
		hdr_idx = ((uint8_t) ch == hdr[0]) ? 1 : 0;

		// ASCII text
		if (ch == 13 || ch == 10)	// CR / LF
		{
			if (line_len)
			{
				line[line_len] = 0;

				if (strcmp(line, "OFF") == 0)
					range = LD2420_RANGE_OFF;
				else if (strncmp(line, "Range ", 6) == 0)
				{
					uint32_t v = 0;
					for (const char *p = &line[6]; *p >= '0' && *p <= '9'; p++)
						v = v * 10 + (*p - '0');
					range = (v > 0xFFFD) ? 0xFFFD : (uint16_t) v;	// 0xFFFE/0xFFFF are reserved
				}
				// "ON": presence, the range follows in the next line
			}
			line_len = 0;
		}
		else if (ch >= 0x20 && ch < 0x7F)
		{
			if (line_len < LINE_MAX_LEN - 1)
				line[line_len++] = ch;
			else
				line_len = 0;	// garbage
		}
		else
			line_len = 0;
	}

	// sensor silent
	if ((HAL_GetTick() - last_rx_ms) > RX_TIMEOUT_MS)
		range = LD2420_ERROR;
}

/*
 * Range in cm, LD2420_RANGE_OFF (0xFFFF) when no presence, LD2420_ERROR (0xFFFE) when no data
 */
uint16_t get_ld2420_range(void)
{
	return range;
}

/*
 * Send a command frame to the sensor.
 * value: command value bytes (may be NULL when len = 0)
 */
bool ld2420_send_cmd(uint16_t cmd, const uint8_t *value, uint16_t len)
{
	uint8_t buf[32];
	uint16_t n = 0;
	uint16_t dlen = 2 + len;

	if (len > 16)
		return false;

	memcpy(&buf[n], hdr, 4);
	n += 4;
	buf[n++] = dlen & 0xFF;
	buf[n++] = dlen >> 8;
	buf[n++] = cmd & 0xFF;
	buf[n++] = cmd >> 8;
	if (len)
	{
		memcpy(&buf[n], value, len);
		n += len;
	}
	memcpy(&buf[n], ftr, 4);
	n += 4;

	ack_valid = false;
	return sci2_putsn((char*) buf, n);
}

/*
 * Set the work mode (system parameter 0): enable config, write param, end config.
 * mode: LD2420_MODE_SIMPLE (0x64)  ON/OFF + "Range n" text output (default)
 *       LD2420_MODE_ENERGY (0x04)  binary frames with energy per gate
 *       LD2420_MODE_DEBUG  (0x00)  debug output
 * The sensor needs a moment between the commands, so this blocks ~3 x 50 ms.
 */
bool ld2420_set_mode(uint32_t mode)
{
	const uint8_t en[2] =
	{ 0x01, 0x00 };	// protocol version
	uint8_t par[6] =
	{ 0x00, 0x00, mode, mode >> 8, mode >> 16, mode >> 24 };
	bool ok;

	ok = ld2420_send_cmd(LD2420_CMD_ENABLE_CONFIG, en, sizeof(en));
	HAL_Delay(50);
	process_ld2420_data();
	ok &= ld2420_send_cmd(LD2420_CMD_WRITE_SYS_PARAM, par, sizeof(par));
	HAL_Delay(50);
	process_ld2420_data();
	ok &= ld2420_send_cmd(LD2420_CMD_END_CONFIG, NULL, 0);
	HAL_Delay(50);
	process_ld2420_data();

	return ok;
}

/*
 * Last received command ack (cmd = sent command | 0x0100, status 0 = OK)
 */
bool ld2420_ack_received(uint16_t *cmd, uint16_t *status)
{
	if (!ack_valid)
		return false;
	if (cmd)
		*cmd = ack_cmd;
	if (status)
		*status = ack_status;
	ack_valid = false;
	return true;
}
