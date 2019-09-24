/*
 * TCA9535.c
 *
 *  Created on: 05.10.2018
 *      Author: valtin
 */

#include "TCA9535.h"

bool tca9535_init_device(tca_device_t *device, I2C_HandleTypeDef *hi2c, uint8_t addressNumber, uint32_t isInput, uint32_t invertInput)
{
	device->valid = false;
	device->hi2c = hi2c;
	if ((addressNumber == 0) || (addressNumber > TCA9535_NUMBER_OF_I2C_ADDRESSES)){
		// address invalid
		return false;
	} else {
		addressNumber--;
		device->address = TCA9535_ADDRESS_FIXED_PART | (addressNumber <<1);
	}

	uint8_t data[TCA9535_BUFFER_SIZE_MAX];

	// set the input configuration
	data[0] = TCA9535_REGISTER_CONFIG_0; // start register
	// input data |xxxxxxxx|xxxxxxxx|io15, io14, ... io8|io7 ... io0|
	data[1] = (uint8_t)(isInput & 0xFF);
	data[2] = (uint8_t)((isInput >>8) & 0xFF);
	if (HAL_I2C_Master_Transmit(device->hi2c, (device->address |TCA9535_ADDRESS_WRITE_BIT), data, 3, TCA9535_DEFAULT_TIMEOUT_MS) != HAL_OK){
		return false;
	}

	// set the polarity
	data[0] = TCA9535_REGISTER_INVERSION_0; // start register
	// input data |xxxxxxxx|xxxxxxxx|io15, io14, ... io8|io7 ... io0|
	data[1] = (uint8_t)(invertInput & 0xFF);
	data[2] = (uint8_t)((invertInput >>8) & 0xFF);
	if (HAL_I2C_Master_Transmit(device->hi2c, (device->address |TCA9535_ADDRESS_WRITE_BIT), data, 3, TCA9535_DEFAULT_TIMEOUT_MS) != HAL_OK){
		return false;
	}

	return true;
}

bool tca9535_read_inputs(tca_device_t *device, uint32_t *input)
{
	uint8_t data[TCA9535_BUFFER_SIZE_MAX];

	// read the inputs
	data[0] = TCA9535_REGISTER_INPUT_0; // start register
	if (HAL_I2C_Master_Transmit(device->hi2c, (device->address |TCA9535_ADDRESS_WRITE_BIT), data, 1, TCA9535_DEFAULT_TIMEOUT_MS) != HAL_OK){
		return false;
	}
	if (HAL_I2C_Master_Receive(device->hi2c, (device->address |TCA9535_ADDRESS_READ_BIT), data, 2, TCA9535_DEFAULT_TIMEOUT_MS) != HAL_OK){
		return false;
	}
	*input = data[0] | (data[1] <<8);

	return true;
	// TODO
	//HAL_I2C_Master_Sequential_Transmit_IT(hi2c, DevAddress, pData, Size, XferOptions)
	// input data |xxxxxxxx|xxxxxxxx|io15, io14, ... io8|io7 ... io0|
}

bool tca9535_set_outputs(tca_device_t *device, uint32_t output)
{
	uint8_t data[TCA9535_BUFFER_SIZE_MAX];

	// set the outpurs
	data[0] = TCA9535_REGISTER_OUTPUT_0; // start register
	data[1] = (uint8_t)(output & 0xFF);
	data[2] = (uint8_t)((output >>8) & 0xFF);
	if (HAL_I2C_Master_Transmit(device->hi2c, (device->address |TCA9535_ADDRESS_WRITE_BIT), data, 1, TCA9535_DEFAULT_TIMEOUT_MS) != HAL_OK){
		return false;
	}

	return true;
}
