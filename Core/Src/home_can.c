/*
 * home_can.c
 *
 * Converts the received home CAN bus messages into gvars.home_data for the GUI.
 * The CAN driver (queues, FDCAN, ID < 100 network/shell) is the universal can_functions.c,
 * it calls can_rx_message() for every received data frame.
 *
 * Message layout: see Project_Gateway/inc/can_id.h
 */

#include "main.h"
#include "vars.h"
#include "rtc.h"
#include "can_functions.h"
#include "home_can.h"

#define BIG16(d)	((uint16_t) ((d)[1] | ((d)[0] << 8)))
#define LITTLE16(d)	((uint16_t) ((d)[0] | ((d)[1] << 8)))





/*
 * Start FDCAN1, call after MX_FDCAN1_Init()
 */
void home_can_start(void)
{
	HAL_PWR_EnableBkUpAccess();		// RTC backup registers for the day totals

	FDCAN_Config();
}

/*
 * Called by handle_can() for every received data frame
 */
void can_rx_message(const can_trx_t *msg)
{
	const uint8_t *d = msg->data;
	uint32_t len = msg->len;
	uint32_t now = home_millis();

	switch (msg->id)
	{
	case ID_300:
		if (len >= 8)
		{
			gvars.home_data.consume_w = BIG16(&d[0]);
			gvars.home_data.solar_w = BIG16(&d[2]);
			gvars.home_data.h2o = BIG16(&d[4]);
			gvars.home_data.ev_w = BIG16(&d[6]);
			gvars.home_data.energy_rx_ms = now;
		}
		break;

	case ID_320:
		if (len >= 6)
		{
			gvars.home_data.grid_w = (int16_t) LITTLE16(&d[0]);
			gvars.home_data.tariff = d[5];
			gvars.home_data.grid_rx_ms = now;
		}
		break;

	case ID_138:
		if (len >= 3)
		{
			if (d[0] == 1)
				gvars.home_data.battery_charge_w = (int16_t) LITTLE16(&d[1]);
			else if (d[0] == 2)
				gvars.home_data.battery_produce_w = (int16_t) LITTLE16(&d[1]);
			else
				break;
			gvars.home_data.battery_rx_ms = now;
		}
		break;

	case ID_139:
		if (len >= 3 && d[0] == 2 && d[1] == 0 && d[2] <= 100)
		{
			gvars.home_data.battery_soc = d[2];
			gvars.home_data.soc_rx_ms = now;
		}
		break;

	case ID_189:
		if (len >= 2)
		{
			gvars.home_data.pressure_mbar10 = BIG16(d);
			gvars.home_data.pressure_rx_ms = now;
		}
		break;

	case ID_190:
		if (len >= 2)
		{
			gvars.home_data.humidity = BIG16(d);
			gvars.home_data.humidity_rx_ms = now;
		}
		break;

	case ID_213:
		if (len >= 2)
		{
			gvars.home_data.outdoor_temp = (int16_t) BIG16(d);
			gvars.home_data.outdoor_temp_rx_ms = now;
		}
		break;

	case ID_243:
		if (len >= 2)
		{
			gvars.home_data.wind_hmh = (int16_t) BIG16(d);
			gvars.home_data.wind_rx_ms = now;
		}
		break;

	case ID_215:
		if (len >= 2)
		{
			gvars.home_data.rain_adc = BIG16(d);
			gvars.home_data.rain_rx_ms = now;
		}
		break;

	case ID_234:
		if (len >= 2)
		{
			uint16_t st = BIG16(d);

			// new charge session, same rule as the gateway (ev_charging.c). Checked on every message:
			// CAR_DETECTED can be shorter than the 1 second tick of the Model
			if (gvars.home_data.ev_state_rx_ms != 0 && gvars.home_data.ev_state == EV_STATE_CAR_DETECTED
					&& (st == EV_STATE_CHARGING || st == EV_STATE_SOLAR_WAIT))
				gvars.home_data.ev_session_cnt++;

			gvars.home_data.ev_state = st;
			gvars.home_data.ev_state_rx_ms = now;
		}
		break;

	case ID_214:
		if (len >= 4)
		{
			gvars.home_data.rain_count = (uint32_t) d[0] | (uint32_t) d[1] << 8 | (uint32_t) d[2] << 16 | (uint32_t) d[3] << 24;
			gvars.home_data.rain_count_rx_ms = now;
		}
		break;

	case ID_202:
		if (len >= 4)
		{
			gvars.home_data.epoch = (uint32_t) d[3] | (uint32_t) d[2] << 8 | (uint32_t) d[1] << 16 | (uint32_t) d[0] << 24;
			gvars.home_data.epoch_rx_ms = now;
		}
		break;

	case ID_404:
		if (len >= 1)
		{
			// PIR received on change
			gvars.home_data.pir = d[0];
			gvars.home_data.pir_rx_ms = now;
		}
		break;

	default:
		break;
	}
}
