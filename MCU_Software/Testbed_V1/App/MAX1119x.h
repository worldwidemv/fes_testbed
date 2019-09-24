/*
 * MAX1119x.h
 *
 *  Created on: 07.10.2018
 *      Author: valtin
 */

#ifndef MAX1119X_H_
#define MAX1119X_H_

#include "stdint.h"
#include "stdbool.h"
#include "main.h"
#include "tim.h"
#include "spi.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

#define MAX1119X_RECEIVE_BUFFER_SIZE					2000
#define MAX1119X_RECEIVE_BUFFER_SIZE_HALF				(MAX1119X_RECEIVE_BUFFER_SIZE/2)
#define MAX1119X_SEND_BUFFER_SIZE						1

#define MAX1119X_NUMBER_OF_PACKAGE_CONTAINER			16
#define MAX1119X_NUMBER_OF_PACKAGE_CONTAINER_MASK		0x0F
#define MAX1119X_PACKAGE_OFFSET							7

#define MAX1119X_DEMUX_INPUT_BUFFER_SIZE 				16		// 8*16 = up to 128 channels

#define MAX1119X_SEARCH_WIDTH							15
#define MAX1119X_SEARCH_THRESHOLD_DEFAULT_CH1_POSITIV	+250
#define MAX1119X_SEARCH_THRESHOLD_DEFAULT_CH1_NEGATIVE	-150
#define MAX1119X_SEARCH_THRESHOLD_DEFAULT_CH2_POSITIV	+250
#define MAX1119X_SEARCH_THRESHOLD_DEFAULT_CH2_NEGATIVE	-150

#define MAX1119X_BASE_SAMPLING_FREQUENCY				1000000.0

//Typedefs
typedef enum {
	mAdc_packageType_none			= 0,
	mAdc_packageType_noStimulation	= 1,
	mAdc_packageType_Stimulation	= 2
} maxAdc_dataPackageType_t;

typedef struct __packed {
	int16_t ch1;
	int16_t ch2;
} maxAdc_dataBuffer_t;

typedef struct __packed {
	uint16_t		numberOfSamples;
	maxAdc_dataBuffer_t	data[MAX1119X_RECEIVE_BUFFER_SIZE_HALF];
} maxAdc_dataPackage_samples_t;

typedef struct __packed {
	uint32_t 		counter;
	uint8_t			dataType;
	uint16_t		numberOfSamples;
	maxAdc_dataBuffer_t	data[1];
} maxAdc_dataPackageNoStim_t;

typedef struct __packed {
	uint8_t			GSBP_Header[6]; // 3

	uint32_t 		counter;
	uint8_t			dataType;
	uint16_t		numberOfSamples;
	maxAdc_dataBuffer_t	data[MAX1119X_RECEIVE_BUFFER_SIZE_HALF];
    uint8_t			GSBP_Footer[34];
} maxAdc_dataPackage_t;

typedef struct {
	bool 				readyToBeSend;
	bool				stimWasActive;
	bool				stimWasActiveViaCurrent;
	uint16_t			stimChannelDetect_I_PN;
	uint8_t				demuxChannelsActive[MAX1119X_DEMUX_INPUT_BUFFER_SIZE];
	uint8_t				demuxChannelsPassive[MAX1119X_DEMUX_INPUT_BUFFER_SIZE];
	maxAdc_dataPackage_t package;
} maxAdc_dataPackageContainer_t;

typedef struct {
	bool 				valid;
	SPI_HandleTypeDef 	*hspiCLK;	// SPI interface used to send data to the ADC and generate the CLK
	TIM_HandleTypeDef 	*htim;
	TIM_TypeDef 		*htimDef;
	uint32_t			pwmChannel;
	uint32_t			ocChannel;
	uint32_t			isr;
	uint8_t				isrID;
	SPI_HandleTypeDef 	*hspiData1;	// SPI interface for data ch1
	SPI_HandleTypeDef 	*hspiData2;	// SPI interface for data ch2
	USBD_CDC_HandleTypeDef *hcdc;

	uint16_t			sendBuffer;
	int16_t 			dataBufferCh1[MAX1119X_RECEIVE_BUFFER_SIZE];
	int16_t 			dataBufferCh2[MAX1119X_RECEIVE_BUFFER_SIZE];

	bool 				measurementStarted;
	bool				stimIsActive;
	bool				stimIsActiveCurrent;
	bool				overridePackageData;
	uint8_t 			samplePeriodMultiplier;
	uint8_t 			searchWidth;
	int16_t 			searchThresholdCh1Positiv;
	int16_t 			searchThresholdCh1Negative;
	int16_t 			searchThresholdCh2Positiv;
	int16_t 			searchThresholdCh2Negative;
	uint8_t 			packageNumberToFillNext;

	uint32_t 			counterUpdateDataFunctionCalls;	// keeps track how often updateData_* functions are called
														// -> numberOfSamples = MAX1119X_RECEIVE_BUFFER_SIZE/2 *counterUpdateDataFunctionCalls
	uint8_t 			packageNumberToSendNext;
	maxAdc_dataPackageContainer_t externalSendBuffer[MAX1119X_NUMBER_OF_PACKAGE_CONTAINER];
} maxAdc_device_t;


/*
 * Init function for the MAX1119x ADC
 * param1 maxAdc_device_t: object for all the internal ADC variables
 * param2 TIM_HandleTypeDef: timer handle -> the timer generates the sampling frequency and triggers the DMA transfers
 * 											 the timer should be configured with CubeMX to generate a PWM signal on the first channel and have a the TIM update DMA channel configured
 * param3 SPI_HandleTypeDef: first spi handle  -> should be configured as full-duplex master with a high priority DMA channel for the receiver (half bit, circular, half and full fifo ISRs
 * param4 SPI_HandleTypeDef: second spi handle -> should be configured as receiver slave with a high priority DMA channel for the receiver (half bit, circular, half and full fifo ISRs
 */
bool max1119x_initDevice(
		__IO maxAdc_device_t *device, USBD_CDC_HandleTypeDef *hcdc,
		TIM_HandleTypeDef *htim, TIM_TypeDef *htimDef, uint32_t PWM_Channel, uint32_t OC_Channel, uint32_t ISR, uint8_t ISR_ID, uint8_t samplePeriodMultiplier,
		SPI_HandleTypeDef *hspiCLK, SPI_HandleTypeDef *hspiData1, SPI_HandleTypeDef *hspiData2,
		uint8_t searchWidth, int16_t searchThresholdCh1Positiv, int16_t searchThresholdCh1Negative, int16_t searchThresholdCh2Positiv, int16_t searchThresholdCh2Negative);

bool max1119x_startMeasurment( __IO maxAdc_device_t *device );

void max1119x_updateAdcData_onlyHighResStim( __IO maxAdc_device_t *device, uint16_t fromIndex, uint16_t toIndex);
void max1119x_updateAdcData_partialHighResStream( __IO maxAdc_device_t *device, uint16_t fromIndex, uint16_t toIndex);
void max1119x_updateAdcData_streamLowResWithStim( __IO maxAdc_device_t *device, uint16_t fromIndex, uint16_t toIndex);

bool max1119x_stopMeasurment( __IO maxAdc_device_t *device );

bool max1119x_deinitDevice( __IO maxAdc_device_t *device );

#endif /* MAX1119X_H_ */
