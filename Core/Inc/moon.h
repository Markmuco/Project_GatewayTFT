/*
 * moon.h
 *
 *  Created on: 14 Oct 2026
 *      Author: marku
 */

#ifndef INC_MOON_H_
#define INC_MOON_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOON_PHASES		8	// 0 new, 2 first quarter, 4 full, 6 last quarter

uint8_t moon_phase(uint32_t epoch);		// phase 0..7
uint8_t moon_age(uint32_t epoch);		// days since new moon 0..29

#ifdef __cplusplus
}
#endif

#endif /* INC_MOON_H_ */
