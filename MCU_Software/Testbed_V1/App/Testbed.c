/*
 * Testbed.c
 *
 *  Created on: 17.10.2018
 *      Author: valtin
 */

#include "Testbed.h"

#include "spi.h"
#include "tim.h"
#include "usb_device.h"

testbed_t testbed;


void testbed_init(testbed_initCmd_t *config, testbed_initAck_t *result)
{
	memset(&testbed, 0, sizeof(testbed_t));
	memcpy(&testbed.config, config, sizeof(testbed_initCmd_t));

	// ensure sensible fallbacks
	if (testbed.config.searchWidth == 0){
		testbed.config.searchWidth = MAX1119X_SEARCH_WIDTH;
	}
	if (testbed.config.samplePeriodeMultiplier == 0){
		testbed.config.samplePeriodeMultiplier = 1;
	}

	if (testbed.config.thresholdCh1Positiv  == 0){
		testbed.config.thresholdCh1Positiv  = MAX1119X_SEARCH_THRESHOLD_DEFAULT_CH1_POSITIV;
	}
	if (testbed.config.thresholdCh1Negative == 0){
		testbed.config.thresholdCh1Negative = MAX1119X_SEARCH_THRESHOLD_DEFAULT_CH1_NEGATIVE;
	}
	if (testbed.config.thresholdCh2Positiv  == 0){
		testbed.config.thresholdCh2Positiv  = MAX1119X_SEARCH_THRESHOLD_DEFAULT_CH2_POSITIV;
	}
	if (testbed.config.thresholdCh2Negative == 0){
		testbed.config.thresholdCh2Negative = MAX1119X_SEARCH_THRESHOLD_DEFAULT_CH2_NEGATIVE;
	}

	testbed.config.demuxEnabled = 0;

	// do stuff, depending on the state
	switch (testbed.config.dataType){
	case tb_stimDataOnly:
		break;
	case tb_constStream:
		break;
	case tb_varStream:

		break;
	default:
		// error unknown data type
		// TODO
		break;
	}

	// initialise the ADC
	result->successful = (uint8_t) max1119x_initDevice(&testbed.maxAdcDevice, (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData,
			&htim8, TIM8, TIM_CHANNEL_3, TIM_CHANNEL_1, TIM_DMA_CC1, TIM_DMA_ID_CC1, testbed.config.samplePeriodeMultiplier,
			&hspi1, &hspi1, &hspi4,
			testbed.config.searchWidth, testbed.config.thresholdCh1Positiv, testbed.config.thresholdCh1Negative, testbed.config.thresholdCh2Positiv, testbed.config.thresholdCh2Negative);

	result->sampleFrequency = MAX1119X_BASE_SAMPLING_FREQUENCY / testbed.config.samplePeriodeMultiplier;
	result->sampleBufferSize = MAX1119X_RECEIVE_BUFFER_SIZE_HALF;
}





bool testbed_sendAdcData_realData( __IO maxAdc_device_t *device )
{
	if (!device->valid || !device->measurementStarted){
		return false;
	}
	if (device->hcdc->TxState != 0){
		return false;
	}

	// USB device is ready to send data
	if (device->externalSendBuffer[device->packageNumberToSendNext].readyToBeSend){
		// update the package header
		device->externalSendBuffer[device->packageNumberToSendNext].package.GSBP_Header[0] = GSBP__PACKAGE_START_BYTE;
		device->externalSendBuffer[device->packageNumberToSendNext].package.GSBP_Header[1] = MeasurmentDataACK;
		device->externalSendBuffer[device->packageNumberToSendNext].package.GSBP_Header[2] = GSBP_GetPackageNumber(&GSBP_USB, true);
		uint16_t GSBP_HeaderNumberOfBytes = MAX1119X_PACKAGE_OFFSET + // TODO: active channels ans Ende Packen...
				(device->externalSendBuffer[device->packageNumberToSendNext].package.numberOfSamples * sizeof(maxAdc_dataBuffer_t));
		// add channel info, if the STIM was active
		if (device->externalSendBuffer[device->packageNumberToSendNext].stimWasActive){
			if (testbed.config.demuxEnabled){
				// TODO
			} else {
				// add the channel info for the normal stimulation channels to the end of the normal data
				*(&device->externalSendBuffer[device->packageNumberToSendNext].package.GSBP_Header[5] +
								GSBP_HeaderNumberOfBytes +1) = (uint8_t)((device->externalSendBuffer[device->packageNumberToSendNext].stimChannelDetect_I_PN >> 0) & 0xFF);
				*(&device->externalSendBuffer[device->packageNumberToSendNext].package.GSBP_Header[5] +
								GSBP_HeaderNumberOfBytes +2) = (uint8_t)((device->externalSendBuffer[device->packageNumberToSendNext].stimChannelDetect_I_PN >> 8) & 0xFF);
				GSBP_HeaderNumberOfBytes += 2;
			}
		}
		device->externalSendBuffer[device->packageNumberToSendNext].package.GSBP_Header[3] = (uint8_t)((GSBP_HeaderNumberOfBytes >> 8) & 0xFF);
		device->externalSendBuffer[device->packageNumberToSendNext].package.GSBP_Header[4] = (uint8_t)((GSBP_HeaderNumberOfBytes >> 0) & 0xFF);
		device->externalSendBuffer[device->packageNumberToSendNext].package.GSBP_Header[5] =
				GSBP_GetHeaderChecksum((uint8_t *)&device->externalSendBuffer[device->packageNumberToSendNext].package.GSBP_Header[1]);

		// update the package footer
		*(&device->externalSendBuffer[device->packageNumberToSendNext].package.GSBP_Header[5] +
				GSBP_HeaderNumberOfBytes +1) = 0x00;
		*(&device->externalSendBuffer[device->packageNumberToSendNext].package.GSBP_Header[5] +
				GSBP_HeaderNumberOfBytes +2) = GSBP__PACKAGE_END_BYTE;


		// send the data
		CDC_Transmit_FS( (uint8_t *)&device->externalSendBuffer[device->packageNumberToSendNext].package, (uint16_t)(GSBP_HeaderNumberOfBytes +TESTBED_GSBP_PACKAGE_OVERHEAD_SIZE));
		//HAL_UART_Transmit_DMA(GSBP_UART_Debug.UART_Handle, (uint8_t *) MeasurmentDataPackage, (uint16_t) (temp.data+8) );

		// wait until the data is send
		uint8_t NextToFill = device->packageNumberToFillNext;
		uint8_t NextToFill2 = NextToFill -1;
		NextToFill2 &= MAX1119X_NUMBER_OF_PACKAGE_CONTAINER_MASK;
		if ((NextToFill == device->packageNumberToFillNext) || (NextToFill2 == device->packageNumberToSendNext)) {
			uint64_t counter = 1;
			while (device->hcdc->TxState != 0){
				if (counter == 0){
					break;
				}
				counter++;
			}
		}
		// resets / give the data free
		device->externalSendBuffer[device->packageNumberToSendNext].readyToBeSend = false;
		device->externalSendBuffer[device->packageNumberToSendNext].stimWasActive = false;
		device->externalSendBuffer[device->packageNumberToSendNext].stimWasActiveViaCurrent = false;
		device->externalSendBuffer[device->packageNumberToSendNext].stimChannelDetect_I_PN = 0;
		if (testbed.config.demuxEnabled){
			memset((uint8_t*)device->externalSendBuffer[device->packageNumberToSendNext].demuxChannelsActive, 0, MAX1119X_DEMUX_INPUT_BUFFER_SIZE*2);
		}

		// go to the next package to send
		device->packageNumberToSendNext++;
		device->packageNumberToSendNext &= MAX1119X_NUMBER_OF_PACKAGE_CONTAINER_MASK;

		return true;
	}

	return false;
}

bool testbed_sendAdcData_keepAlive( __IO maxAdc_device_t *device )
{
	if (!device->valid || !device->measurementStarted){
		return false;
	}
	if (device->hcdc->TxState != 0){
		return false;
	}

	GSBP_Handles.ACK.CommandID = MeasurmentDataACK;
	maxAdc_dataPackageNoStim_t *data = (maxAdc_dataPackageNoStim_t*)GSBP_Handles.ACK.Data;
	data->dataType = mAdc_packageType_noStimulation;
	data->counter  = device->counterUpdateDataFunctionCalls;
	data->numberOfSamples = 0;
	data->data[0].ch1 = 0;
	data->data[0].ch2 = 0;
	GSBP_Handles.ACK.DataSize = sizeof(maxAdc_dataPackageNoStim_t);
	GSBP_SendPackage(&GSBP_USB, &GSBP_Handles.ACK);

	return true;
}


void testbed_deInit(void)
{
	max1119x_stopMeasurment(&testbed.maxAdcDevice);
	max1119x_deinitDevice(&testbed.maxAdcDevice);
}
