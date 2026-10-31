/*
 * BH1750.h
 *
 *  Created on: 29 apr. 2016
 *      Author: Mark
 */

#include "main.h"
#include "time.h"
#include "BH1750.h"

static I2C_HandleTypeDef * i2c;

// Local functions
static HAL_StatusTypeDef BH1750_write(uint8_t cmd);
static HAL_StatusTypeDef BH1750_read_word(uint8_t * value);

/*!
 * \brief
 *
 * \param pointer to I2C handler
 *
 * \return - HAL_OK
 */
HAL_StatusTypeDef BH1750_init(I2C_HandleTypeDef * hi2c)
{
	i2c = hi2c;
    return BH1750_write(power_down);
}

/*!
 * \brief Non Blocking read function use read after x ms
 *
 * \param	mode:
			cont_H_res_mode2
			cont_L_res_mode
			one_time_H_res_mode1
			one_time_H_res_mode2
			one_time_L_res_mode
 *
 * \return - HAL_OK
 */
HAL_StatusTypeDef start_lux_conversion(uint8_t mode)
{
	HAL_StatusTypeDef res;
    if ((res = BH1750_write(power_up) != HAL_OK))
    	return res;

    if ((res = BH1750_write(mode)) != HAL_OK)
    	return res;

    return res;
}


/*!
 * \brief Non Blocking read function
 * \param	pointer to data
 *
 * \return - HAL_OK
 */
HAL_StatusTypeDef get_lux_value(uint16_t * lux_value)
{
	HAL_StatusTypeDef res;
	uint8_t data[2];

    if ((res = BH1750_read_word(data)) != HAL_OK)
    	return res;

    if (( res =BH1750_write(power_down)) != HAL_OK)
    	return res;

    *lux_value = data[0] <<8  | data[1];

    return res;
}

/*!
 * \brief Blocking read function
 *
 * \param	pointer to data
 * \param	mode:
			cont_H_res_mode2
			cont_L_res_mode
			one_time_H_res_mode1
			one_time_H_res_mode2
			one_time_L_res_mode
 * \param	Read delay (ms)
 *
 * \return - HAL_OK
 */
HAL_StatusTypeDef direct_lux_value(uint16_t * lux_value, uint8_t mode, uint16_t delay_time)
{
	HAL_StatusTypeDef res;
	uint8_t data[2];

    if ((res = BH1750_write(power_up) != HAL_OK))
    	return res;

    if ((res = BH1750_write(mode)) != HAL_OK)
    	return res;

    HAL_Delay(delay_time);

    if ((res != BH1750_read_word(data)) != HAL_OK)
    	return res;

    if (( res =BH1750_write(power_down)) != HAL_OK)
    	return res;

    *lux_value = data[0] <<8  | data[1];

    return res;
}

/*!
 * \brief Write to BH1750
 *
 * \param
 *
 * \return - HAL_OK
 */
static HAL_StatusTypeDef BH1750_write(uint8_t cmd)
{
    return HAL_I2C_Master_Transmit(i2c, (uint16_t) BH1750_ADDRESS, &cmd, 1, 100);
}

/*!
 * \brief Read BH1750 sensor
 *
 * \param
 *
 * \return - HAL_OK
 */
static HAL_StatusTypeDef BH1750_read_word(uint8_t * value)
{
    uint8_t data[2] = {0,0};
    HAL_StatusTypeDef res;

    if ((res = HAL_I2C_Master_Transmit(i2c, (uint16_t) BH1750_ADDRESS, data, 1, 100)) != HAL_OK)
		return res;

    if ((res =  HAL_I2C_Master_Receive(i2c, (uint16_t) BH1750_ADDRESS, value, 2, 100)) != HAL_OK)
		return res;

    return res;
}




