/*
 * BH1750.h
 *
 *  Created on: 29 apr. 2016
 *      Author: Mark
 */
//#include "stm32f1xx_hal.h"

#ifndef INC_BH1750_H_
#define INC_BH1750_H_



#endif /* INC_BH1750_H_ */

#define  BH1750_ADDRESS 0x46


#define  power_down 0x00
#define  power_up 0x01
#define  reset 0x07
#define  cont_H_res_mode1 0x10
#define  cont_H_res_mode2 0x11
#define  cont_L_res_mode 0x13
#define  one_time_H_res_mode1 0x20
#define  one_time_H_res_mode2 0x21
#define  one_time_L_res_mode 0x23


HAL_StatusTypeDef BH1750_init(I2C_HandleTypeDef * hi2c);
HAL_StatusTypeDef start_lux_conversion(uint8_t mode);
HAL_StatusTypeDef get_lux_value(uint16_t * lux_value);
HAL_StatusTypeDef direct_lux_value(uint16_t * lux_value, uint8_t mode, uint16_t delay_time);
