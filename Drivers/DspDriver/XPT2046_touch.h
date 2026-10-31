/*
 * XPT2046_touch.h
 *
 *  Created on: 20 sep. 2019.
 *      Author: Andriy Honcharenko
 */

#ifndef XPT2046_TOUCH_H
#define XPT2046_TOUCH_H

#include "main.h"
#include <stdbool.h>


// Warning! Use SPI bus with < 2.5 Mbit speed, better ~650 Kbit to be save.
#define XPT2046_SPI_PORT hspi1
extern SPI_HandleTypeDef XPT2046_SPI_PORT;


#define XPT2046_IRQ_Pin       	T_IRQ_Pin
#define XPT2046_IRQ_GPIO_Port 	T_IRQ_GPIO_Port
#define XPT2046_CS_Pin        	TC_CS_Pin
#define XPT2046_CS_GPIO_Port  	TC_CS_GPIO_Port

#define TOUCH_ORIENTATION_PORTRAIT 			(0U)
#define TOUCH_ORIENTATION_LANDSCAPE 		(1U)
#define TOUCH_ORIENTATION_PORTRAIT_MIRROR 	(2U)
#define TOUCH_ORIENTATION_LANDSCAPE_MIRROR 	(3U)

#define ORIENTATION	(TOUCH_ORIENTATION_PORTRAIT)

// change depending on screen orientation
#if (ORIENTATION == 0)
#define XPT2046_SCALE_X 800
#define XPT2046_SCALE_Y 480
#elif (ORIENTATION == 1)
#define XPT2046_SCALE_X 800
#define XPT2046_SCALE_Y 480
#elif (ORIENTATION == 2)
#define XPT2046_SCALE_X 800
#define XPT2046_SCALE_Y 480
#elif (ORIENTATION == 3)
#define XPT2046_SCALE_X 800
#define XPT2046_SCALE_Y 480
#endif

// to calibrate uncomment UART_Printf line in ili9341_touch.c
#define XPT2046_MIN_RAW_X 37588
#define XPT2046_MAX_RAW_X 58700
#define XPT2046_MIN_RAW_Y 41200
#define XPT2046_MAX_RAW_Y 59500

#ifdef __cplusplus
 extern "C" {
#endif

// call before initializing any SPI devices
extern void XPT2046_TouchInit(void);
extern bool XPT2046_TouchPressed(void);
extern bool XPT2046_TouchGetCoordinates(int32_t* x, int32_t* y);

#ifdef __cplusplus
}
#endif

#endif /* XPT2046_TOUCH_H_ */
