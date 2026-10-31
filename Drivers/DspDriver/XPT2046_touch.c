/*
 * XPT2046_touch.c
 *
 *  Created on: 20 sep. 2019.
 *      Author: Andriy Honcharenko
 */

#include <stdio.h>
#include <stdlib.h>
#include <XPT2046_touch.h>

#include "uart_sci.h"


#if (ORIENTATION == 0)
#define READ_X 0xD0
#define READ_Y 0x90
#elif (ORIENTATION == 1)
#define READ_Y 0xD0
#define READ_X 0x90
#elif (ORIENTATION == 2)
#define READ_X 0xD0
#define READ_Y 0x90
#elif (ORIENTATION == 3)
#define READ_Y 0xD0
#define READ_X 0x90
#endif



static void XPT2046_TouchSelect()
{
    HAL_GPIO_WritePin(XPT2046_CS_GPIO_Port, XPT2046_CS_Pin, GPIO_PIN_RESET);
}

static void XPT2046_TouchUnselect()
{
    HAL_GPIO_WritePin(XPT2046_CS_GPIO_Port, XPT2046_CS_Pin, GPIO_PIN_SET);
}

/*
 * One dummy conversion with PD1/PD0 = 00: afterwards the chip is in power-down between
 * conversions with PENIRQ enabled. Call once after MX_SPI1_Init().
 */
void XPT2046_TouchInit(void)
{
    static const uint8_t cmd_read_y[] = { READ_Y };
    static const uint8_t zeroes_tx[] = { 0x00, 0x00 };
    uint8_t dummy[2];

    XPT2046_TouchSelect();
    HAL_SPI_Transmit(&XPT2046_SPI_PORT, (uint8_t*)cmd_read_y, sizeof(cmd_read_y), 100);
    HAL_SPI_TransmitReceive(&XPT2046_SPI_PORT, (uint8_t*)zeroes_tx, dummy, sizeof(dummy), 100);
    XPT2046_TouchUnselect();
}

bool XPT2046_TouchPressed(void)
{
    return HAL_GPIO_ReadPin(XPT2046_IRQ_GPIO_Port, XPT2046_IRQ_Pin) == GPIO_PIN_RESET;
}

bool XPT2046_TouchGetCoordinates(int32_t *x, int32_t *y)

{
	static const uint8_t cmd_read_x[] = { READ_X };
    static const uint8_t cmd_read_y[] = { READ_Y };
    static const uint8_t zeroes_tx[] = { 0x00, 0x00 };

    XPT2046_TouchSelect();

    uint32_t avg_x = 0;
    uint32_t avg_y = 0;
    uint8_t nsamples = 0;

    for(uint8_t i = 0; i < 16; i++)
    {
        if(!XPT2046_TouchPressed())
            break;

        nsamples++;

        uint8_t y_raw[2];
        uint8_t x_raw[2];

        HAL_SPI_Transmit(&XPT2046_SPI_PORT, (uint8_t*)cmd_read_y, sizeof(cmd_read_y), HAL_MAX_DELAY);
        HAL_SPI_TransmitReceive(&XPT2046_SPI_PORT, (uint8_t*)zeroes_tx, y_raw, sizeof(y_raw), HAL_MAX_DELAY);

        HAL_SPI_Transmit(&XPT2046_SPI_PORT, (uint8_t*)cmd_read_x, sizeof(cmd_read_x), HAL_MAX_DELAY);
        HAL_SPI_TransmitReceive(&XPT2046_SPI_PORT, (uint8_t*)zeroes_tx, x_raw, sizeof(x_raw), HAL_MAX_DELAY);

        avg_x += (((uint16_t)x_raw[0]) << 8) | ((uint16_t)x_raw[1]);
        avg_y += (((uint16_t)y_raw[0]) << 8) | ((uint16_t)y_raw[1]);
    }

    XPT2046_TouchUnselect();

    if(nsamples < 16)
        return false;

//    // Uncomment this line to calibrate touchscreen:
//    tty_printf("avg_x = %6d, avg_y = %6d\r\n", (int) avg_x /16, (int) avg_y/16);
//    tty_printf("\x1b[1F");

    uint32_t raw_x = (avg_x / 16);
    if(raw_x < XPT2046_MIN_RAW_X) raw_x = XPT2046_MIN_RAW_X;
    if(raw_x > XPT2046_MAX_RAW_X) raw_x = XPT2046_MAX_RAW_X;

    uint32_t raw_y = (avg_y / 16);
    if(raw_y < XPT2046_MIN_RAW_Y) raw_y = XPT2046_MIN_RAW_Y;
    if(raw_y > XPT2046_MAX_RAW_Y) raw_y = XPT2046_MAX_RAW_Y;

//    // Uncomment this line to calibrate touchscreen:
//    tty_printf("raw_x = %6d, raw_y = %6d\r\n", (int) raw_x, (int) raw_y);
//    tty_printf("\x1b[1F");

#if (ORIENTATION == 0)
	*x = (raw_x - XPT2046_MIN_RAW_X) * XPT2046_SCALE_X / (XPT2046_MAX_RAW_X - XPT2046_MIN_RAW_X);
	*y = (raw_y - XPT2046_MIN_RAW_Y) * XPT2046_SCALE_Y / (XPT2046_MAX_RAW_Y - XPT2046_MIN_RAW_Y);
#elif (ORIENTATION == 1)
	*x = (raw_x - XPT2046_MIN_RAW_X) * XPT2046_SCALE_X / (XPT2046_MAX_RAW_X - XPT2046_MIN_RAW_X);
	*y = (raw_y - XPT2046_MIN_RAW_Y) * XPT2046_SCALE_Y / (XPT2046_MAX_RAW_Y - XPT2046_MIN_RAW_Y);
#elif (ORIENTATION == 2)
    *x = (raw_x - XPT2046_MIN_RAW_X) * XPT2046_SCALE_X / (XPT2046_MAX_RAW_X - XPT2046_MIN_RAW_X);
    *y = XPT2046_SCALE_Y - (raw_y - XPT2046_MIN_RAW_Y) * XPT2046_SCALE_Y / (XPT2046_MAX_RAW_Y - XPT2046_MIN_RAW_Y);
#elif (ORIENTATION == 3)
    *x = XPT2046_SCALE_X - (raw_x - XPT2046_MIN_RAW_X) * XPT2046_SCALE_X / (XPT2046_MAX_RAW_X - XPT2046_MIN_RAW_X);
    *y = (raw_y - XPT2046_MIN_RAW_Y) * XPT2046_SCALE_Y / (XPT2046_MAX_RAW_Y - XPT2046_MIN_RAW_Y);
#endif

    // Uncomment this line to calibrate touchscreen:
    tty_printf("x = %6d, y = %6d\r\n", (int) *x, (int) *y);
    tty_printf("\x1b[1F");

    return true;
}
