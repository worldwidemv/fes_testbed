/*
 * TCA9535.h
 *
 *  Created on: 05.10.2018
 *      Author: valtin
 */

#ifndef TCA9535_H_
#define TCA9535_H_

#include "stdint.h"
#include "stdbool.h"
#include "main.h"
#include "i2c.h"

#define TCA9535_NUMBER_OF_I2C_ADDRESSES	8
#define TCA9535_BUFFER_SIZE_MAX			5
#define TCA9535_DEFAULT_TIMEOUT_MS		3

#define TCA9535_REGISTER_INPUT_0		0x00
#define TCA9535_REGISTER_INPUT_1		0x01
#define TCA9535_REGISTER_OUTPUT_0		0x02
#define TCA9535_REGISTER_OUTPUT_1		0x03
#define TCA9535_REGISTER_INVERSION_0	0x04
#define TCA9535_REGISTER_INVERSION_1	0x05
#define TCA9535_REGISTER_CONFIG_0		0x06
#define TCA9535_REGISTER_CONFIG_1		0x07

#define TCA9535_ADDRESS_FIXED_PART		0x40
#define TCA9535_ADDRESS_READ_BIT		0x01
#define TCA9535_ADDRESS_WRITE_BIT		0x00

typedef struct {
	bool 				valid;
	I2C_HandleTypeDef 	*hi2c;
	uint16_t			address;
} tca_device_t;

bool tca9535_init_device(tca_device_t *device, I2C_HandleTypeDef *hi2c, uint8_t addressNumber, uint32_t isInput, uint32_t invertInput);
bool tca9535_read_inputs(tca_device_t *device, uint32_t *input);
bool tca9535_set_outputs(tca_device_t *device, uint32_t output);

#endif /* TCA9535_H_ */
