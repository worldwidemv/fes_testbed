/*
 * MAX1119x.c
 *
 *  Created on: 07.10.2018
 *      Author: valtin
 */

#include "string.h"
#include "MAX1119x.h"

#include "Testbed.h"

void max1119x_TIM_Init(TIM_HandleTypeDef *htim, TIM_TypeDef *timDef, uint8_t samplePeriodMultiplier, uint32_t htimPwmChannel, uint32_t htimOcChannel);

bool max1119x_initDevice(
		__IO maxAdc_device_t *device, USBD_CDC_HandleTypeDef *hcdc,
		TIM_HandleTypeDef *htim, TIM_TypeDef *htimDef, uint32_t PWM_Channel, uint32_t OC_Channel, uint32_t ISR, uint8_t ISR_ID, uint8_t samplePeriodMultiplier,
		SPI_HandleTypeDef *hspiCLK, SPI_HandleTypeDef *hspiData1, SPI_HandleTypeDef *hspiData2,
		uint8_t searchWidth, int16_t searchThresholdCh1Positiv, int16_t searchThresholdCh1Negative, int16_t searchThresholdCh2Positiv, int16_t searchThresholdCh2Negative)
{
	// resets
	memset((void *)device, 0, sizeof(maxAdc_device_t));
	device->valid = false;

	/*
	 * set up the ADC handle
	 */
	device->hspiCLK = hspiCLK;
	device->htim = htim;
	device->htimDef = htimDef;
	device->pwmChannel = PWM_Channel;
	device->ocChannel = OC_Channel;
	device->isr = ISR;
	device->isrID = ISR_ID;
	device->hspiData1 = hspiData1;
	device->hspiData2 = hspiData2;
	device->hcdc = hcdc;
	// config
	device->samplePeriodMultiplier = samplePeriodMultiplier;
	device->searchWidth = searchWidth;
	device->searchThresholdCh1Positiv  = searchThresholdCh1Positiv;
	device->searchThresholdCh1Negative = searchThresholdCh1Negative;
	device->searchThresholdCh2Positiv  = searchThresholdCh2Positiv;
	device->searchThresholdCh2Negative = searchThresholdCh2Negative;

	/*
	 * configure and start the DMA streams for receiving data
	 */
	HAL_SPI_Receive_DMA(device->hspiData1, (uint8_t*)device->dataBufferCh1, MAX1119X_RECEIVE_BUFFER_SIZE);
	HAL_SPI_Receive_DMA(device->hspiData2, (uint8_t*)device->dataBufferCh2, MAX1119X_RECEIVE_BUFFER_SIZE);

	/*
	 *  configure and start timer, SPI and the SPI CLK DMA channel
	 */
	// set up the timer
	max1119x_TIM_Init(device->htim, device->htimDef, device->samplePeriodMultiplier, device->pwmChannel, device->ocChannel);

	// set up SPI Clock, independent from the data channels, copied from the HAL functions
	device->sendBuffer = 0;
	// Check Direction parameter
	assert_param(IS_SPI_DIRECTION_2LINES_OR_1LINE(device->hspiCLK->Init.Direction));
	// Process Locked
	__HAL_LOCK(device->hspiCLK);
	// Set the transaction information
	device->hspiCLK->pTxBuffPtr  = (uint8_t *)&device->sendBuffer;
	device->hspiCLK->TxXferSize  = MAX1119X_SEND_BUFFER_SIZE;
	device->hspiCLK->TxXferCount = 0;
	// Enable the Tx DMA Stream
	HAL_DMA_Start(device->htim->hdma[device->isrID], (uint32_t)&device->sendBuffer, (uint32_t)&device->hspiCLK->Instance->DR, MAX1119X_SEND_BUFFER_SIZE);

	// Check if the SPI is already enabled
	if((device->hspiCLK->Instance->CR1 &SPI_CR1_SPE) != SPI_CR1_SPE){
		// Enable SPI peripheral
		__HAL_SPI_ENABLE(device->hspiCLK);
	}
	// Enable Tx DMA Request bit
	__HAL_TIM_ENABLE_DMA(device->htim, device->isr);

	// Process Unlocked
	__HAL_UNLOCK(device->hspiCLK);

	// start the base timer to generate the DMA requests
	HAL_TIM_Base_Start(device->htim);
	// start the PWM generation
	HAL_TIM_PWM_Start(device->htim, device->pwmChannel);

	device->valid = true;
	return true;
}

bool max1119x_startMeasurment( __IO maxAdc_device_t *device )
{
	if (!device->valid || !device->measurementStarted){
		device->measurementStarted = true;
		return true;
	}
	return false;
}

/*
 * only send the complete buffer part if STIM data was detected within the buffer
 * -> this function is called by the SPI RX DMA channels half and full ISR
 */
void max1119x_updateAdcData_onlyHighResStim( __IO maxAdc_device_t *device, uint16_t fromIndex, uint16_t toIndex)
{
	if (!device->valid || !device->measurementStarted){
		return;
	}

	// update the updateDataFunction call counter
	device->counterUpdateDataFunctionCalls++;

	// resets
	device->stimIsActive = false;
	device->stimIsActiveCurrent = false;
	device->overridePackageData = false;

	// stimulation is currently inactive -> search for the start of the stimulation
	for (uint16_t i=fromIndex; i<toIndex; i+=device->searchWidth){
		// search if the stim current (ch1) exceeds the thresholds
		if (	(device->dataBufferCh1[i] > device->searchThresholdCh1Positiv) ||
				(device->dataBufferCh1[i] < device->searchThresholdCh1Negative) ){
			// the value is larger/smaller than the thresholds -> the stimulation is now active
			device->stimIsActive = true;
			device->stimIsActiveCurrent = true;
			break;
		}
	}
	if (!device->stimIsActive){
		for (uint16_t i=fromIndex; i<toIndex; i+=device->searchWidth){
			// search if the stim voltage (ch2) exceeds the thresholds
			if (	(device->dataBufferCh2[i] > device->searchThresholdCh2Positiv) ||
					(device->dataBufferCh2[i] < device->searchThresholdCh2Negative) ){
				// the value is larger/smaller than the thresholds -> the stimulation is now active
				device->stimIsActive = true;
				break;
			}
		}
	}

	if (device->stimIsActive){
		// stimulation is active, in this section -> send this section to the PC
		__IO maxAdc_dataPackageContainer_t *pkg = &device->externalSendBuffer[device->packageNumberToFillNext];

		// does this package contain UNSEND data?
		if (pkg->readyToBeSend){
			if (!device->stimIsActiveCurrent){
				// this package is ready to be send, so it contains old data which was not yet send -> so we would override data!
				// -> mark this and ABORT, BECAUSE only the voltage exceeded the thresholds and we assume that we have enough bandwidth to send the data,
				// -> that means, this CAN'T be the start of a pulse, it must be the end of the pulse and the new data would be older and therefore less relevant
				// -> abort and send the earlier data
				pkg->package.dataType |= TESTBED_PACKAGE_DATA_DISCARDED;
				return;
			} else {
				// the package is ready to be send, so it contains old data which was not yet send -> so we would override data
				// -> mark this and OVERRIDE the data, BECAUSE the current exceeded the thresholds, so this could be the start of the pulse
				if (pkg->stimWasActiveViaCurrent){
					// the package contains old data where the current exceeds the thresholds  -> the old data is more important
					pkg->package.dataType |= TESTBED_PACKAGE_DATA_DISCARDED;
					return;
				} else {
					// the package contains old data where only the voltage exceeds the thresholds -> the new data is more important
					device->overridePackageData = true;
				}
			}
		}

		pkg->package.numberOfSamples = 0;
		for (uint16_t i=fromIndex; i<toIndex; i++){
			// search if the voltage is below the thresholds
			pkg->package.data[pkg->package.numberOfSamples].ch1 = device->dataBufferCh1[i];
			pkg->package.data[pkg->package.numberOfSamples].ch2 = device->dataBufferCh2[i];
			pkg->package.numberOfSamples++;
		}

		// copying the data is done -> finish up the package
		if (pkg->package.numberOfSamples != 0){
			pkg->package.dataType = mAdc_packageType_Stimulation;
			if (device->overridePackageData){
				pkg->package.dataType |= TESTBED_PACKAGE_DATA_OVERRIDEN;
			}
			pkg->readyToBeSend = true;
			pkg->stimWasActive = true;
			pkg->stimWasActiveViaCurrent = device->stimIsActiveCurrent;
			pkg->package.counter = device->counterUpdateDataFunctionCalls;

			// set the next package up
			device->packageNumberToFillNext++;
			device->packageNumberToFillNext &= MAX1119X_NUMBER_OF_PACKAGE_CONTAINER_MASK;
		}
	}
}


/*
 * send the complete buffer, as soon as the USB interface is free
 */
void max1119x_updateAdcData_partialHighResStream( __IO maxAdc_device_t *device, uint16_t fromIndex, uint16_t toIndex)
{
	if (!device->valid || !device->measurementStarted){
		return;
	}

	// update the updateDataFunction call counter
	device->counterUpdateDataFunctionCalls++;

	if (device->hcdc->TxState == 0){
		// the USB interface is free -> send this section to the PC
		__IO maxAdc_dataPackageContainer_t *pkg = &device->externalSendBuffer[device->packageNumberToFillNext];
		pkg->package.numberOfSamples = 0;

		for (uint16_t i=fromIndex; i<toIndex; i+=device->searchWidth){
			// search if the voltage is below the thresholds
			pkg->package.data[pkg->package.numberOfSamples].ch1 = device->dataBufferCh1[i];
			pkg->package.data[pkg->package.numberOfSamples].ch2 = device->dataBufferCh2[i];
			pkg->package.numberOfSamples++;
		}

		// this package is done -> finish up the package
		if (pkg->package.numberOfSamples != 0){
			pkg->package.dataType = mAdc_packageType_Stimulation;
			if (pkg->readyToBeSend){
				// the package is ready to be send, so it was not yet send -> we override data -< mark this
				pkg->package.dataType |= TESTBED_PACKAGE_DATA_OVERRIDEN;
			}
			pkg->readyToBeSend = true;
			pkg->package.counter = device->counterUpdateDataFunctionCalls;

			// set the next package up
			device->packageNumberToFillNext++;
			device->packageNumberToFillNext &= MAX1119X_NUMBER_OF_PACKAGE_CONTAINER_MASK;
		}
	}
}


void max1119x_updateAdcData_streamLowResWithStim( __IO maxAdc_device_t *device, uint16_t fromIndex, uint16_t toIndex)
{
	if (!device->valid || !device->measurementStarted){
		return;
	}


	uint8_t iPackage = device->packageNumberToFillNext;
	__IO maxAdc_dataPackageContainer_t *pkg = &device->externalSendBuffer[iPackage];
	pkg->package.numberOfSamples = 0;

	__IO uint16_t i = 0;
	__IO uint8_t  j = 0;
	if (!device->stimIsActive){
		// stimulation is currently inactive -> search for the start of the stimulation
		for (i=fromIndex; i<toIndex; i+=device->searchWidth){
			// search if the voltage exceeds the thresholds
			if ((device->dataBufferCh1[i] > device->searchThresholdCh1Positiv) ||
				(device->dataBufferCh1[i] < device->searchThresholdCh1Negative) ){
				// the value is larger/smaller than the thresholds -> the stimulation is now active
				device->stimIsActive = true;
				break;
			} else {
				// the value is not larger/smaller than the threshold
				// the stimulation is inactive -> copy this sample to the buffer
				pkg->package.data[pkg->package.numberOfSamples].ch1 = device->dataBufferCh1[i];
				pkg->package.data[pkg->package.numberOfSamples].ch2 = device->dataBufferCh2[i];
				pkg->package.numberOfSamples++;
			}
		}
		// cleanup
		fromIndex = i;
		// this package is done -> finish up the package
		if (pkg->package.numberOfSamples != 0){
			pkg->package.dataType = mAdc_packageType_noStimulation;
			if (pkg->readyToBeSend){
				// the package is ready to be send, so it was not yet send -> we override data -< mark this
				pkg->package.dataType |= TESTBED_PACKAGE_DATA_OVERRIDEN;
			}
			pkg->readyToBeSend = true;
			device->counterUpdateDataFunctionCalls++;
			pkg->package.counter = device->counterUpdateDataFunctionCalls;

			// set the next package up
			iPackage++;
			iPackage &= MAX1119X_NUMBER_OF_PACKAGE_CONTAINER_MASK;
			pkg = &device->externalSendBuffer[iPackage];
			pkg->package.numberOfSamples = 0;
		}
	}

	if (device->stimIsActive){
		// stimulation is currently active, no mater if it was active before or if the stimulation start was detected in this run
		// -> search for the end of the stimulation pulse
		for (i=fromIndex; i<toIndex; i+=device->searchWidth){
			// search if the voltage is below the thresholds
			if ((device->dataBufferCh1[i] < device->searchThresholdCh1Positiv) ||
				(device->dataBufferCh1[i] > device->searchThresholdCh1Negative) ){
				// the value is smaller/larger than the thresholds -> the stimulation is now inactive
				device->stimIsActive = false;
				break;
			} else {
				// the value is larger/smaller than the threshold
				// the stimulation is active -> copy this sample and the next N samples to the buffer
				for (j=0; j<MAX1119X_SEARCH_WIDTH; j++){
					pkg->package.data[pkg->package.numberOfSamples].ch1 = device->dataBufferCh1[i +j];
					pkg->package.data[pkg->package.numberOfSamples].ch2 = device->dataBufferCh2[i +j];
					pkg->package.numberOfSamples++;
				}
			}
		}
		// cleanup
		fromIndex = i;
		// this package is done -> finish up the package
		if (pkg->package.numberOfSamples != 0){
			pkg->package.dataType = mAdc_packageType_Stimulation;
			if (pkg->readyToBeSend){
				// the package is ready to be send, so it was not yet send -> we override data -< mark this
				pkg->package.dataType |= TESTBED_PACKAGE_DATA_OVERRIDEN;
			}
			pkg->readyToBeSend = true;
			pkg->stimWasActive = true;
			device->counterUpdateDataFunctionCalls++;
			pkg->package.counter = device->counterUpdateDataFunctionCalls;

			// set the next package up
			iPackage++;
			iPackage &= MAX1119X_NUMBER_OF_PACKAGE_CONTAINER_MASK;
			pkg = &device->externalSendBuffer[iPackage];
			pkg->package.numberOfSamples = 0;
		}
	}

	if (!device->stimIsActive){
		// stimulation is currently inactive, but it was active before -> search for the start of the stimulation
		for (i=fromIndex; i<toIndex; i+=device->searchWidth){
			// search if the voltage exceeds the thresholds
			if (	(device->dataBufferCh1[i] > device->searchThresholdCh1Positiv) ||
					(device->dataBufferCh1[i] < device->searchThresholdCh1Negative) ){
				// the value is larger/smaller than the thresholds -> the stimulation is now active
				device->stimIsActive = true;
				// TODO this is an error -> mark this as an error ....
				break;
			} else {
				// the value is not larger/smaller than the threshold
				// the stimulation is inactive -> copy this sample to the buffer
				pkg->package.data[pkg->package.numberOfSamples].ch1 = device->dataBufferCh1[i];
				pkg->package.data[pkg->package.numberOfSamples].ch2 = device->dataBufferCh2[i];
				pkg->package.numberOfSamples++;
			}
		}
		// this package is done -> finish up the package
		if (pkg->package.numberOfSamples != 0){
			pkg->package.dataType = mAdc_packageType_noStimulation;
			if (pkg->readyToBeSend){
				// the package is ready to be send, so it was not yet send -> we override data -< mark this
				pkg->package.dataType |= TESTBED_PACKAGE_DATA_OVERRIDEN;
			}
			pkg->readyToBeSend = true;
			device->counterUpdateDataFunctionCalls++;
			pkg->package.counter = device->counterUpdateDataFunctionCalls;

			// set the next package up
			iPackage++;
			iPackage &= MAX1119X_NUMBER_OF_PACKAGE_CONTAINER_MASK;
		}
	}

	// go to the next package to fill
	device->packageNumberToFillNext = iPackage;
}


bool max1119x_stopMeasurment( __IO maxAdc_device_t *device )
{
	if (!device->valid){
		return false;
	}
	if (device->measurementStarted){
		device->measurementStarted = false;
		return true;
	}
	return false;
}

bool max1119x_deinitDevice( __IO maxAdc_device_t *device )
{
	if (!device->valid){
		return false;
	}
	// stop the measurement
	device->measurementStarted = false;

	// stop the DMA transfer
	HAL_SPI_DMAStop(device->hspiData1);
	HAL_SPI_DMAStop(device->hspiData2);

	// stop the base timer to generate the DMA requests
	HAL_SPI_DMAStop(device->hspiCLK);
	HAL_TIM_Base_Stop(device->htim);
	// stop the PWM generation
	HAL_TIM_PWM_Stop(device->htim, device->pwmChannel);
	// deinitialise the timer
	HAL_TIM_Base_DeInit(device->htim);

	return true;
}


/**
 * @brief Rx Half Transfer completed callback.
 * @param  hspi pointer to a SPI_HandleTypeDef structure that contains
 *               the configuration information for SPI module.
 * @retval None
 */
void HAL_SPI_RxHalfCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi == testbed.maxAdcDevice.hspiData2){
		switch (testbed.config.dataType){
		case tb_stimDataOnly:
			max1119x_updateAdcData_onlyHighResStim( &testbed.maxAdcDevice, 0, MAX1119X_RECEIVE_BUFFER_SIZE_HALF );
			break;
		case tb_constStream:
			max1119x_updateAdcData_partialHighResStream( &testbed.maxAdcDevice, 0, MAX1119X_RECEIVE_BUFFER_SIZE_HALF);
			break;
		case tb_varStream:
			max1119x_updateAdcData_streamLowResWithStim( &testbed.maxAdcDevice, 0, MAX1119X_RECEIVE_BUFFER_SIZE_HALF );
			break;
		}

	}
}


/**
 * @brief Rx Transfer completed callback.
 * @param  hspi pointer to a SPI_HandleTypeDef structure that contains
 *               the configuration information for SPI module.
 * @retval None
 */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi == testbed.maxAdcDevice.hspiData2){
		switch (testbed.config.dataType){
		case tb_stimDataOnly:
			max1119x_updateAdcData_onlyHighResStim( &testbed.maxAdcDevice, MAX1119X_RECEIVE_BUFFER_SIZE_HALF, MAX1119X_RECEIVE_BUFFER_SIZE);
			break;
		case tb_constStream:
			max1119x_updateAdcData_partialHighResStream( &testbed.maxAdcDevice, MAX1119X_RECEIVE_BUFFER_SIZE_HALF, MAX1119X_RECEIVE_BUFFER_SIZE);
			break;
		case tb_varStream:
			max1119x_updateAdcData_streamLowResWithStim( &testbed.maxAdcDevice, MAX1119X_RECEIVE_BUFFER_SIZE_HALF, MAX1119X_RECEIVE_BUFFER_SIZE);
			break;
		}
	}
}

/* TIM init function */
void max1119x_TIM_Init(TIM_HandleTypeDef *htim, TIM_TypeDef *timDef, uint8_t samplePeriodMultiplier, uint32_t htimPwmChannel, uint32_t htimOcChannel)
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;
  TIM_OC_InitTypeDef sConfigOC;
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig;

  /*
   * Basic Timer Config -> Timer Period
   */
  htim->Instance = timDef;
  htim->Init.Prescaler = 0;
  htim->Init.CounterMode = TIM_COUNTERMODE_UP;
  htim->Init.Period = 100*samplePeriodMultiplier;
  htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim->Init.RepetitionCounter = 0;
  if (HAL_TIM_Base_Init(htim) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(htim, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  if (HAL_TIM_OC_Init(htim) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  if (HAL_TIM_PWM_Init(htim) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  /*
   * Configure the Output Comparator ISR
   */
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(htim, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 50*samplePeriodMultiplier + 20*(samplePeriodMultiplier-1);
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_OC_ConfigChannel(htim, &sConfigOC, htimOcChannel) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  /*
   * Configure the PWM Generation for the ADC Clock
   */
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 55*samplePeriodMultiplier;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
  sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
  if (HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC, htimPwmChannel) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(htim, &sBreakDeadTimeConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  HAL_TIM_MspPostInit(htim);

}
