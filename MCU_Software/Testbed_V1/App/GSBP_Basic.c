/*
 * GeneralSerialByteProtocol.c
 *
 *  Created on: Jan 16, 2015
 *      Author: Markus Valtin
 */

#include <string.h>
#include "GSBP_Basic_Config.h"
#include "GSBP_Basic.h"


#if (GSBP_SETUP__INTERFACE_UART_USED)
	#include "usart.h"
#endif
#if (GSBP_SETUP__INTERFACE_USB_USED)
	#include "usbd_cdc_if.h"
#endif


/***
 * ### GSBP general global variables ###
 ***/
GSBP_HandlesList_t GSBP_Handles;



/***
 * ### GSBP "public" API functions ###
 *
 * general functions
 ***/

// Initialize a handle variable and start the interface
void GSBP_InitHandle(GSBP_Handle_t *Handle, UART_HandleTypeDef *huart)
{
	// initial checks
	if (Handle->State  & GSBP_HandleState__HandleEnabled){
		// handle already active -> quit
		return;
	}
	if (GSBP_Handles.ActiveHandles  >= GSBP_SETUP__NUMBER_OF_HANDLES){
		// all handles are already active and assigned -> quit / error?
		return;
	}

	// add handle to the GSBP handle list
	Handle->HandleListIndex = GSBP_Handles.ActiveHandles;
	GSBP_Handles.Handles[GSBP_Handles.ActiveHandles] = Handle;
	GSBP_Handles.ActiveHandles++;

	// update the state
	Handle->State |=  GSBP_HandleState__HandleEnabled;
	Handle->State &= ~GSBP_HandleState__HandleDisabled;

#if (GSBP_SETUP__INTERFACE_UART_USED)
	// UART interface
	if (Handle->InterfaceType == GSBP_InterfaceUART){
		// set the UART handle pointer
		Handle->UART_Handle = huart;
		// start Receiving
		#if (GSBP_SETUP__UART_RX_METHOD == 0)	// POLLING
			// noting to do here
		#elif (GSBP_SETUP__UART_RX_METHOD == 1)	// IT
		if (HAL_UART_Receive_IT(Handle->UART_Handle, Handle->RxBuffer, GSBP_SETUP__RX_BUFFER_SIZE) != HAL_OK){
			MCU_ErrorHandler(EH_UART_InitError);
		}
		#elif (GSBP_SETUP__UART_RX_METHOD == 2)	// DMA
		if (HAL_UART_Receive_DMA(Handle->UART_Handle, Handle->RxBuffer, GSBP_SETUP__RX_BUFFER_SIZE) != HAL_OK){
			MCU_ErrorHandler(EH_UART_InitError);
		}
		#else
		#error "Unsupported GSBP_SETUP__UART_RX_METHOD"
		#endif
	}
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
	// USB interface
	if (Handle->InterfaceType == GSBP_InterfaceUSB){
	    // set the UART handle pointer
		Handle->UART_Handle = NULL;
		// TODO
	}
#endif

	// handle the default handle
	if (Handle->State & GSBP_HandleState__DefaultHandle){
		GSBP_SetDefaultHandle(Handle);
	}
}

// remove the given handle and make sure the handle list is in order
void GSBP_DeInitHandle(GSBP_Handle_t *Handle)
{
	if (Handle->State & GSBP_HandleState__DefaultHandle){
		GSBP_Handles.DefaultHandle = 0;
	}
	if (GSBP_Handles.ActiveHandles > 0) {
		for (uint8_t i=Handle->HandleListIndex; i< GSBP_Handles.ActiveHandles; i++){
			if (GSBP_Handles.Handles[i]->UART_Handle != NULL){
				// move all handles up in case this handle is not the last
				GSBP_Handles.Handles[i] = GSBP_Handles.Handles[i+1];
			}
		}
		// set the last handle to NULL even it should already be NULL
		GSBP_Handles.Handles[GSBP_Handles.ActiveHandles -1] = NULL;
		// count down the active handles
		GSBP_Handles.ActiveHandles--;
	}
	// update the state
	Handle->State &= ~GSBP_HandleState__HandleEnabled;
	Handle->State |=  GSBP_HandleState__HandleDisabled;

#if (GSBP_SETUP__INTERFACE_UART_USED)
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
#endif
}

// update the default handle to the given handle
uint8_t GSBP_SetDefaultHandle(GSBP_Handle_t *Handle)
{
	// initial checks
	if (!(Handle->State  & GSBP_HandleState__HandleEnabled)){
		// error: the handle is not active
		return 0;
	}

	// search for the current default handle
	if (GSBP_Handles.ActiveHandles > 0) {
		for (uint8_t i=0; i< GSBP_Handles.ActiveHandles; i++){
			if (GSBP_Handles.Handles[i]->State & GSBP_HandleState__DefaultHandle){
				GSBP_Handles.Handles[i]->State &= ~GSBP_HandleState__DefaultHandle;
			}
		}
	} else {
		// error: there should be active handles
		return 0;
	}

	GSBP_Handles.DefaultHandle = Handle;
	Handle->State |= GSBP_HandleState__DefaultHandle;
	return 1;
}



/***
 * ### GSBP "public" API functions ###
 *
 * functions for receiving packages
 ***/

// GSBP receive function for polling -> works probally not reliable
uint16_t GSBP_GetNBytes(GSBP_Handle_t *Handle, uint16_t N_Bytes, uint16_t TimeOut)
{
	memset(Handle->RxBuffer, 0, sizeof(Handle->RxBuffer));
	HAL_UART_Receive(Handle->UART_Handle, Handle->RxBuffer, N_Bytes, (uint32_t)TimeOut);
	memcpy(&Handle->RxTempBuffer[Handle->RxTempSize], Handle->RxBuffer, strlen((char*)Handle->RxBuffer));
	Handle->RxTempSize += strlen((char*)Handle->RxBuffer);
	return Handle->RxTempSize;
}

// clear the RX buffer of that handle
void GSBP_ClearBuffer(GSBP_Handle_t *Handle)
{
    memset(Handle->RxBuffer, 0, GSBP_SETUP__RX_BUFFER_SIZE);
    Handle->RxBufferSize = 0;
    memset(Handle->RxTempBuffer, 0, GSBP_RX_TEMP_BUFFER_SIZE);
    Handle->RxTempSize = 0;
}

// copy the bytes received by that handle to a second buffer for later inspection
uint8_t GSBP_SaveBuffer(GSBP_Handle_t *Handle)
{
	// run only if the handle is enabled
	if (Handle->State & GSBP_HandleState__HandleDisabled){
		return 0;
	}

	int16_t NumberOfBytesToTransfer = 0;

    // TODO: prüfen, was passiert, wenn daten ankommen, während diese funktion ausgeführt wird  -> HAL_Delay einfügen ...
	// TODO: bei packeten von mehreren Interfaces, muss sichergestellt sien, dass die Packete vollständig sind, da sonnst die Bytereihenfole in RxBufferTemp nicht mehr stimmt....
    #if SERIAL_DEBUG_LEVEL >= 1
        SerialDebug_High(Serial_D1);
    #endif

#if (GSBP_SETUP__INTERFACE_UART_USED)
    // UART Interface
    if (Handle->InterfaceType == GSBP_InterfaceUART){
#if (GSBP_SETUP__UART_RX_METHOD == 0)	// Polling
        // get the header Bytes from the interface and the the payload
    	// works, but not really reliable
    	convert_uint16_t sizePayload;
    	HAL_StatusTypeDef ret = HAL_UART_Receive(Handle->UART_Handle, &Handle->RxTempBuffer[Handle->RxTempSize], (uint16_t)GSBP__PACKAGE_SIZE_HEADER, (uint32_t)GSBP_SETUP__UART_RX_POLLING_TIMEOUT);
    	if (ret == HAL_OK){
    		sizePayload.c_data[0] = Handle->RxTempBuffer[Handle->RxTempSize +3];
    		sizePayload.c_data[1] = Handle->RxTempBuffer[Handle->RxTempSize +4];
    		ret = HAL_UART_Receive(Handle->UART_Handle, &Handle->RxTempBuffer[Handle->RxTempSize +GSBP__PACKAGE_SIZE_HEADER], (sizePayload.data +GSBP__PACKAGE_TAIL_SIZE -1), (uint32_t)GSBP_SETUP__UART_RX_POLLING_TIMEOUT);
    		if (ret == HAL_OK){
    			Handle->RxTempSize += sizePayload.data +GSBP__PACKAGE_SIZE_HEADER +GSBP__PACKAGE_TAIL_SIZE;
    		}
    	}
#elif (GSBP_SETUP__UART_RX_METHOD == 1)	// IT
    	if (Handle->UART_Handle->RxXferSize != Handle->UART_Handle->RxXferCount){
        	__HAL_LOCK(Handle->UART_Handle);
        	NumberOfBytesToTransfer = Handle->UART_Handle->RxXferSize - Handle->UART_Handle->RxXferCount;
        	// copy N bytes
        	memcpy(&Handle->RxTempBuffer[Handle->RxTempSize], Handle->RxBuffer, NumberOfBytesToTransfer );
        	Handle->RxTempSize += NumberOfBytesToTransfer;
        	// Reset the buffer
    		Handle->UART_Handle->pRxBuffPtr = Handle->RxBuffer;
        	Handle->UART_Handle->RxXferCount = Handle->UART_Handle->RxXferSize;
        	__HAL_UNLOCK(Handle->UART_Handle);
    	}
#elif (GSBP_SETUP__UART_RX_METHOD == 2)	// DMA
#ifdef STM32L432xx
    	if ((Handle->RxBufferIndex2 = Handle->UART_Handle->RxXferSize -Handle->UART_Handle->hdmarx->Instance->CNDTR) != Handle->RxBufferIndex1){
#else
    	if ((Handle->RxBufferIndex2 = Handle->UART_Handle->RxXferSize -Handle->UART_Handle->hdmarx->Instance->NDTR) != Handle->RxBufferIndex1){
#endif
    		// save buffer
    		if (Handle->RxBufferIndex2 < Handle->RxBufferIndex1){
    			// the circular buffer was reseted -> 2 copy operations
    			// copy the data from position x to the end of the buffer
    			NumberOfBytesToTransfer = Handle->UART_Handle->RxXferSize -Handle->RxBufferIndex1;
    			memcpy(&Handle->RxTempBuffer[Handle->RxTempSize], &Handle->RxBuffer[Handle->RxBufferIndex1], NumberOfBytesToTransfer);
    			Handle->RxTempSize += NumberOfBytesToTransfer;
    			// copy the data from the start of the buffer until position y
    			memcpy(&Handle->RxTempBuffer[Handle->RxTempSize], &Handle->RxBuffer[0], Handle->RxBufferIndex2);
    			Handle->RxTempSize += Handle->RxBufferIndex2;
    		} else {
    			// the circular buffer was NOT reseted -> 1 copy operations
    			// copy the data from position x to position y
    			NumberOfBytesToTransfer = Handle->RxBufferIndex2 -Handle->RxBufferIndex1;
    			memcpy(&Handle->RxTempBuffer[Handle->RxTempSize], &Handle->RxBuffer[Handle->RxBufferIndex1], NumberOfBytesToTransfer);
    			Handle->RxTempSize += NumberOfBytesToTransfer;
    		}
    		Handle->RxBufferIndex1 = Handle->RxBufferIndex2;
    		// resets are UNNECESSARY because the DMA has to be configured in circular mode -> so it will go back automatically
    	}
#else
#error "Unsupported GSBP_SETUP__UART_RX_METHOD"
#endif
    }
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
    // USB Interface
    if (Handle->InterfaceType == GSBP_InterfaceUSB) {
    	if (Handle->RxBufferSize != 0){
    		NumberOfBytesToTransfer = Handle->RxBufferSize;
    		// copy N bytes
    		memcpy(&Handle->RxTempBuffer[Handle->RxTempSize], Handle->RxBuffer, NumberOfBytesToTransfer );
    		Handle->RxTempSize += NumberOfBytesToTransfer;
    		// Reset the buffer
    		Handle->RxBufferSize = 0;
    	}
    }
#endif

#if SERIAL_DEBUG_LEVEL >= 1
    SerialDebug_Low(Serial_D1);
#endif

    return Handle->RxTempSize;
}

// build the package struct from a byte stream save in the handle temp buffer
uint8_t GSBP_BuildPackage(GSBP_Handle_t *Handle, gsbp_PackageRX_t *Package)
{
	uint16_t    RxPackageStartIndex = 0, RxPackageEndIndex = 0;
	uint16_t    RxTempBufferSize = 0;
	uint8_t     PackageNumberLocalTemp = 0;
	uint32_t    ChecksumDataTemp = 0;
	uint16_t    i,j;

	GSBP_BuildPackageCallback(Handle);

	// search the start byte TODO: what to do with the first bytes???
	for (i = 0; i < Handle->RxTempSize; i++){
		if (Handle->RxTempBuffer[i] == GSBP__PACKAGE_START_BYTE){
			RxPackageStartIndex = i + 1; // the byte after the start byte is the first header byte
			break;
		}
	}

	if (RxPackageStartIndex == 0){
		// no start byte found
		Handle->RxTempSize = 0;
		return 0;   // False TODO TRUE/FALSE
	}

	// check the checksum TODO: verschiede Paketstrukturen berücksichtigen
	if (Handle->RxTempBuffer[RxPackageStartIndex + 4] != GSBP_GetHeaderChecksum(&Handle->RxTempBuffer[RxPackageStartIndex]) ){
		// checksum does NOT match
		GSBP_SendError(GSBP_Handles.DefaultHandle, E_ChecksumMissmatch); // TODO: sollten Fehler bei BuildPAckage nicht an die Schnittstelle gehen, von der die Bytes kommen? -> Liste mit Quelle der Bytes -> SAveBuffer darf nur komplette Packete in RxTemp schreiben
		Handle->RxTempBuffer[RxPackageStartIndex -1] = 0x00; // clear the GSBP start byte -> masking this package
		//Handle->RxTempSize = 0;
		return 0;	// False TODO
	}
	RxPackageEndIndex = RxPackageStartIndex;
	Package->CommandID 		= (uint16_t) Handle->RxTempBuffer[RxPackageEndIndex++];
	PackageNumberLocalTemp 	= Handle->RxTempBuffer[RxPackageEndIndex++];
	// TODO Check the package numbers

	convert_uint16_t temp;
	temp.c_data[1] = Handle->RxTempBuffer[RxPackageEndIndex++];
	temp.c_data[0] = Handle->RxTempBuffer[RxPackageEndIndex++];
	Package->DataSize = (uint16_t) temp.data;
	// checksum
	RxPackageEndIndex++;

	// check the data length
	if (Handle->RxTempSize >= (RxPackageEndIndex + 1)) {
		// + 1 END Byte -> no payload?
		if (Package->DataSize == 0) {
			if (Handle->RxTempBuffer[RxPackageEndIndex] == GSBP__PACKAGE_END_BYTE) {
				// ### no payload -> we are done. ###
				Handle->RxTempBuffer[RxPackageStartIndex -1] = 0x00; // clear the GSBP start byte -> masking this package
				//Handle->RxTempSize = 0;
				return 1; // TRUE
			} else {
				GSBP_SendError(GSBP_Handles.DefaultHandle, E_EndByteMissmatch);
				Handle->RxTempBuffer[RxPackageStartIndex -1] = 0x00; // clear the GSBP start byte -> masking this package
				//Handle->RxTempSize = 0;
				return 0; // FALSE
			}
		} else {
			// we have a payload -> check the length
			if (Handle->RxTempSize >= (RxPackageEndIndex + Package->DataSize + GSBP__PACKAGE_TAIL_SIZE)) {
				// the size match -> copy the data
				memcpy(Package->Data, &(Handle->RxTempBuffer[RxPackageEndIndex]), Package->DataSize);
				// get the data checksum
				// TODO
				// check the checksum
				// TODO
				// update the size counter
				RxPackageEndIndex += Package->DataSize;   // data
				RxPackageEndIndex++;                      // checksum

				// Check the END byte
				if (Handle->RxTempBuffer[RxPackageEndIndex++] != GSBP__PACKAGE_END_BYTE) {
					// size does not match -> ERROR
					GSBP_SendError(GSBP_Handles.DefaultHandle, E_EndByteMissmatch);
					Handle->RxTempBuffer[RxPackageStartIndex -1] = 0x00; // clear the GSBP start byte -> masking this package
					//Handle->RxTempSize = 0;
					return 0; // FALSE
				} else {
					// we are done.
					Handle->RxTempBuffer[RxPackageStartIndex -1] = 0x00; // clear the GSBP start byte -> masking this package
					//Handle->RxTempSize = 0;
					return 1; // TRUE
				}
			} else {
				// size does not match -> ERROR
				/*
				char temp[500] = {0};
				sprintf(temp, "StatusDMA: %u, RxTempSize: %u, StartIndex: %u, EndIndex: %u, RxBufferSize: %u, Package Size: %u -> Buffer: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
						Handle->UART_Handle->hdmarx->State, Handle->RxTempSize, RxPackageStartIndex, RxPackageEndIndex, RxPackageEndIndex -RxPackageStartIndex, Package->DataSize,
						Handle->RxTempBuffer[RxPackageStartIndex +0], Handle->RxTempBuffer[RxPackageStartIndex +1], Handle->RxTempBuffer[RxPackageStartIndex +2], Handle->RxTempBuffer[RxPackageStartIndex +3],
						Handle->RxTempBuffer[RxPackageStartIndex +4], Handle->RxTempBuffer[RxPackageStartIndex +5], Handle->RxTempBuffer[RxPackageStartIndex +6], Handle->RxTempBuffer[RxPackageStartIndex +7]);
				GSBP_SendErrorParMsg(GSBP_Handles.DefaultHandle, E_UARTSizeMissmatch, NULL, 0, temp);
				*/
				//TODO better error handling
				// maybe the package was not yet completely received ...
				//Handle->RxTempBuffer[RxPackageStartIndex -1] = 0x00; // clear the GSBP start byte -> masking this package
				//Handle->RxTempSize = 0;
				return 0; // FALSE
			}
		}
	}
	//Handle->RxTempSize = 0;
	return 0;
}



/***
 * ### GSBP "public" API functions ###
 *
 * functions for sending packages
 ***/

// Sends in format:
//Start, command, Pack Num, data size high, data size low, checksum, data ... , 0 byte (checksum 2?), End. Total Bytes = DataSize + 8

uint8_t GSBP_SendPackage(GSBP_Handle_t *Handle, gsbp_PackageTX_t *Package)
{
	if (Handle == NULL){
		return 0;
	}
    if (Handle->State & GSBP_HandleState__HandleDisabled){
    	return 0;
    }

    uint16_t TxBufferSize_temp = 0;
    uint32_t uartTimeout = 0;

	// wait until the last  transfer is done
#if (GSBP_SETUP__INTERFACE_UART_USED)
    // UART interface
    if (Handle->InterfaceType == GSBP_InterfaceUART){
    	uartTimeout = HAL_GetTick() +GSBP_SETUP__UART_TX_SEND_TIMEOUT;
    	while (Handle->UART_Handle->gState & HAL_UART_STATE__TX_ACTIVE) {  // TODO ERROR, falls RX = DMA !!!! Flag richtig?
    		// Timeout after N ms
    		if (HAL_GetTick() >= uartTimeout) {
    			break;
    		}
    	}
    }
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
    // USB interface
    if (Handle->InterfaceType == GSBP_InterfaceUSB){
    	// TODO
    }
#endif

    /*
     * build the TxBuffer
     */
    TxBufferSize_temp = 0;
    // build the HEADER
    Handle->TxBuffer[Handle->TxBufferSize + TxBufferSize_temp++] = GSBP__PACKAGE_START_BYTE;
    Handle->TxBuffer[Handle->TxBufferSize + TxBufferSize_temp++] = __MakeUChar(Package->CommandID);
    Handle->TxBuffer[Handle->TxBufferSize + TxBufferSize_temp++] = GSBP_GetPackageNumber(Handle, 1);
    convert_uint16_t temp;
    temp.data = (uint16_t) Package->DataSize;
    Handle->TxBuffer[Handle->TxBufferSize + TxBufferSize_temp++] = temp.c_data[1];
    Handle->TxBuffer[Handle->TxBufferSize + TxBufferSize_temp++] = temp.c_data[0];
    // header checksum
    Handle->TxBuffer[Handle->TxBufferSize + TxBufferSize_temp++] = GSBP_GetHeaderChecksum(&Handle->TxBuffer[Handle->TxBufferSize+1]);

    // build the PAYLOAD
    if (Package->DataSize > 0) {
        // SET DATA
        memcpy(&(Handle->TxBuffer[Handle->TxBufferSize + TxBufferSize_temp]), Package->Data, Package->DataSize);
        TxBufferSize_temp += Package->DataSize;

        // SET Checksum Data
        //Handle->TxBuffer[Handle->TxBufferSize + TxBufferSize_temp++] = 0x00; // TODO add Checksum
        Handle->TxBuffer[Handle->TxBufferSize + TxBufferSize_temp++] = GSBP_GetDataChecksum(Package->Data, Package->DataSize); // TODO add Checksum
    }
    // build the TAIL
    Handle->TxBuffer[Handle->TxBufferSize + TxBufferSize_temp++] = GSBP__PACKAGE_END_BYTE;

    /*
     * send the command
     */
#if (GSBP_SETUP__INTERFACE_UART_USED)
    // UART Interface
    if (Handle->InterfaceType == GSBP_InterfaceUART){
#if (GSBP_SETUP__UART_TX_METHOD == 0)	// Polling
    	if (HAL_UART_Transmit(Handle->UART_Handle, (uint8_t *) Handle->TxBuffer, (uint16_t) (Handle->TxBufferSize + TxBufferSize_temp), GSBP__UART_TX_TIMEOUT) != HAL_OK) {
#endif

#if (GSBP_SETUP__UART_TX_METHOD == 1)	// IT
 		if (HAL_UART_Transmit_IT(Handle->UART_Handle, (uint8_t *) Handle->TxBuffer, (uint16_t) (Handle->TxBufferSize + TxBufferSize_temp)) != HAL_OK) {
#endif
#if (GSBP_SETUP__UART_TX_METHOD == 2)	// DMA
    	if (HAL_UART_Transmit_DMA(Handle->UART_Handle, (uint8_t *) Handle->TxBuffer, (uint16_t) (Handle->TxBufferSize + TxBufferSize_temp)) != HAL_OK) {
#endif
    		// buffer NOT send -> increase the buffer size
    		Handle->TxBufferSize += TxBufferSize_temp;
    		return 0; // FALSE
    	} else {
    		// buffer send -> set the buffer size to zero
    		Handle->TxBufferSize = 0;
    		return 1; // TRUE
    	}
    }
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
 	// USB Interface
    if (Handle->InterfaceType == GSBP_InterfaceUSB) {
    	if (CDC_Transmit_FS( (uint8_t *) Handle->TxBuffer, (uint16_t) (Handle->TxBufferSize + TxBufferSize_temp)) != USBD_OK) {
    		// buffer NOT send -> increase the buffer size
    		Handle->TxBufferSize += TxBufferSize_temp;
    		return 0; // FALSE
    	} else {
    		// buffer send -> set the buffer size to zero
    		Handle->TxBufferSize = 0;
    		return 1; // TRUE
    	}
    }
#endif
    return 0; // FALSE
}

void    GSBP_SendStatus(GSBP_Handle_t *Handle)
{
	// set status ACK
	GSBP_Handles.ACK.CommandID = StatusACK;
#ifndef APP_MCU_STATES_H_
	GSBP_Handles.ACK.DataSize = 6;
    // MCU status
	GSBP_Handles.ACK.Data[0] = 0;
	GSBP_Handles.ACK.Data[1] = 0;
    // supply voltage
    convert_uint32_t temp;
    temp.data = (uint32_t) (3.333*1000);
    GSBP_Handles.ACK.Data[2] = temp.c_data[0];
    GSBP_Handles.ACK.Data[3] = temp.c_data[1];
    GSBP_Handles.ACK.Data[4] = temp.c_data[2];
    GSBP_Handles.ACK.Data[5] = temp.c_data[3];
#else
    GSBP_Handles.ACK.DataSize = MCU_GetStatusInformation(GSBP_Handles.ACK.Data);
#endif

    if (!GSBP_SendPackage(Handle, &GSBP_Handles.ACK)) {
        while (!GSBP_ReSendPackage(Handle)) {
            HAL_Delay(300);
        }
    }
}

void GSBP_SendDebugPackage(GSBP_Handle_t *Handle, uint8_t *Data)
{
	uint16_t BytesToSend = strlen((char*)Data);
	if (BytesToSend > GSBP_SETUP__MAX_PAYLOAD_SIZE_TX){
		BytesToSend = GSBP_SETUP__MAX_PAYLOAD_SIZE_TX;
	}
	GSBP_Handles.ACK.CommandID = DebugCMD;
	memcpy(GSBP_Handles.ACK.Data, Data, BytesToSend);
	GSBP_Handles.ACK.DataSize = BytesToSend;
	GSBP_SendPackage(Handle, &GSBP_Handles.ACK);
}


void GSBP_SendErrorParMsg(GSBP_Handle_t *Handle, uint8_t ErrorCode, uint8_t* Paramters, uint8_t NumberOfParameters, const char* ErrorMessage)
{
#define GSBP__PACKAGE__SEND_ERROR__NUMBER_OF_FIXED_PARAMETER	5
	GSBP_Handles.ACK.CommandID = ErrorCMD;

	// fixed parameter
	GSBP_Handles.ACK.Data[0] = ErrorCode;
	GSBP_Handles.ACK.Data[1] = NumberOfParameters;

#if (GSBP_SETUP__USE_MCU_STATES ==1)
	GSBP_Handles.ACK.Data[2] = MCU_State_CurrentState;
	GSBP_Handles.ACK.Data[3] = (MCU_State_SubSystemStates >> 8) & 0xFF;
	GSBP_Handles.ACK.Data[4] = MCU_State_SubSystemStates & 0xFF;
#else
	GSBP_Handles.ACK.Data[2] = 0x00;
	GSBP_Handles.ACK.Data[3] = 0x00;
	GSBP_Handles.ACK.Data[4] = 0x00;
#endif

	// dynamic parameters
	for(uint8_t i = 0; i<NumberOfParameters; i++){
		GSBP_Handles.ACK.Data[GSBP__PACKAGE__SEND_ERROR__NUMBER_OF_FIXED_PARAMETER +i] = Paramters[i];
	}

	size_t ErrorMessageLength = strlen(ErrorMessage);
	if (ErrorMessageLength > 0) {
		if ( (ErrorMessageLength + GSBP__PACKAGE__SEND_ERROR__NUMBER_OF_FIXED_PARAMETER + GSBP__PACKAGE_OVERHEAD) > GSBP_SETUP__TX_BUFFER_SIZE ){
			ErrorMessageLength = GSBP_SETUP__TX_BUFFER_SIZE - (GSBP__PACKAGE__SEND_ERROR__NUMBER_OF_FIXED_PARAMETER + GSBP__PACKAGE_OVERHEAD);
		}
		memcpy(&GSBP_Handles.ACK.Data[GSBP__PACKAGE__SEND_ERROR__NUMBER_OF_FIXED_PARAMETER +NumberOfParameters], ErrorMessage, ErrorMessageLength);
	}
	GSBP_Handles.ACK.DataSize = GSBP__PACKAGE__SEND_ERROR__NUMBER_OF_FIXED_PARAMETER + NumberOfParameters + (uint16_t)ErrorMessageLength;

    if (!GSBP_SendPackage(Handle, &GSBP_Handles.ACK)){
		do {
            HAL_Delay(300);
        } while (!GSBP_ReSendPackage(Handle));
    }
    HAL_Delay(5);
}


/*
 * ReSend Command
 */
uint8_t GSBP_ReSendPackage(GSBP_Handle_t *Handle)
{
    if (Handle->TxBufferSize == 0) {
        return 1; // TRUE
    }

#if (GSBP_SETUP__INTERFACE_UART_USED)
    // UART Interface
    if (Handle->InterfaceType == GSBP_InterfaceUART){
#if (GSBP_SETUP__UART_TX_METHOD == 0)	// Polling
    	if (HAL_UART_Transmit(Handle->UART_Handle, (uint8_t *) Handle->TxBuffer, (uint16_t) Handle->TxBufferSize, GSBP__UART_TX_TIMEOUT) == HAL_OK) {
#endif
#if (GSBP_SETUP__UART_TX_METHOD == 1)	// IT
 		if (HAL_UART_Transmit_IT(Handle->UART_Handle, (uint8_t *) Handle->TxBuffer, (uint16_t) Handle->TxBufferSize) == HAL_OK) {
#endif
#if (GSBP_SETUP__UART_TX_METHOD == 2)	// DMA
 		if (HAL_UART_Transmit_DMA(Handle->UART_Handle, (uint8_t *) Handle->TxBuffer, (uint16_t) Handle->TxBufferSize) == HAL_OK) {
#endif
 			// buffer send -> set the buffer size to zero
 			Handle->TxBufferSize = 0;
 			return 1; // TRUE
 		}
    }
#endif

#if (GSBP_SETUP__INTERFACE_USB_USED)
 	// USB Interface
 	if (Handle->InterfaceType == GSBP_InterfaceUSB) {
 		if (CDC_Transmit_FS( (uint8_t *) Handle->TxBuffer, (uint16_t) (Handle->TxBufferSize)) == USBD_OK) {
 			// buffer send -> set the buffer size to zero
 			Handle->TxBufferSize = 0;
 		    return 1; // TRUE
 		}
 	}
#endif
    return 0; // FALSE
}

__weak void GSBP_BuildPackageCallback(GSBP_Handle_t *Handle)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(Handle);
}

/***
 * ### GSBP "private" functions ###
 *
 * helper functions for GSBP
 ***/

// get the local package number
uint8_t GSBP_GetPackageNumber(GSBP_Handle_t *Handle, uint8_t Increment)
{
    if (Increment){
        // reset the 8 bit counter
        if (Handle->RxPackageNumber >= GSBP__PACKAGE_LOCAL_NUMBER_MAX){
            // set to 0 because 0 is never used
        	Handle->RxPackageNumber = 0;
        }
        Handle->RxPackageNumber++;
    }
    return Handle->RxPackageNumber;
}

// get the header checksum for the given buffer
uint8_t GSBP_GetHeaderChecksum(uint8_t *Buffer)
{
    uint8_t i, ChecksumHeaderTemp = GSBP__PACKAGE_HEADER_CHECKSUM_START;
    // TODO andere Paketstrukturen einbeziehen
    for (i = 0; i < 4; i++){
        ChecksumHeaderTemp ^= Buffer[i];
        //TODO Checksumme ist nicht gut -> lieber one's sum ....
        //http://betterembsw.blogspot.de/2010/05/which-error-detection-code-should-you.html
    }
    return ChecksumHeaderTemp;
}

// get the data checksum for the given buffer
uint8_t GSBP_GetDataChecksum(uint8_t *Buffer, uint8_t length)
{
	if (length > 50){
		return 0;
	}
	uint8_t i, ChecksumDataTemp = 0x00;
    // TODO andere Paketstrukturen einbeziehen
    for (i = 0; i < length; i++){
        ChecksumDataTemp ^= Buffer[i];
        //TODO Checksumme ist nicht gut -> lieber one's sum ....
        //http://betterembsw.blogspot.de/2010/05/which-error-detection-code-should-you.html
    }
    return ChecksumDataTemp;
}



/***
 *  GSBP HAL ReImplementations
 *  ############################################################################
 ***/

/***
  * @brief  Rx Transfer completed callback
  * @param  UartHandle: UART handle
  * @note   This example shows a simple way to report end of IT Rx transfer, and
  *         you can add your own implementation.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	// IT
#if (GSBP_SETUP__UART_RX_METHOD == 1)	// IT
	//search the right GSBP handle
	GSBP_Handle_t *Handle = NULL;
	uint8_t i = 0;
	for (i=0; i< GSBP_Handles.ActiveHandles; i++){
		if ( (GSBP_Handles.Handles[i]->UART_Handle - huart) == 0 ){
			Handle = GSBP_Handles.Handles[i];
			break;
		}
	}

	if (*Handle != NULL) {
		// save the full buffer
		GSBP_SaveBuffer(Handle);
		if (Handle->InterfaceType == GSBP_InterfaceUART){
			// restart the RX interrupt read in
			if (HAL_UART_Receive_IT(Handle->UART_Handle, Handle->RxBuffer, GSBP_SETUP__RX_BUFFER_SIZE) != HAL_OK){
				MCU_ErrorHandler(EH_GSBP_InitError);
			}
		}
	} else {
		MCU_ErrorHandler(EH_GSBP_HandleError);
	}
#endif

	// DMA
	// DMA should be configured in circular mode -> no implementation needed
	// The callback is still called if the the buffer resets
}

/**
  * @brief  Tx Transfer completed callbacks.
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
/*void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	UNUSED(huart);
}*/

/***
  * @brief  UART error callbacks.
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	//search the right GSBP handle
	GSBP_Handle_t *Handle = NULL;
	uint8_t i = 0;
	for (i=0; i< GSBP_Handles.ActiveHandles; i++){
		if ( (GSBP_Handles.Handles[i]->UART_Handle - huart) == 0 ){
			Handle = GSBP_Handles.Handles[i];
			break;
		}
	}

	if (huart->ErrorCode == HAL_UART_ERROR_ORE){
		// remove the error condition
		huart->ErrorCode = HAL_UART_ERROR_NONE;
		// set the correct state, so that the UART_RX_IT works correctly
		huart->gState = HAL_UART_STATE_BUSY_RX;
	}

	if ((void*)Handle != NULL) {
		// set the flag
		Handle->State |= GSBP_HandleState__ReceiveError;
	} else {
		MCU_ErrorHandler(EH_GSBP_HandleError);
	}
}

