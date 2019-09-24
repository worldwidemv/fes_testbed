/*
 * GeneralSerialByteProtocol.c
 *
 *  Created on: Jan 16, 2015
 *      Author: Markus Valtin
 */

#include <string.h>
#include "main.h"
#include "GSBP_Basic_Config.h"
#include "GSBP_Basic.h"

#include "MCU_StateMachine.h"

/*
 * ### Task/Board specific global Variables ###
 * TODO
 * Add task/board specific implementation for each available interface
 */
#if (GSBP_SETUP__INTERFACE_UART_USED)
	#include "usart.h"
	GSBP_Handle_t GSBP_UART;			// UART interface
#endif
#if (GSBP_SETUP__INTERFACE_USB_USED)
	GSBP_Handle_t GSBP_USB;			// real MCU USB interface
#endif


/*
 * ### Task/Board specific Functions
 */

/*
 * Initialize all handle variables and start the interfaces
 */
void GSBP_Init(void)
{
	// initialize the global handle list
	memset(&GSBP_Handles, 0, sizeof(GSBP_Handles));
    GSBP_Handles.DefaultHandle = 0;

    /*
     * TODO
     * Add task/board specific implementation for each available interface
     */

    // clear the GSBP handles and configure them
#if (GSBP_SETUP__INTERFACE_UART_USED)
	memset(&GSBP_UART, 0, sizeof(GSBP_Handle_t));
	GSBP_UART.InterfaceType = GSBP_InterfaceUART;
	GSBP_UART.State |=  GSBP_HandleState__HandleDisabled;
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
	memset(&GSBP_USB, 0, sizeof(GSBP_Handle_t));
	GSBP_USB.InterfaceType = GSBP_InterfaceUSB;
	GSBP_USB.State |= GSBP_HandleState__HandleDisabled;
#endif

	// configure the default handle
	GSBP_USB.State |= GSBP_HandleState__DefaultHandle;

	// enable the handles if you do not use the GSBP_Management functions
#if (GSBP_SETUP__INTERFACE_UART_USED)
	GSBP_InitHandle(&GSBP_UART, &huart2);
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
	GSBP_InitHandle(&GSBP_USB, NULL);
#endif
}






/*
 * Check the interface/handle if any new data was received and if so,
 * check if the package is complete and execute the commands
 * Input: GSBP interface handle
 * Output: number of evaluated packages
 */
uint8_t GSBP_EvaluatePackage(GSBP_Handle_t *Handle)
{
	gsbp_PackageTX_t *Ack = &GSBP_Handles.ACK;
	uint8_t PackagesEvaluated = 0;
	// check if new commands were received
	if ( GSBP_SaveBuffer(Handle) >= GSBP__PACKAGE_SIZE_MIN ) {
		// get one package and process it's payload
		while (GSBP_BuildPackage(Handle, &GSBP_Handles.CMD)){
			// DEBUG: send the received command id back (direct, so better use a debug interface)
			// HAL_UART_Transmit_DMA(GSBP_UART.UART_Handle, (uint8_t *) &GSBP_Handles.CMD.CommandID, (uint16_t)1);
			switch (GSBP_Handles.CMD.CommandID) {
			case ResetCMD:
				Ack->CommandID = ResetACK;
				Ack->DataSize = 0;
				GSBP_SendPackage(Handle, Ack);
				// reset the MCU
				MCU_StateChange(MCU_Reset);
				break;
			case InitCMD:
				switch (MCU_State_CurrentState){
				case MCU_PreInit_Connected:
				case MCU_PostInit:
				case MCU_MeasurmentActiv:
				case MCU_MeasurmentFinished:
					// start the initialization -> got to DoInit state
					MCU_StateChange(MCU_DoInit);
					break;
				default:
					GSBP_SendError(Handle, E_CMD_NotValidNow);
				}
				break;
			case StartMeasurmentCMD:
				switch (MCU_State_CurrentState){
				case MCU_PostInit:
				case MCU_MeasurmentFinished:
					// start the measurement -> got to MeasurementDoStart state
					MCU_StateChange(MCU_MeasurmentDoStart);
					break;
				case MCU_MeasurmentActiv:
					// already in the measurement mode -> only send Ack
					GSBP_Handles.ACK.CommandID = StartMeasurmentACK;
					GSBP_Handles.ACK.DataSize = 0;
					GSBP_SendPackage(GSBP_Handles.DefaultHandle, &(GSBP_Handles.ACK));
					break;
				default:
					GSBP_SendError(Handle, E_CMD_NotValidNow);
				}
				break;
			case StopMeasurmentCMD:
				switch (MCU_State_CurrentState){
				case MCU_MeasurmentActiv:
				case MCU_MeasurmentFinished:
					// stop the measurement -> got to MeasurementDoStop state
					MCU_StateChange(MCU_MeasurmentDoStop);
					break;
				default:
					GSBP_SendError(Handle, E_CMD_NotValidNow);
				}
				break;

			case StatusCMD:
				GSBP_SendStatus(Handle);
				break;
			case RepeatLastCommandCMD:
				Ack->CommandID = RepeatLastCommandACK;
				Ack->DataSize = 0;
				// todo repeat last command
				GSBP_SendPackage(Handle, Ack);
				break;
			case DeInitCMD:
				// reset -> got to MCU_DoDeInit state
				MCU_StateChange(MCU_DoDeInit);
				break;
			case DebugCMD:
				// not used
				break;
			case ErrorCMD:
				// TODO Error handling
				break;
			case EchoCMD:
				// Echo Command
				Ack->CommandID = EchoACK;
				Ack->DataSize = GSBP_Handles.CMD.DataSize;
				memcpy(Ack->Data, GSBP_Handles.CMD.Data, GSBP_Handles.CMD.DataSize);
				GSBP_SendPackage(Handle, Ack);
				break;
			default:
				GSBP_SendError(Handle, E_UnknownCMD);
			}
			PackagesEvaluated++;
		}
	}
	return PackagesEvaluated;
}
