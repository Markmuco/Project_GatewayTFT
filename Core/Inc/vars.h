/*
 * vars.h
 *
 * Home TFT display, global variables in gvars.
 * Also used by the TouchGFX Model (C++) and the simulator: no HAL includes here.
 */

#ifndef VARS_H_
#define VARS_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


#define APP_KEY   (0xAA55AA55)
#define VERSION   (0x0101FFFF)
#define WAIT_KEY  (0x55AA55AA)

#define CAN_SERIAL  (0x20260013)	// unique serial of this node on the CAN bus 1 leverl
#define CAN_KEY		(0x112255AA)	// RAM_KEY value: jump to the CAN bootloader

// Shared with the bootloader, the linker script keeps 0x20000000..0x2000000F free
#define RAM_KEY   	(0x20000000)	// RAM address
#define RAM_SERIAL	(0x20000004)

#define SHOW_HUMIDITY		(0)

// Non volatile storage for the day totals (RTC backup registers)
#define HOME_BKP_MAGIC		0
#define HOME_BKP_DAY		1
#define HOME_BKP_CONSUME	2
#define HOME_BKP_SOLAR		3
#define HOME_BKP_EV			4
#define HOME_BKP_EVSES		5		// energy (Wh) of the EV charge session

// EV state, ID_234 (ev_state_t of Project_Gateway/inc/vars.h)
#define EV_STATE_CAR_DETECTED	2
#define EV_STATE_CHARGING		3
#define EV_STATE_SOLAR_WAIT		8
#define EV_STATE_SOLAR_CHARGING	9
#define EV_STATE_SOLAR_SHTDN	10



#define BACK_LOW 			gvars.eeprom.back_low
#define BACK_HIGH 			gvars.eeprom.back_high
#define LUX_DARK 			gvars.eeprom.lux_dark
#define LUX_BRIGHT 			gvars.eeprom.lux_bright

#define BACK_SHORT_SEC		gvars.eeprom.back_short_sec
#define BACK_LONG_SEC		gvars.eeprom.back_long_sec

/* Swap bytes in 16-bit value.  */
#define __bswap_16(x)				((__uint16_t) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))


// Application info
typedef struct s_app_info
{
  uint32_t key;
  uint32_t version;
  uint32_t crc32;
  uint32_t size;
  uint32_t can_serial;
  uint8_t build_date[sizeof(__DATE__)];
  uint8_t build_time[sizeof(__TIME__)];
  uint8_t dummy[3];
} app_info_t;

typedef struct
{
	uint32_t key;
	uint32_t version;
	uint32_t crc32;
	uint32_t size;
	uint8_t build_date[sizeof(__DATE__)];
	uint8_t build_time[sizeof(__TIME__)];
} bl_info_t;

/*
 * Values received from the CAN bus (home_can.c), see Project_Gateway/inc/can_id.h.
 * Every group has a rx_ms timestamp (home_millis() at reception), 0 = never received.
 */
typedef struct
{
	// ID_300, send by gateway every 4 seconds
	volatile int32_t consume_w;			// total house consumption (W), including EV
	volatile int32_t solar_w;			// solar production (W)
	volatile int32_t h2o;				// water (L/hour)
	volatile int32_t ev_w;				// EV charge power (W)
	volatile uint32_t energy_rx_ms;

	// ID_320, DSM smart meter
	volatile int16_t grid_w;			// + = import from grid, - = export to grid
	volatile uint8_t tariff;			// 1: low 2: normal
	volatile uint32_t grid_rx_ms;

	// ID_138 / ID_139, Powerwall
	volatile uint8_t battery_soc;		// %
	volatile uint32_t soc_rx_ms;
	volatile int16_t battery_charge_w;	// charger power (W)
	volatile int16_t battery_produce_w;	// inverter power (W)
	volatile uint32_t battery_rx_ms;

	// Garden sensors
	volatile uint16_t pressure_mbar10;	// ID_189 0.1 mbar (like Project_Gateway: pressure / 10 = mbar)
	volatile uint32_t pressure_rx_ms;
	volatile uint16_t humidity;			// ID_190 %
	volatile uint32_t humidity_rx_ms;
	volatile int16_t outdoor_temp;		// ID_213 0.1 C
	volatile uint32_t outdoor_temp_rx_ms;
	volatile int16_t wind_hmh;			// ID_243 peak wind hectometer/h (like Project_Gateway)
	volatile uint32_t wind_rx_ms;
	volatile uint16_t rain_adc;			// ID_215 rain sensor conductivity: > 700 dry, < 100 rain
	volatile uint32_t rain_rx_ms;
	volatile uint32_t rain_count;		// ID_214 total of rain pulses, 0.3 mm per pulse (little endian uint32)
	volatile uint32_t rain_count_rx_ms;

	// ID_234, EV state, sent on change
	volatile uint16_t ev_state;
	volatile uint32_t ev_state_rx_ms;
	volatile uint16_t ev_session_cnt;	// +1 at a new charge session: CAR_DETECTED -> CHARGING / SOLAR_WAIT (home_can.c)

	// ID_404, PIR (sent on change)
	volatile uint8_t pir;				// 0: no movement, else movement
	volatile uint32_t pir_rx_ms;

	// ID_202, local time
	volatile uint32_t epoch;
	volatile uint32_t epoch_rx_ms;

} home_data_t;


typedef struct
{
	uint16_t back_low;		// pwm
	uint16_t back_high; 	//
	uint16_t lux_dark; 		// below this value the display dims
	uint16_t lux_bright; 	// above this value the display brightens
	uint16_t back_short_sec;	// seconds to hold the backlight after movement
	uint16_t back_long_sec;	// seconds to hold the backlight after movement
	uint32_t uptime_sec;	// total operating time, counted in dimmer.c, stored once an hour
	uint32_t backlight_sec;	// total time the backlight was on, counted in dimmer.c, stored once an hour
} eeprom_t;

typedef struct
{
	home_data_t home_data;
	eeprom_t eeprom;

	bool mx25l_ok;						// MX25L3233F flash found at startup (USB mass storage)

} gvars_t;

extern gvars_t gvars;

uint32_t home_millis(void);
void home_backup_write(uint32_t index, uint32_t value);
uint32_t home_backup_read(uint32_t index);
void init_vars(void);

#ifdef __cplusplus
}
#endif

#endif /* VARS_H_ */
