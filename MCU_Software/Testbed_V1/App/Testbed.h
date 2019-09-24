/*
 * Testbed.h
 *
 *  Created on: 17.10.2018
 *      Author: valtin
 */

#ifndef TESTBED_H_
#define TESTBED_H_

#include "stdint.h"
#include "stdbool.h"
#include "main.h"

#include "MAX1119x.h"
#include "TCA9535.h"

#include "GSBP_Basic_Config.h"


/*
 * defines
 */

#define TESTBED_GSBP_PACKAGE_OVERHEAD_SIZE		8
#define TESTBED_ADC_NO_STIM_WAIT_TIME_MS		60

#define TESTBED_STIM_DETECTION_MASK_PORT_A		(SC_P2_Pin)
#define TESTBED_STIM_DETECTION_MASK_PORT_B		(SC_P1_Pin)
#define TESTBED_STIM_DETECTION_MASK_PORT_C		(SC_P3_Pin | SC_P4_Pin | SC_N1_Pin | SC_N2_Pin | SC_N3_Pin | SC_N4_Pin)

#define TESTBED_PACKAGE_DATA_OVERRIDEN			0x80
#define TESTBED_PACKAGE_DATA_DISCARDED 			0x40
#define TESTBED_PACKAGE_TYPE_MASK				~(TESTBED_PACKAGE_DATA_OVERRIDEN | TESTBED_PACKAGE_DATA_DISCARDED)
/*
 * definitions
 */
typedef enum {
	tb_noData		= 0,
	tb_constStream 	= 1,
	tb_stimDataOnly = 2,
	tb_varStream	= 3
} testbed_dataTypes_t;

typedef struct __packed {
	uint8_t dataType;				// type of data to be send
	uint8_t samplePeriodeMultiplier;// sample rate multiplier for the ADC
	uint8_t searchWidth;    		// search wide, for detecting the stimulation
	int16_t thresholdCh1Positiv;
	int16_t thresholdCh1Negative;
	int16_t thresholdCh2Positiv;
	int16_t thresholdCh2Negative;

	uint8_t demuxEnabled;
	uint8_t demuxChannelsActive;
	uint8_t demuxChannelsPassive;
} testbed_initCmd_t;

typedef struct __packed {
	uint8_t 	successful;
	uint32_t 	sampleBufferSize;
	double    	sampleFrequency;
} testbed_initAck_t;

typedef struct {
	testbed_initCmd_t config;

	__IO bool updateStimChannels;
	uint32_t keepAlive_packageCounter;
	__IO maxAdc_device_t maxAdcDevice;
} testbed_t;

/*
 * global variables
 */
extern testbed_t testbed;


/*
 * functions
 */
void testbed_init(testbed_initCmd_t *config, testbed_initAck_t *result);

bool testbed_sendAdcData_keepAlive( __IO maxAdc_device_t *device );
bool testbed_sendAdcData_realData( __IO maxAdc_device_t *device );

void testbed_deInit(void);


static inline void testbed_saveChannelInfoStim_PositivAndNegative(void)
{
	HAL_GPIO_TogglePin(D2_GPIO_Port, D2_Pin);
	// save the GPIO state of the STIM channels
	testbed.maxAdcDevice.externalSendBuffer[testbed.maxAdcDevice.packageNumberToFillNext].stimChannelDetect_I_PN |= GPIOC->IDR & TESTBED_STIM_DETECTION_MASK_PORT_C;
	testbed.maxAdcDevice.externalSendBuffer[testbed.maxAdcDevice.packageNumberToFillNext].stimChannelDetect_I_PN |= GPIOB->IDR & TESTBED_STIM_DETECTION_MASK_PORT_B;
	testbed.maxAdcDevice.externalSendBuffer[testbed.maxAdcDevice.packageNumberToFillNext].stimChannelDetect_I_PN |= GPIOA->IDR & TESTBED_STIM_DETECTION_MASK_PORT_A;
	HAL_GPIO_TogglePin(D2_GPIO_Port, D2_Pin);
}

#endif /* TESTBED_H_ */
