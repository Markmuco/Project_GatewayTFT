/*
 * moon.c
 *
 *  Created on: 14 Oct 2026
 *      Author: marku
 *
 * Phase of the moon from the epoch time, integer only.
 * Reference new moon: 6 jan 2000 18:14 UTC, mean synodic month 29.530589 days.
 * The epoch on the CAN bus is local time, the error of 1-2 hours does not matter here.
 */

#include "moon.h"

#define NEW_MOON_EPOCH		947182440u	// 2000-01-06 18:14 UTC
#define SYNODIC_SECONDS		2551443u	// 29.530589 days

static uint32_t moon_seconds(uint32_t epoch)
{
	return (epoch - NEW_MOON_EPOCH) % SYNODIC_SECONDS;
}

/*
 * Phase in 8 steps, rounded to the nearest step:
 * 0 new, 1 waxing crescent, 2 first quarter, 3 waxing gibbous,
 * 4 full, 5 waning gibbous, 6 last quarter, 7 waning crescent
 */
uint8_t moon_phase(uint32_t epoch)
{
	uint32_t s = moon_seconds(epoch);

	return (uint8_t) (((uint64_t) s * MOON_PHASES + SYNODIC_SECONDS / 2) / SYNODIC_SECONDS % MOON_PHASES);
}

/*
 * Days since new moon 0..29
 */
uint8_t moon_age(uint32_t epoch)
{
	return (uint8_t) (moon_seconds(epoch) / 86400u);
}
