/*
 * uart_comm.h
 *
 *  Created on: Jan 16, 2015
 *      Author: Markus Valtin
 */
#include "stm32f4xx_hal.h"
//#include "Debug.h"

#ifndef APP_GSBP_BASIC_CONFIG_H_
#define APP_GSBP_BASIC_CONFIG_H_

#ifdef __cplusplus
 extern "C" {
#endif

/*
 * ### Task/Board specific SETUP ###
 * TODO
 * Adjust the following settings
 */
// GSBP interfaces to use
#define GSBP_SETUP__INTERFACE_UART_USED			0		// 0= not active; 1= is active
#define GSBP_SETUP__INTERFACE_USB_USED			1		// 0= not active; 1= is active
#define GSBP_SETUP__NUMBER_OF_HANDLES			1		// How many UART/USB/... connections exist?

// GSPB_SaveBuffer / GSBP_EvaluatePackage
#define GSBP_SETUP__CALLBACK_PERIOD_IN_MS		20		// How often do you want to check for new packages?
#define GSBP_SETUP__CALLBACK_PERIOD_MIN_IN_MS	3		// For resets, if we expect a command.

// GSBP payload/package settings
#define GSBP_SETUP__MAX_PAYLOAD_SIZE_RX         100		// What is the largest payload/package you expect to receive?
#define GSBP_SETUP__MAX_PAYLOAD_SIZE_TX         200		// What is the largest payload/package you expect to send?
#define GSBP_SETUP__UART_RX_METHOD				2		// 0 = polling based; 1 = interrupt based; 2 = DMA based
#define GSBP_SETUP__UART_TX_METHOD				2		// 0 = polling based; 1 = interrupt based; 2 = DMA based
#define GSBP_SETUP__UART_RX_POLLING_TIMEOUT		20		// Timeout for the UART_Read function
#define GSBP_SETUP__UART_TX_SEND_TIMEOUT		20		// Timeout for the UART_Send function

// GSBP buffer size (override if necessary)
#define GSBP_SETUP__RX_BUFFER_SIZE				2*GSBP_SETUP__MAX_PAYLOAD_SIZE_RX
#define GSBP_SETUP__TX_BUFFER_SIZE				2*GSBP_SETUP__MAX_PAYLOAD_SIZE_RX

/*
 * ### Task/Board specific COMMAD and ERROR declarations ###
 */
// GSBP Command IDs used
typedef enum {
	// GSBP standard commands, starting at 235
	ResetCMD						= 237,
	ResetACK						= 238,
	InitCMD							= 239,
	InitACK							= 240,
	DeInitCMD						= 235,
	DeInitACK						= 236,
	StartMeasurmentCMD				= 241,
	StartMeasurmentACK				= 242,
	StopMeasurmentCMD				= 243,
	StopMeasurmentACK				= 244,
	StatusCMD						= 245,
	StatusACK						= 246,
	RepeatLastCommandCMD			= 247,
	RepeatLastCommandACK			= 248,
	DebugCMD						= 249,
	ErrorCMD						= 250,
	/*
	 * TODO
	 * Add task/board specific commands
	 */
	MeasurmentDataACK				= 100,
	EchoCMD							= 110,
	EchoACK							= 111
} gsbp_Command_t;

// GSBP Error codes used with GSBP_SendError()
 typedef enum {
	// GSBP standard errors
	E_UnknownCMD                	= 1,
	E_ChecksumMissmatch             = 2,
    E_EndByteMissmatch              = 3,
    E_UARTSizeMissmatch             = 4,
	E_CMD_NotValidNow				= 11,
	E_CMD_NotExpected				= 12,
    E_State_UnknowState				= 15,
	/*
	 * TODO
	 * Add task/board specific error codes
	 */
	E_NpNewData						= 20
} gsbp_ErrorCode_t;


/*
 * ### Task/Board specific global variables ###
 * TODO
 * Add task/board specific implementation for each available interface
 */
// Include the GSBP_Basic definitions which will use these settings
#include "GSBP_Basic.h"

#if (GSBP_SETUP__INTERFACE_UART_USED)
	extern GSBP_Handle_t GSBP_UART;
	// BT
	extern GSBP_Handle_t GSBP_BT;
#endif
#if (GSBP_SETUP__INTERFACE_USB_USED)
	extern GSBP_Handle_t GSBP_USB;
#endif


/*
 * ### Task/Board specific function declarations ###
 * TODO
 * Add task/board specific implementation as needed
 */

void GSBP_Init(void);
uint8_t GSBP_EvaluatePackage(GSBP_Handle_t *Handle);

inline uint8_t GSBP_CheckAndEvaluatePackages(void){
	// check all enabled GSBP handles
	uint8_t i = 0, PackagesEvaluated = 0;

	for (i=0; i< GSBP_Handles.ActiveHandles; i++){
		if (GSBP_Handles.Handles[i]->UART_Handle != NULL){
			PackagesEvaluated += GSBP_EvaluatePackage(GSBP_Handles.Handles[i]);
		}
	}
	return PackagesEvaluated;
}

/*
 * Callbacks
 */
void InitCallback(gsbp_PackageRX_t *pInitCMD);

#ifdef __cplusplus
 }
#endif
#endif /* APP_GSBP_BASIC_CONFIG_H_ */
