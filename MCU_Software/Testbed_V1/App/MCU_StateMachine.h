/*
 * MCU_States.h
 *
 *  Created on: 12.01.2016
 *      Author: valtin
 */

#ifndef APP_MCU_STATEMACHINE_H_
#define APP_MCU_STATEMACHINE_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "main.h"
#include "stm32f4xx_hal.h"
#include "GSBP_Basic_Config.h"
//#include "MCU_ErrorHandler.h"
//#include "MCU_BatteryMonitoring.h"
// Definitions

#define MCU_STATEMACHINE__LOWFREQUENCY_TIMER_PERIOD_IN_MS	100

// Declarations


typedef enum {
	// standard states
	MCU_PreMCU_Init				= 0,
	MCU_PreInit_NotConnected	= 5,
	MCU_PreInit_Connected		= 10,
	MCU_DoInit					= 13,
	MCU_PostInit				= 15,
	MCU_MeasurmentDoStart		= 50,
	MCU_MeasurmentActiv			= 53,
	MCU_MeasurmentDoStop		= 55,
	MCU_MeasurmentFinished		= 60,
	MCU_DoDeInit				= 180,
	// task specific states


	// special states
	MCU_PreSleep				= 200,
	MCU_PostSleep				= 201,
	MCU_UnrecoverableError		= 210,
	MCU_Reset					= 240,
	MCU_Shutdown				= 250,
	MCU_RunEventState			= 255
} mcu_States_t;

typedef enum {
	MCU_SSS_default				= 0b0000000000000000,
	MCU_SSS_StatusLED_isOn		= 0b0000000000000001,
	MCU_SSS_BatteryWeak			= 0b0000000000000010,
	MCU_SSS_MeasurmentActive	= 0b0000000000000100,
	MCU_SSS_IMUDATA__DATAREADY  = 0b1000000000000000,
	// task specific implementation
} mcu_SubSystemStates_t;

typedef enum {
    UART_InitError      				= 1,
    UART_ReceiveError   				= 2,
    UART_TransmitError  				= 3,
	UART_HandleError    				= 4,
	GSBP_InterfaceError					= 10,
	MCU_State_UnknowState				= 100,
	BatteryMonitoring__ADC_InitError    = 110,
    Timer_InitError     				= 120,
} mcu_ErrorTyp_t;

// Variables
extern uint32_t MCU_State_LowFrequncyTimerCounter;
extern volatile uint8_t  MCU_State_CurrentState;
extern volatile uint8_t  MCU_State_LastState;
extern volatile uint16_t MCU_State_SubSystemStates;
extern GSBP_HandlesList_t GSBP_Handles;

// Functions
void    MCU_StateChange(mcu_States_t GotoState);
void    MCU_StatusCallback(void);
uint8_t MCU_GetStatusInformation(uint8_t *Data);

void	MCU_LowFrequencyTimerCallback(void);

#ifdef __cplusplus
 }
#endif

#endif /* APP_MCU_STATEMACHINE_H_ */
