/*
 *      TU Berlin --- Fachgebiet Regelungssystem
 *      Communication Interface for Stimulation Testbed
 *
 *      Author: Markus Valtin
 *
 *      File:           Testbed_Interface.h -> Header file for the communication interface class
 *      Version:        0.1 (10.2018)
 *      Changelog:
 *
 *      SVN Eigenschaften:
 *      $Rev:: 5                                                     $: Revision of last commit
 *      $Author:: valtin                                             $: SVN Author of last commit
 *      $Date:: 2016-05-31 09:05:18 +0200 (Di, 31. Mai 2016)         $: Date of last commit
 */
//TODO:
// - eigenen Printf Funktion?? -> http://www.cplusplus.com/forum/general/6439/
// - Option für Byte stuffing
// - Option für eigene Acks für jedes Kommando / ein Ack mit Datenbereich in dem das Kommando das bestätigt wird drin steht + weitere Payload Daten

#ifndef TESTBED_H
#define TESTBED_H

// TODO: Ausmissten + C++ Header verwenden
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <termios.h>
//#include <linux/serial.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <math.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>  /* Primitive System Data Types */
#include <sys/time.h>
#include <sys/stat.h>


// ### MISC SETUP START ###
#define FALSE                                           0
#define TRUE                                            1

#define GSBP__DEBUG_SENDING_COMMANDS                    FALSE
#define GSBP__DEBUG_RECEIVING_COMMANDS                  FALSE
// ### MISC SETUP STOP  ###
// #############################################################################

// ### GSBP SETUP START ###
#define GSBP__QUEUE_SIZE_UART_PACKAGES                  300                     // number of uart pacakges
#define GSBP__MAX_SIZE_USER_DATA                        30000                    // bytes of the payload

#define GSBP__USE_WATCHDOG_THREAT                       TRUE
#define GSBP__USE_RECEIVING_THREAT                      TRUE
// ### GSBP SETUP STOP  ###
// #############################################################################


//
// GSBP defines
// #############################################################################
#define GSBP__UART_BAUTRATE                             B921600 //B115200 //B2000000 //B3500000
#define GSBP__UART_USE_UART_FLOW_CONTROL                FALSE //TRUE
#define GSBP__UART_MAX_PACKAGE_SIZE                     (GSBP__MAX_SIZE_USER_DATA + 50) // + max 50 byte for the package overhead
#define GSBP__UART_START_BYTE                           0x7E
#define GSBP__UART_END_BYTE                             0x81
#define GSBP__UART_HEADER_CHECKSUM_START                GSBP__UART_START_BYTE
#define GSBP__UART_MAX_LOCAL_PACKAGE_NUMBER             255
#define GSBP__UART_PACKAGE_HEADER_SIZE                  6 // TODO determin this via preprocessor
#define GSBP__UART_DATA_CHECKSUM_SIZE                   1
#define GSBP__UART_PACKAGE_TAIL_SIZE                    (GSBP__UART_DATA_CHECKSUM_SIZE + 1)  // x + stop byte   // TODO determin this via preprocessor
#define GSBP__UART_NUMBER_OF_DATA_BYTES__START_BYTE     3 // TODO determin this via preprocessor

#define GSBP__ACTIVATE_DESTINATION_FEATURE              FALSE
#define GSBP__ACTIVATE_SOURCE_FEATURE                   FALSE
#define GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE       FALSE
#define GSBP__ACTIVATE_16BIT_CMD_FEATURE                FALSE
#define GSBP__ACTIVATE_CONTROL_BYTE_FEATURE             FALSE
#define GSBP__ACTIVATE_16BIT_PACKAGE_LENGHT_FEATURE     TRUE

#define GSBP__ACTIVATE_32BIT_CRC_CHECKSUM               FALSE
#define GSBP__ACTIVATE_16BIT_CRC_CHECKSUM               FALSE
#define GSBP__ACTIVATE_8BIT_CRC_CHECKSUM                FALSE
#define GSBP__ACTIVATE_8BIT_XOR_CHECKSUM                FALSE

#define GSBP__PACKAGE_READ_TIMEOUT_US                   11000    // important timeout to detect  a if not every byte of a package is received and to check if the ReadPackage function should terminate
#define GSBP__PACKAGE__N_SEND_RECEIVE_RETRYS            3
#define GSBP__PACKAGE__N_SEND_RECEIVE_RETRYS_INIT       30
#define GSBP__DEBUG__PP_N_DATA_BYTES_PER_LINE           32       // how many bytes of package payload data should be shown in debug mode (PrintPackage(); format: 0xXX )
// end

#define __MakeUChar(X)                                   (unsigned char)(X & 0x0000FF)

//
// external system specific defines

#define TESTBED_NUMBER_OF_SAMPLES_PER_PULS			65000
#define TESTBED_PULSE_QUEUE_SIZE					1023
#define TESTBED_PULSE_QUEUE_INDEX_MASK				0x03FF
#define TESTBED_PULSE_ENTRY_FIXED_HEADER_SIZE		40

#define TESTBED_STIM_CHANNEL_DETECT_C1				0x01
#define TESTBED_STIM_CHANNEL_DETECT_C2				0x02
#define TESTBED_STIM_CHANNEL_DETECT_C3				0x04
#define TESTBED_STIM_CHANNEL_DETECT_C4				0x08

#define TESTBED_STIM_DATA_PACKAGE_OFFSET			7
#define TESTBED_STIM_CHANNEL_DETECT_MCU_ALL			0x70B3 // all bits set
#define TESTBED_STIM_CHANNEL_DETECT_MCU_P1			0x1000 // bit 13
#define TESTBED_STIM_CHANNEL_DETECT_MCU_P2			0x0080 // bit 8
#define TESTBED_STIM_CHANNEL_DETECT_MCU_P3			0x0010 // bit 5
#define TESTBED_STIM_CHANNEL_DETECT_MCU_P4			0x0020 // bit 6
#define TESTBED_STIM_CHANNEL_DETECT_MCU_N1			0x2000 // bit 14
#define TESTBED_STIM_CHANNEL_DETECT_MCU_N2			0x4000 // bit 15
#define TESTBED_STIM_CHANNEL_DETECT_MCU_N3			0x0001 // bit 1
#define TESTBED_STIM_CHANNEL_DETECT_MCU_N4			0x0002 // bit 2

#define TESTBED_PACKAGE_DATA_OVERRIDEN				0x80
#define TESTBED_PACKAGE_DATA_DISCARDED				0x40
#define TESTBED_PACKAGE_TYPE_MASK					~(TESTBED_PACKAGE_DATA_OVERRIDEN | TESTBED_PACKAGE_DATA_DISCARDED)

#define MAX1119X_RECEIVE_BUFFER_SIZE				5000
#define MAX1119X_SEARCH_WIDTH						4
#define	__packed									__attribute__((__packed__))


namespace ns_Testbed_01 {
    //
    // declaration specific to GSBP and the external system

    // EXT status
    typedef struct {
        struct timeval LastUpdate;
        uint16_t State;
        double  Voltage;
        // project specific implementations
        // ...
    } stateEXT_t;

    // commands and acks
    typedef enum {
        MeasurmentDataACK                    = 100,
		EchoCMD								 = 110,
		EchoACK								 = 111,

        ResetCMD                             = 237,  // TODO
        ResetACK                             = 238,  // TODO
        InitCMD                              = 239,
        InitACK                              = 240,
		DeInitCMD							 = 235,
		DeInitACK							 = 236,
        StartMeasurmentCMD                   = 241,
        StartMeasurmentACK                   = 242,
        StopMeasurmentCMD                    = 243,
        StopMeasurmentACK                    = 244,
        StateCMD                             = 245,
        StateACK                             = 246,
        RepeatLastCommandCMD                 = 247,  // TODO
        RepeatLastCommandACK                 = 248,  // TODO
        DebugCMD                             = 249,
        ErrorCMD                             = 250
    } command_t;

    // errors
    typedef enum {
        GSBP_Error__CMD_NotValidNow          = 11,

        NoDefinedErrorCode                   = 0,
        UnknownCMDError                      = 1,
        ChecksumMissmatchError               = 2,
        EndByteMissmatchError                = 3,
        UARTSizeMissmatchError               = 4,
        MaxErrorsEnumID                      = 12
    } error_t;


    //
    // declarations specific only to GSBP

    // package state
    typedef enum {
        PackageIsBroken                      = 0,
        PackageIsBroken_IncompleteTimout     = 1,
        PackageIsBroken_IncompleteData       = 2,
        PackageIsBroken_InvalidCommandID     = 3,
        PackageIsBroken_StartByteError       = 10,
        PackageIsBroken_EndByteError         = 11,
        PackageIsOk                          = 128
    }  packageState_t;

    // package typ
    typedef enum {
        isCommand = 0,
        isACK     = 1
    } packageTyp_t;

    typedef union {
        uint8_t  c_data[2];
        uint16_t data;
    } convert16_t;

    typedef union {
        uint8_t  c_data[2];
        int16_t data;
    } convert16s_t;

    typedef union {
        uint8_t  c_data[4];
        uint32_t data;
    } convert32_t;

    // package
    typedef struct package_t {
        packageTyp_t    Typ;                                         // CMD or ACK; checks/debug
        uint16_t        CommandID;                                   // CMD/ACK ID;                    TO SET!  TODO: uint8_t als default!
        packageState_t  State;                                       // packet status; checks
        uint8_t         PackageNumberLocal;                          // raw package number (8 bit); info
        uint64_t        PackageNumberGlobal;                         // absolute package number (32 bit); info
        unsigned char   Data[GSBP__MAX_SIZE_USER_DATA];               // user payload;                  TO SET!
        size_t          DataSize;                                    // user payload size;             TO SET!
        uint8_t         ChecksumHeader;                              // header checksum; info
        uint32_t        ChecksumData;                                // data checksum; info
        // additional features, see GSBP configuration
        #if (GSBP__ACTIVATE_DESTINATION_FEATURE == TRUE || GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE == TRUE)
        uint8_t         Destination;                                 // destination; TO SET!
        #endif
        #if (GSBP__ACTIVATE_SOURCE_FEATURE == TRUE || GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE == TRUE)
        uint8_t         Source;                                      // source; TO SET!
        #endif
        #if (GSBP__ACTIVATE_CONTROL_BYTE_FEATURE == TRUE)
        uint8_t         Control;                                     // control byte; TO SET!
        #endif
        // pointer for the queue
        struct package_t *Next;
    } package_t;

    // transmission statistics
    typedef struct {
        uint8_t             LocalRxPackageNumber;
        uint64_t            GlobalRxPackageNumber;
        uint64_t            GlobalRxPackageNumber_Missing;
        uint64_t            GlobalRxPackageNumber_BrokenStructur;
        uint64_t            GlobalRxPackageNumber_BrokenChecksum;

        uint8_t             LocalTxPackageNumber;
        bool                ResetLocalTxPackageNumber;
        uint64_t            GlobalTxPackageNumber;
        uint64_t            GlobalTxPackageNumber_ReSend;
        uint64_t            GlobalTxPackageNumber_ErrorReceived;
        uint64_t            BytesDiscarded;
    } statsGSBP_t;

    /*
     *  EXT Definition
     */

    typedef enum {
    	tb_noData		= 0,
    	tb_constStream 	= 1,
    	tb_stimDataOnly = 2,
    	tb_varStream	= 3
    } testbed_dataTypes_t;
    typedef enum {
    	tb_outputStimDataStructs		= 0,
		tb_outputStreamDataCompressed	= 1,
		tb_outputStreamDataFull			= 2,
    } testbed_dataOutput_t;

    // init variables
    typedef struct {
    	testbed_dataOutput_t	outputType;
    	uint8_t dataPackageType;		// type of data to be send from the MCU
    	uint8_t samplePeriodeMultiplier;// sample rate multiplier for the ADC
    	uint8_t searchWidth;    		// search wide, for detecting the stimulation
    	int16_t thresholdCh1Positiv;
    	int16_t thresholdCh1Negative;
    	int16_t thresholdCh2Positiv;
    	int16_t thresholdCh2Negative;

    	uint8_t demuxEnabled;
    	uint8_t demuxChannelsActive;
    	uint8_t demuxChannelsPassive;
    } initConfig_t;

    typedef struct {
    	initConfig_t config;
    	uint32_t	sampleBufferSize;
    	double		sampleFrequency;
    }testbed_config_t;

    typedef struct {
    	double time;
    	double voltage;
        double current;
        uint64_t    samplesCounter; // TODO remove
    } stimData_t;

    typedef struct {
    	bool        isNew;
    	uint32_t    numberOfSamples;
    	uint64_t    samplesCounter;
    	uint32_t    packageCounter;
    	stimData_t  samples[TESTBED_NUMBER_OF_SAMPLES_PER_PULS];
    } testbed_streamData_t;

    typedef struct __packed {
    	uint8_t  startByte[3];
    	uint16_t numberOfSamples;
    	uint8_t  pulseNotComplete;
    	uint8_t  dataWasOverriden;
    	uint8_t  activeStimChannels_positive;
    	uint8_t  activeStimChannels_negative;
    	uint8_t  activeDemuxChannels_positive[16];
    	uint8_t  activeDemuxChannels_negative[16];
    	bool newData;
    } testbed_stimulationPulseHeader_t;

    typedef struct __packed {
    	testbed_stimulationPulseHeader_t header;
    	uint64_t sampleNumber[TESTBED_NUMBER_OF_SAMPLES_PER_PULS];
    	float time[TESTBED_NUMBER_OF_SAMPLES_PER_PULS];
    	int16_t stimCurrent[TESTBED_NUMBER_OF_SAMPLES_PER_PULS];
    	int16_t stimVoltage[TESTBED_NUMBER_OF_SAMPLES_PER_PULS];
    } testbed_stimulationPulse_t;


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
    	uint32_t 					counter;
    	uint8_t						dataType;
    	uint16_t					numberOfSamples;
    	maxAdc_dataBuffer_t			data[MAX1119X_RECEIVE_BUFFER_SIZE];
    } maxAdc_dataPackage_t;


    typedef struct __packed {
    	uint8_t dataType;	// type of data to be send
    	uint8_t samplePeriodeMultiplier; // sample rate multiplier for the ADC
    	uint8_t searchWidth;    // sample rate divider for stream mode
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
    	uint32_t    packageCounterStart;
    	uint32_t    packageCounter;
    	uint64_t	sampleCounter;
    	bool		watchdogTriggered;

    	bool 		pulseStarted;
    	uint16_t	indexPulsToFillNext;
    	uint16_t	indexPulsToReturnNext;
    	testbed_stimulationPulse_t	pulseQueue[TESTBED_PULSE_QUEUE_SIZE];
    	testbed_streamData_t		streamData;

    	initConfig_t config;
    	uint32_t	sampleBufferSize;
    	double		sampleFrequency;
    } testbed_t;

    class Testbed_Interface
    {
    private:
        /* Private Variables */
        char              ID[255];
        char              SerialDeviceFileName[255];
        int               fd;
        bool              DeviceConnected;
        bool              DeviceWorking;
        bool              DeviceInitialised;
        bool              ReceicePackages;
        bool              ReceicerThreatRunning;

        statsGSBP_t       StatsGSBP;
        uint16_t          lastCMDSend;
        stateEXT_t        StateEXT;

        // UART packagte queue
        package_t         UARTQueue[GSBP__QUEUE_SIZE_UART_PACKAGES];
        uint16_t          UARTQueue_Head, UARTQueue_Tail;
        unsigned char     LastPackageSend[GSBP__UART_MAX_PACKAGE_SIZE];
        size_t            LastPackageSendSize;
        package_t         *QueueHead;
        package_t         *QueueTail;
        uint32_t          QueueSize;

        pthread_t         init_thread;
        pthread_t         receiver_thread;
        pthread_mutex_t   ReadPackage_mutex;
        pthread_mutex_t   QueueLock_mutex;

        testbed_t		  testbed;
        pthread_mutex_t   testbed_mutex;

        /* Private Functions */
        int       OpenDevice(void);
        uint8_t   GetPackageNumberLocal(packageTyp_t Typ, bool Increment);
        uint64_t  GetPackageNumberGlobal(packageTyp_t Typ);
        void*     UARTmalloc( void );
        void      UARTfree( void );

        void      ClearPackage(package_t * Package);
        bool      CheckPackage(package_t *Package);
        void      PrintPackage(package_t *Package);
        bool      SendPackage(package_t *Command);
        bool      ReSendPackage(void);

        #if (GSBP__USE_RECEIVING_THREAT == FALSE)
        bool      ReadPackages(bool doReturnAfterTimeout);
        #endif
        void      BuildPackage(unsigned char *RxBuffer, size_t RxBufferSize, packageState_t State, uint8_t ChecksumHeader);
        void      PushPackage(package_t *Package);

        bool      WaitForPackage(uint8_t CMD, int32_t WaitTimeout_us, uint32_t N_MaxOtherPackages, package_t *Package);
        error_t   PackageHandler_Error(package_t * Package);
        void      PackageHandler_Debug(package_t * Package);

        bool      ext_PackageHandler(package_t *Package);
        void      ext_UpdateState(package_t *Package);
        void      ext_PrintState(void);
        bool      ext_IsCommandIDValid(command_t CMD);
        const char* ext_GetCommandString(command_t CMD);
        const char* ext_GetErrorString(error_t Error);

        bool      ext_PackageHandlerMeasPackage(package_t *Package);
        void      ext_PackageHandlerMeasPackage_StimStruct_NoStimulation(maxAdc_dataPackage_t *data);
        void      ext_PackageHandlerMeasPackage_StimStruct_Stimulation(maxAdc_dataPackage_t *data, size_t dataSize);
        void      ext_PackageHandlerMeasPackage_StreamCompressed_NoStimulation(maxAdc_dataPackage_t *data);
        void      ext_PackageHandlerMeasPackage_StreamCompressed_Stimulation(maxAdc_dataPackage_t *data, size_t dataSize);
        void      ext_PackageHandlerMeasPackage_StreamFull_NoStimulation(maxAdc_dataPackage_t *data);
        void      ext_PackageHandlerMeasPackage_StreamFull_NoStimulationData(maxAdc_dataPackage_t *data);
        void  	  ext_PackageHandlerMeasPackage_StreamFull_Stimulation(maxAdc_dataPackage_t *data, size_t dataSize);

    public:
        /* Public Variables */


        /* Public Functions */
        Testbed_Interface(char *SerialDeviceID, char *SerialDeviceFile);
        ~Testbed_Interface(void);

        bool      isDeviceOpen(void);
        bool      isDeviceWorking(void);
        void      PrintState(void);
        void      PrintStatsGSBP(void);
        #if (GSBP__USE_RECEIVING_THREAT == TRUE)
        bool      ReadPackages( bool doReturnAfterTimeout );
        #endif

        bool      PopPackage(package_t *Package);

        bool      ext_SendInit( initConfig_t *InitConfig );
        bool 	  ext_SendDeInit(void);
        bool      ext_StartMeasurment( uint16_t NumberOfSamplesToGet = 0 );
        bool      ext_StopMeasurment(void);
        bool      ext_GetStimPuls( testbed_stimulationPulse_t *puls );
        bool      ext_GetStreamData( testbed_streamData_t *puls );

        bool	  ext_GetConfig(testbed_config_t *config);
        stateEXT_t    ext_GetState( int32_t WaitTimeout_us );

    };

} // end namespace
#endif

