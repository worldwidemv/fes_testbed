/*
 * GeneralSerialByteProtocol_Definitions.h
 *
 *  Created on: 16.05.2016
 *      Author: valtin
 */

#ifndef APP_GSBP_BASIC_H_
#define APP_GSBP_BASIC_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "GSBP_Basic_Config.h"
#ifdef APP_GSBP_BASIC_CONFIG_H_

#define GSBP_RX_TEMP_BUFFER_OVERSIZE            30
#define GSBP_RX_TEMP_BUFFER_SIZE                (GSBP_SETUP__RX_BUFFER_SIZE+GSBP_RX_TEMP_BUFFER_OVERSIZE)

// Commands

#define GSBP__PACKAGE_START_BYTE                0x7E
#define GSBP__PACKAGE_END_BYTE                  0x81
#define GSBP__PACKAGE_HEADER_CHECKSUM_START     GSBP__PACKAGE_START_BYTE
#define GSBP__PACKAGE_SIZE_HEADER				6
#define GSBP__PACKAGE_SIZE_MIN					7 // TODO berechnen, aus den unterschiedlichen Packetstrukturen....
#define GSBP__PACKAGE_DATA_CHECKSUM_SIZE        1
#define GSBP__PACKAGE_TAIL_SIZE            		(GSBP__PACKAGE_DATA_CHECKSUM_SIZE + 1)  // x + stop
#define GSBP__PACKAGE_LOCAL_NUMBER_MAX          255
#define GSBP__PACKAGE_OVERHEAD					15 // TODO aus dem vorherigen größen berechnen

#define GSBP__UART_TX_TIMEOUT					10

// HAL Extensions
#define HAL_UART_STATE__TX_ACTIVE				0x01U
#define HAL_UART_STATE__RX_ACTIVE				0x02U
#define HAL_UART_STATE__TX_RX_ACTIVE			0x03U

// Serial Debug
#define SERIAL_DEBUG_LEVEL				 		1			// 4 = UART DEBUG on D15

#ifndef APP_MCU_DEBUG_H_
	// empty defines, so the debug code can stay inside of the code
	#define SerialDebug_High(X)
	#define SerialDebug_Low(X)
	#define SerialDebug_Toggle(X)
#else
	#define Serial_D1							D6
	#define Serial_D2							D7
	#define Serial_D3							D8
	#define Serial_D4							D9
	#if  SERIAL_DEBUG_LEVEL > 0
		#define SerialDebug_High(X)				Debug_High(X)
		#define SerialDebug_Low(X)				Debug_Low(X)
		#define SerialDebug_Toggle(X)			Debug_Toogle(X)
	#else
		// empty defines, so the debug code can stay inside of the code
		#define SerialDebug_High(X)
		#define SerialDebug_Low(X)
		#define SerialDebug_Toggle(X)
	#endif
#endif


#ifndef APP_MCU_ERRORHANDLER_H_
	// empty defines, so the debug code can stay inside of the code
	#define MCU_ErrorHandler(X)
	typedef enum {
		EH_GSBP_InitError,
		EH_GSBP_HandleError,
	} gsbp_MCUErrorStates_t;

#else
	#include "MCU_ErrorHandler.h"
#endif


#if (GSBP_SETUP__USE_MCU_STATES ==1)
	extern volatile uint8_t  MCU_State_CurrentState;
	extern volatile uint16_t MCU_State_SubSystemStates;
#endif
// Functional defines
#define __MakeUChar(X)                          (unsigned char)(X & 0x0000FF)

//Typedefs
typedef union {
    uint8_t c_data[2];
    uint16_t data;
} convert_uint16_t;

typedef union {
    uint8_t c_data[4];
    uint32_t data;
} convert_uint32_t;

typedef enum {
	GSBP_InterfaceUSB,
	GSBP_InterfaceUART,
} gsbp_InterfaceType_t;

typedef enum {
	GSBP_InterfaceEnable,
	GSBP_InterfaceDisable,
	GSBP_InterfaceReset_Interface_1,
	GSBP_InterfaceReset_Interface_2,
	GSBP_InterfaceReset_Interface_3,
	GSBP_InterfaceReset_Interface_4,
	GSBP_InterfaceReset_Interface_5,
	GSBP_InterfaceReset_Interface_6,
	GSBP_InterfaceReset_Interface_7,
	GSBP_InterfaceReset_Interface_8,
} gsbp_InterfaceAction;

typedef enum {
	GSBP_Interface_1				= 0b00000001,
	GSBP_Interface_2				= 0b00000010,
	GSBP_Interface_3				= 0b00000100,
	GSBP_Interface_4				= 0b00001000,
	GSBP_Interface_5				= 0b00010000,
	GSBP_Interface_6				= 0b00100000,
	GSBP_Interface_7				= 0b01000000,
	GSBP_Interface_8				= 0b10000000,
	GSBP_NoInterface				= 0b00000000,
} gsbp_Interface_t;


typedef struct gsbp_PackageRX_t {
    uint16_t    CommandID;                      // CMD/ACK ID; MANDATORY
    uint8_t     PackageNumberLocal;             // raw package number (8 bit); info
    uint8_t     Data[GSBP_SETUP__MAX_PAYLOAD_SIZE_RX]; // user payload; MANDATORY
    uint16_t    DataSize;                       // user payload size; MANDATORY
    //uint8_t     ChecksumHeader;                 // header checksum; info
    //uint32_t    ChecksumData;                   // data checksum; info
} gsbp_PackageRX_t;

typedef struct gsbp_PackageTX_t {
    uint16_t    CommandID;                      // CMD/ACK ID; MANDATORY
    uint8_t     PackageNumberLocal;             // raw package number (8 bit); info
    uint8_t     Data[GSBP_SETUP__MAX_PAYLOAD_SIZE_TX]; // user payload; MANDATORY
    uint16_t    DataSize;                       // user payload size; MANDATORY
    //uint8_t     ChecksumHeader;                 // header checksum; info
    //uint32_t    ChecksumData;                   // data checksum; info
} gsbp_PackageTX_t;

typedef enum {
	GSBP_HandleState__HandleDisabled		= 0b00000001,
	GSBP_HandleState__HandleEnabled			= 0b00000010,
	GSBP_HandleState__ReceiveError			= 0b00001000,
	GSBP_HandleState__DefaultHandle			= 0b10000000,
} gsbp_HandleState_t;

typedef struct {
	volatile uint8_t   		State;
	gsbp_Interface_t    	Interface;
	uint8_t					HandleListIndex;
	gsbp_InterfaceType_t 	InterfaceType;
	UART_HandleTypeDef 		*UART_Handle; /** Pointer to the UART/BT/USB handle */

	// UART Receive Buffer
	uint8_t 			RxPackageNumber;
	uint8_t 			RxBuffer[GSBP_SETUP__RX_BUFFER_SIZE];
	int16_t				RxBufferSize;			// used by USB to track the buffer size
	uint16_t 			RxBufferIndex1;			// used by saveBuffer UART DMA to track the new memory portion
	uint16_t 			RxBufferIndex2;			// used by saveBuffer UART DMA to track the new memory portion
	uint8_t 			RxTempBuffer[GSBP_RX_TEMP_BUFFER_SIZE];
	uint16_t 			RxTempSize;				// used the function saveBuffer and buildPackage to track the size of the TempBuffer


	// UART Transmit Buffer
	uint8_t 			TxPackageNumber; // TODO
	uint8_t 			TxBuffer[GSBP_SETUP__TX_BUFFER_SIZE];
	uint16_t 			TxBufferSize;
} GSBP_Handle_t;

typedef struct {
	// Management Variables
	volatile uint8_t 	DoEnableInterface;
	volatile uint8_t 	DoDisableInterface;
	volatile uint8_t 	InterfacesEnabled;

	GSBP_Handle_t		*Handles[GSBP_SETUP__NUMBER_OF_HANDLES +1];
	GSBP_Handle_t		*DefaultHandle;
	uint8_t				ActiveHandles;

	gsbp_PackageRX_t	CMD;
	gsbp_PackageTX_t	ACK;
} GSBP_HandlesList_t;


/*
 * ### GSBP general global variables ###
 */
extern GSBP_HandlesList_t GSBP_Handles;


/*
 * ### GSBP "public" API functions ###
 *
 * general functions
 */
void    GSBP_InitHandle(GSBP_Handle_t *Handle, UART_HandleTypeDef *huart);
void 	GSBP_DeInitHandle(GSBP_Handle_t *Handle);
uint8_t GSBP_SetDefaultHandle(GSBP_Handle_t *Handle);

/*
 * ### GSBP "public" API functions ###
 *
 * functions for receiving packages
 */
void    GSBP_ClearBuffer(GSBP_Handle_t *Handle);
uint8_t GSBP_SaveBuffer(GSBP_Handle_t *Handle);
uint8_t GSBP_BuildPackage(GSBP_Handle_t *Handle, gsbp_PackageRX_t *Package);


/*
 * ### GSBP "public" API functions ###
 *
 * functions for sending packages
 */
uint8_t GSBP_SendPackage(GSBP_Handle_t *Handle, gsbp_PackageTX_t *Command);

void    GSBP_SendStatus(GSBP_Handle_t *Handle);

void GSBP_SendDebugPackage(GSBP_Handle_t *Handle, uint8_t *Data);

void 	GSBP_SendErrorParMsg(GSBP_Handle_t *Handle, uint8_t ErrorCode, uint8_t* Paramters, uint8_t NumberOfParameters, const char* ErrorMessage);
inline void	GSBP_SendError(GSBP_Handle_t *Handle, uint8_t ErrorCode){
	GSBP_SendErrorParMsg(Handle, ErrorCode, NULL, 0, 0x00);
}
inline void	GSBP_SendErrorPar(GSBP_Handle_t *Handle, uint8_t ErrorCode, uint8_t* Paramters, uint8_t NumberOfParameters){
	GSBP_SendErrorParMsg(Handle, ErrorCode, Paramters, NumberOfParameters, 0x00);
}
inline void	GSBP_SendErrorMsg(GSBP_Handle_t *Handle, uint8_t ErrorCode, const char* ErrorMessage){
	GSBP_SendErrorParMsg(Handle, ErrorCode, NULL, 0, ErrorMessage);
}

uint8_t GSBP_ReSendPackage(GSBP_Handle_t *Handle);
inline uint8_t GSBP_ReSendPackages(void){
	// check all enabled GSBP handles
	uint8_t i = 0, PackagesReSent = 0;

	for (i=0; i< GSBP_Handles.ActiveHandles; i++){
		if (GSBP_Handles.Handles[i]->UART_Handle != NULL){
			// check if a buffer needs to be resent
			if (GSBP_Handles.Handles[i]->TxBufferSize != 0) {
				PackagesReSent += GSBP_ReSendPackage(GSBP_Handles.Handles[i]);
			}
		}
		if (GSBP_Handles.Handles[i]->InterfaceType == GSBP_InterfaceUSB){
			// check if a buffer needs to be resent
			if (GSBP_Handles.Handles[i]->TxBufferSize != 0) {
				PackagesReSent += GSBP_ReSendPackage(GSBP_Handles.Handles[i]);
			}
		}
	}
	return PackagesReSent;
}

__weak void GSBP_BuildPackageCallback(GSBP_Handle_t *Handle);

/*
 * ### GSBP "private" functions ###
 *
 * helper functions for GSBP
 */
uint8_t GSBP_GetPackageNumber(GSBP_Handle_t *Handle, uint8_t Increment);
uint8_t GSBP_GetHeaderChecksum(uint8_t *Buffer);
uint8_t GSBP_GetDataChecksum(uint8_t *Buffer, uint8_t length); // TODO: richtig machen ....

#ifdef __cplusplus
 }
#endif

#endif /* APP_GSBP_BASIC_CONFIG_H_ */
#endif /* APP_GSBP_BASIC_H_ */
