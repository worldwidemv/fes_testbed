/*
 * MCU_StateMachine.c
 *
 *  Created on: 12.01.2016
 *      Author: valtin
 */

#include "MCU_StateMachine.h"

#ifndef APP_GSBP_BASIC_CONFIG_H_
	#include "GSBP_Basic_Config.h"
	#include "GSBP_Basic.h"
#endif

#ifndef set_StateLED_ON
	#define set_StateLED_ON()
	#define set_StateLED_OFF()
#endif

#ifndef APP_MCU_ERRORHANDLER_H_
    #define set_WarnLED_ON()
    #define set_WarnLED_OFF()
    #define set_ErrorLED_ON()
    #define set_ErrorLED_OFF()
	#define E_MCU_State_UnknowState		15
#endif

#ifndef APP_BATTERYMONITORING_H_
#define set_BatteryWeakLED_OFF()
#endif

// Variables
volatile uint8_t MCU_State_LED_TimeOff = 3;
volatile uint8_t MCU_State_LED_TimeOn  = 255;
volatile uint8_t MCU_State_LED_TimeCounter = 0;

uint32_t MCU_State_LowFrequncyTimerCounter = 0;
volatile uint8_t  MCU_State_CurrentState = MCU_PreMCU_Init;
volatile uint8_t  MCU_State_LastState    = MCU_PreMCU_Init;
volatile uint16_t MCU_State_SubSystemStates = MCU_SSS_default;
volatile uint8_t  MCU_State_EventType = 0;
// Functions

void MCU_EventState( uint8_t EventType )
{
	MCU_State_LastState = MCU_State_CurrentState;
	MCU_State_EventType = EventType;
	MCU_State_CurrentState = MCU_RunEventState;
}

/* MCU state machine */
void MCU_StateChange(mcu_States_t GotoState)
{
	MCU_State_LastState = MCU_State_CurrentState;
	switch (GotoState) {
	case MCU_PreMCU_Init:
		// time between end of STM32 HAL init and start of the endless loop -> normally very short
		MCU_State_CurrentState = MCU_PreMCU_Init;
		set_StateLED_OFF();
		set_WarnLED_OFF();
		set_ErrorLED_OFF();
		// indicate the current state
		set_StateLED_ON();
		MCU_State_SubSystemStates |= MCU_SSS_StatusLED_isOn;
		// turn the LED on as long as possible
		MCU_State_LED_TimeOn  = 255;
		MCU_State_LED_TimeOff = 1;
		break;
	case MCU_PreInit_NotConnected:
		// the MCU is in the endless loop but no connection is currently established (UART/USB/BT ...)
		MCU_State_CurrentState = MCU_PreInit_NotConnected;
		// set the LED values for this state
		MCU_State_LED_TimeOn  = 3;
		MCU_State_LED_TimeOff = 3;
		break;
	case MCU_PreInit_Connected:
		// a connection is established (UART/USB/BT ...), so we can receive commands but did not receive the init command yet
		MCU_State_CurrentState = MCU_PreInit_Connected;
		// set the LED values for this state
		MCU_State_LED_TimeOn  = 2;
		MCU_State_LED_TimeOff = 8;
		break;

	case MCU_DoInit:
		// a connection is established (UART/USB/BT ...), and we receive the init command -> do the init
		MCU_State_CurrentState = MCU_DoInit;
		// set the LED values for this state
		MCU_State_LED_TimeOn  = 8;
		MCU_State_LED_TimeOff = 2;
		break;
	case MCU_PostInit:
		// a connection is established (UART/USB/BT ...),  we receive the init command and the init is done
		MCU_State_CurrentState = MCU_PostInit;
		// set the LED values for this state
		MCU_State_LED_TimeOn  = 8;
		MCU_State_LED_TimeOff = 8;
		break;

	case MCU_MeasurmentDoStart:
		// the system is initalized and the start command was received -> start the measurement
		MCU_State_CurrentState = MCU_MeasurmentDoStart;
		// set the LED values for this state
		MCU_State_LED_TimeOn  = 1;
		MCU_State_LED_TimeOff = 1;
		break;
	case MCU_MeasurmentActiv:
		// the system started to make measurements
		MCU_State_CurrentState = MCU_MeasurmentActiv;
		// set the LED values for this state
		MCU_State_LED_TimeOn  = 1;
		MCU_State_LED_TimeOff = 1;
		break;
	// task specific states



	case MCU_MeasurmentDoStop:
		// the system is measuring but received a stop command -> stop the measurements
		MCU_State_CurrentState = MCU_MeasurmentDoStop;
		// set the LED values for this state
		MCU_State_LED_TimeOn  = 1;
		MCU_State_LED_TimeOff = 1;
		break;
	case MCU_MeasurmentFinished:
		// the system stopped to make measurements
		MCU_State_CurrentState = MCU_MeasurmentFinished;
		// set the LED values for this state
		MCU_State_LED_TimeOn  = 8;
		MCU_State_LED_TimeOff = 8;
		break;

	case MCU_DoDeInit:
		// the system is initialized but everything should be reseted -> deinitialise
		MCU_State_CurrentState = MCU_DoDeInit;
		// set the LED values for this state
		MCU_State_LED_TimeOn  = 8;
		MCU_State_LED_TimeOff = 2;
		break;

    // special states
	case MCU_PreSleep:
		// Todo
		break;

	case MCU_PostSleep:
		// Todo
		break;

	case MCU_UnrecoverableError:
		// an unrecoverable error occurred -> project specific implementation -> TODO
		// do not change the LED timer values, so you can see, in which state the error occurred
		break;
	case MCU_Reset:
		// software reset
		NVIC_SystemReset();
		break;
	case MCU_Shutdown:
		// shut down the MCU done to low battery
		if (GSBP_Handles.ActiveHandles > 0){
			GSBP_SendStatus(GSBP_Handles.DefaultHandle);
		}
		// turn of the LEDs
		MCU_State_LED_TimeOn  = 0;
		MCU_State_LED_TimeOff = 0;
		set_StateLED_OFF();
		set_WarnLED_OFF();
		set_ErrorLED_OFF();
		set_BatteryWeakLED_OFF();
		HAL_Delay(100);
		// stop the MCU
		HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFE);
		break;
	default:
		// send an error message to the PC if a connection was found
		if (GSBP_Handles.ActiveHandles > 0) {
			GSBP_SendError(GSBP_Handles.DefaultHandle, E_MCU_State_UnknowState);
		}
		// leaf the state as it is
	}
}


void MCU_StatusCallback(void)
{
	// signal the state with the state LED
	MCU_State_LED_TimeCounter++;
	if (MCU_State_SubSystemStates & MCU_SSS_StatusLED_isOn){
		// LED is on -> shut it down
		if (MCU_State_LED_TimeCounter >= MCU_State_LED_TimeOn){
			// reset the counter
			MCU_State_LED_TimeCounter = 0;
			// set the state LED to OFF
			set_StateLED_OFF();
			// update the subsystem state
			MCU_State_SubSystemStates &= ~(MCU_SSS_StatusLED_isOn);
		}
	}
	if (!(MCU_State_SubSystemStates & MCU_SSS_StatusLED_isOn)){
		// LED is off -> turn it on
		if ( (MCU_State_LED_TimeCounter >= MCU_State_LED_TimeOff) && (MCU_State_LED_TimeOn > 0) ){
			// reset the counter
			MCU_State_LED_TimeCounter = 0;
			// set the state LED to on
			set_StateLED_ON();
			// update the subsystem state
			MCU_State_SubSystemStates |= MCU_SSS_StatusLED_isOn;
		}
	}

	// automated state change
/*	switch (MCU_State_CurrentState) {
	case MCU_PreInit_NotConnected:
		if (GSBP_Handles.InterfacesEnabled != GSBP_NoInterface){
			MCU_StateChange(MCU_PreInit_Connected);		}
		break;
	case MCU_PreInit_Connected:
		if (GSBP_Handles.InterfacesEnabled == GSBP_NoInterface){
			MCU_StateChange(MCU_PreInit_NotConnected);		}
		break;
	}
*/
}

uint8_t MCU_GetStatusInformation(uint8_t *Data)
{
    uint8_t index = 0;

    /* Standard GSBP Payload */
    // MCU status
    Data[index++] = 0;  // TODO
    Data[index++] = 0;
    // supply voltage
    convert_uint32_t temp;
#ifndef APP_BATTERYMONITORING_H_
    temp.data = (uint32_t) (3.3333333*1000);
#else
    temp.data = (uint32_t) BatteryMonitoring_GetVoltage_mV();
#endif
    Data[index++] = temp.c_data[0];
    Data[index++] = temp.c_data[1];
    Data[index++] = temp.c_data[2];
    Data[index++] = temp.c_data[3];

    /* project specific implementation ... TODO */

	return index;
}

void MCU_LowFrequencyTimerCallback(void)
{

	// MCU Status Callback e.g. for LEDs
	MCU_StatusCallback();

#ifdef APP_BATTERYMONITORING_H_
	// the BatteryMonitoring callback
	BatteryMonitoring_TimerCallback();
#endif

	// GeneralSerialByteProtocol Enable/Disable the GSBP interfaces
#ifdef APP_GSBP_COMM_H_
	GSBP_EnableInterfacesCallback();
	GSBP_DisableInterfacesCallback();
#endif

}
/*
void MCU_ErrorHandler(mcu_ErrorTyp_t ErrorType)
{
	uint8_t i;

	// handle some errors
	switch (ErrorType){
	default:
		/* Flash the status LED to indicate the error type * /
		while(1) {
			// what a large amount of time
			set_ErrorLED_OFF();
			HAL_Delay(ERROR_HANDLER__ERROR_LED__DELAY_LARGE_OFF);
			set_ErrorLED_ON();
			HAL_Delay(ERROR_HANDLER__ERROR_LED__DELAY_LARGE_ON);
			set_ErrorLED_OFF();
			HAL_Delay(ERROR_HANDLER__ERROR_LED__DELAY_SMALL_OFF);
			// flash the LED X times to indicate the error type
			for (i=1; i<ErrorType; i++){
				set_ErrorLED_ON();
				HAL_Delay(ERROR_HANDLER__ERROR_LED__DELAY_SMALL_ON);
				set_ErrorLED_OFF();
				HAL_Delay(ERROR_HANDLER__ERROR_LED__DELAY_SMALL_OFF);
			}
		}
	}
}
*/
