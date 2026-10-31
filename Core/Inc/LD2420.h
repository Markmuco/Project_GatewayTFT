/*
 * LD2420.h
 *
 *  Created on: 15 Oct 2026
 *      Author: marku
 */

#ifndef INC_LD2420_H_
#define INC_LD2420_H_

#include <stdint.h>
#include <stdbool.h>

#define LD2420_RANGE_OFF	0xFFFF	// no presence
#define LD2420_ERROR		0xFFFE	// no data

// LD2420 command words
#define LD2420_CMD_READ_VERSION		0x0000
#define LD2420_CMD_WRITE_SYS_PARAM	0x0012
#define LD2420_CMD_READ_SYS_PARAM	0x0013
#define LD2420_CMD_ENABLE_CONFIG	0x00FF
#define LD2420_CMD_END_CONFIG		0x00FE

// System parameter 0x0000 (work mode) values
#define LD2420_MODE_SIMPLE		0x00000064	// ON/OFF + Range text (default)
#define LD2420_MODE_ENERGY		0x00000004	// binary energy frames
#define LD2420_MODE_DEBUG		0x00000000

void process_ld2420_data(void);
uint16_t get_ld2420_range(void);

bool ld2420_send_cmd(uint16_t cmd, const uint8_t *value, uint16_t len);
bool ld2420_set_mode(uint32_t mode);
bool ld2420_ack_received(uint16_t *cmd, uint16_t *status);

#endif /* INC_LD2420_H_ */
