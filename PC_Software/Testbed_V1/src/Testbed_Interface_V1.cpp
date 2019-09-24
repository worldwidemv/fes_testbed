/*
 *      TU Berlin --- Fachgebiet Regelungssystem
 *      Communication Interface for the Stimulation Testbed
 *
 *      Author: Markus Valtin
 *
 *      File:           Testbed_Interface.cpp -> Source file for the communication interface class
 *      Version:        0.1 (10.2018)
 *      Changelog:
 *
 *      SVN Eigenschaften:
 *      $Rev:: 5                                                     $: Revision of last commit
 *      $Author:: valtin                                             $: SVN Author of last commit
 *      $Date:: 2016-05-31 09:05:18 +0200 (Di, 31. Mai 2016)         $: Date of last commit
 */

#include <Testbed_Interface_V1.h>

namespace ns_Testbed_01 {

// threat's
void *ReceiverThread(void *data);

/*
 * Constructor
 */
Testbed_Interface::Testbed_Interface(char *SerialDeviceID, char *SerialDeviceFile)
{
	// Initialize the variables
	// ID
	memset(this->ID, 0, sizeof(this->ID));
	sprintf(this->ID,"%s",SerialDeviceID);
	// serial device file
	memset(this->SerialDeviceFileName, 0, sizeof(this->SerialDeviceFileName));
	sprintf(this->SerialDeviceFileName,"%s",SerialDeviceFile);

	this->fd = 0;
	// bool's
	this->DeviceConnected = false;
	this->DeviceWorking = false;
	this->DeviceInitialised = false;
	this->ReceicePackages = false;
	this->ReceicerThreatRunning = false;
	// StatsGSBP
	memset(&(this->StatsGSBP), 0, sizeof(this->StatsGSBP));
	// reset the local package number by the first package that arrives
	this->StatsGSBP.ResetLocalTxPackageNumber = true;
	// UART queue
	memset(this->UARTQueue, 0, sizeof(this->UARTQueue));
	memset(this->LastPackageSend, 0, sizeof(this->LastPackageSend));
	this->LastPackageSendSize = 0;
	this->UARTQueue_Head = 0;
	this->UARTQueue_Tail = 0;
	this->QueueHead = NULL;
	this->QueueTail = NULL;
	this->QueueSize = 0;

	memset(&this->testbed, 0, sizeof(testbed_t));

	// Open the character device and use it as tty/virtual com port
	this->fd = Testbed_Interface::OpenDevice();
	if (this->fd < 0){
		// the device could not be opened
		this->DeviceConnected = false;
		printf("e[1me[91m%s ERROR:e[0m Can't open serial port %s:\n   %s (%d)\n",
				this->ID, this->SerialDeviceFileName, strerror(errno), errno);
	} else {
		// file / socket is open
		this->DeviceConnected = true;
		//printf("%s: Opening device %s -> successful!\n", this->ID, this->SerialDeviceFileName);
		// flush the serial data stream
		tcflush(fd, TCIOFLUSH);

#if (GSBP__USE_RECEIVING_THREAT == true)
		// start receiving packages
		this->ReceicePackages = true;
		this->ReceicerThreatRunning = true;
		if (pthread_create(&receiver_thread, NULL, ReceiverThread, (void *)this) != 0) {
			printf("e[1me[91m%s ERROR:e[0m Can't start the receiver threat:\n   %s (%d)\n",
					this->ID, strerror(errno), errno);
			this->ReceicerThreatRunning = false;
		} else {
			this->ReceicerThreatRunning = true;
			//printf("%s: Receiver threat started\n", this->ID);
		}
#endif

		// update the device status
		Testbed_Interface::ext_GetState( 100*1000 );                                 // wait 1000ms
		//Testbed_Interface::PrintState();
	}
	// all done
}

/*
 * Destructor
 */
Testbed_Interface::~Testbed_Interface(void)
{
	// update the device status
	Testbed_Interface::ext_GetState( 50*1000 );                                    // wait 50ms

	// stop receiving packages
	this->ReceicePackages = false;

	// print some statistics
	//Testbed_Interface::PrintState();
	Testbed_Interface::PrintStatsGSBP();

	// clear the queue
	package_t P;
	while(Testbed_Interface::PopPackage(&P)){}

	// close the device
	if (this->DeviceConnected){
#if (GSBP__USE_RECEIVING_THREAT == true)
		// stop receiving packages
		void *ret;
		if (this->ReceicerThreatRunning){
			pthread_join(this->receiver_thread, &ret);
			if (ret < NULL){
				printf("\e[1m\e[91m%s ERROR:\e[0m Can't stop the receiver threat:\n   %s (%d)\n",
						this->ID, strerror(errno), errno);
			}
			this->ReceicerThreatRunning = false;
		}
#endif

		// flush the serial data stream
		int retval;
		tcflush(this->fd, TCIOFLUSH);
		while (retval = close(this->fd), retval == -1 && errno == EINTR) ;
		//printf("%s: Device %s closed\n", this->ID, this->SerialDeviceFileName);
	}

	pthread_mutex_destroy(&this->ReadPackage_mutex);
	pthread_mutex_destroy(&this->QueueLock_mutex);

	pthread_mutex_destroy(&this->testbed_mutex);
}

int Testbed_Interface::OpenDevice()
{
	struct termios ti;
	int fd, ret;

	if (access(this->SerialDeviceFileName, R_OK|W_OK ) < 0 ) {
		// error with the serial interface -> exit
		return -1;
	}

	fd = open(this->SerialDeviceFileName, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		return -1;
	}
	// discarges of not send or not read bytes
	tcflush(fd, TCIOFLUSH);

	/* Test if we can read the serial port device */
	ret = tcgetattr(fd, &ti);
	if (ret < 0)
	{
		close(fd);
		return -1;
	}

	/* Switch serial port to RAW mode */
	// Raw mode
	//
	// cfmakeraw() sets the terminal to something like the "raw" mode of the old Version 7 terminal driver:
	// input is available character by character, echoing is disabled, and all special processing of terminal input and output characters is disabled.
	// The terminal attributes are set as follows:
	// termios_p->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	// termios_p->c_oflag &= ~OPOST;
	// termios_p->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	// termios_p->c_cflag &= ~(CSIZE | PARENB);
	// termios_p->c_cflag |= CS8;
	//memset(&ti, 0, sizeof(ti));
	cfmakeraw(&ti);
	ret = tcsetattr(fd, TCSANOW, &ti);
	if (ret < 0) {
		close(fd);
		return -1;
	}

	// ### http://linux.die.net/man/3/termios ###
	ret = cfsetospeed(&ti,(speed_t)GSBP__UART_BAUTRATE);
	ret = cfsetispeed(&ti,(speed_t)GSBP__UART_BAUTRATE);
	ret = cfsetspeed(&ti,GSBP__UART_BAUTRATE);
	ti.c_cflag |=  CS8 | CLOCAL | CREAD;     // Parity=0; Only one StopBit
#if (GSBP__UART_USE_UART_FLOW_CONTROL == true)
	ti.c_cflag |= CRTSCTS;       // HW HandShake ON
#else
	ti.c_cflag &= ~CRTSCTS;      // HW HandShake OFF
#endif
#if (GSBP__USE_UART_TWO_STOP_BITS == true)
	ti.c_cflag |= CSTOPB;        // 2 Stopbits   ON
#else
	ti.c_cflag &= ~CSTOPB;       // 2 Stopbits   OFF
#endif
	ti.c_iflag |= IGNBRK;
	ti.c_cc[VTIME] = 0;
	ti.c_cc[VMIN]  = 0;

	tcflush(fd, TCIOFLUSH);

	/* Set the new configuration */
	ret = tcsetattr(fd, TCSANOW, &ti);
	if (ret < 0) {
		close(fd);
		return -1;
	}
	return fd;
}

bool Testbed_Interface::isDeviceOpen(void){
	return this->DeviceConnected;
}

bool Testbed_Interface::isDeviceWorking(void){
	return this->DeviceWorking;
}

/*
 * return the 8 bit number for the package number and increment the internal counter if requested
 */
uint8_t Testbed_Interface::GetPackageNumberLocal(packageTyp_t Typ, bool Increment)
{
	uint8_t RetunValue = 0;
	switch (Typ){
	case isCommand:
		if (Increment){
			// reset the 8 bit counter
			if (this->StatsGSBP.LocalTxPackageNumber >= GSBP__UART_MAX_LOCAL_PACKAGE_NUMBER) {
				// set to 0 because 0 is never used
				this->StatsGSBP.LocalTxPackageNumber = 0;
			}
			this->StatsGSBP.LocalTxPackageNumber++;
			this->StatsGSBP.GlobalTxPackageNumber++;
		}
		return this->StatsGSBP.LocalTxPackageNumber;
		break;
	case isACK:
		RetunValue = this->StatsGSBP.LocalRxPackageNumber;
		if (Increment){
			// reset the 8 bit counter
			if (this->StatsGSBP.LocalRxPackageNumber >= GSBP__UART_MAX_LOCAL_PACKAGE_NUMBER) {
				// set to 0 because 0 is never used
				this->StatsGSBP.LocalRxPackageNumber = 0;
			}
			this->StatsGSBP.LocalRxPackageNumber++;
			this->StatsGSBP.GlobalRxPackageNumber++;
		}
		return RetunValue;
		break;
	default:
		// ERROR this should never happen
		printf("e[1me[91m%s ERROR:e[0m GetPackageNumberLocal: Invalid Type '%d'\n", this->ID, (int)Typ);
		return 0;
	}
}

uint64_t Testbed_Interface::GetPackageNumberGlobal(packageTyp_t Typ)
{
	switch (Typ){
	case isCommand:
		return this->StatsGSBP.GlobalTxPackageNumber;
		break;
	case isACK:
		return this->StatsGSBP.GlobalRxPackageNumber;
		break;
	default:
		// ERROR this should never happen
		printf("e[1me[91m%s ERROR:e[0m GetPackageNumberGlobal: Invalid Type '%d'\n", this->ID, (int)Typ);
		return 0;
	}
}

void* Testbed_Interface::UARTmalloc( void )
{
	UARTQueue_Head++;
	if (UARTQueue_Head >= GSBP__QUEUE_SIZE_UART_PACKAGES){
		UARTQueue_Head = 0;                                                       // reset the queue
	}
	if (UARTQueue_Tail == UARTQueue_Head){
		// queue end was overrun -> datalose
		package_t Package;
		Testbed_Interface::PopPackage(&Package);
		printf("\e[1m\e[91m%s Error:\e[0m UART queue is to small! One package was lost!\n", this->ID);
	}
	return &UARTQueue[UARTQueue_Head];
}

void Testbed_Interface::UARTfree( void )
{
	memset(&UARTQueue[UARTQueue_Tail], 0, sizeof(package_t));                    // TODO: optimierung?
	UARTQueue_Tail++;
	if (UARTQueue_Tail >= GSBP__QUEUE_SIZE_UART_PACKAGES){
		UARTQueue_Tail = 0;                                                       // reset the queue
	}
}



/*
 * Send Command
 */
bool Testbed_Interface::SendPackage(package_t *Command)
{
	unsigned char       TxBuffer[GSBP__UART_MAX_PACKAGE_SIZE];
	size_t              TxBufferSize;
	unsigned char       ChecksumHeaderTemp;
	int                 ret;
	unsigned int        i;

	Command->Typ = isCommand;
	// TODO check package
	//Testbed_Interface::CheckPackage(pakage_t *Package)

	// ### build TxBuffer ###
	memset(TxBuffer, 0, sizeof(TxBuffer));
	TxBufferSize = 0;
	// SET HEADER
	TxBuffer[TxBufferSize++] = GSBP__UART_START_BYTE;
#if (GSBP__ACTIVATE_DESTINATION_FEATURE == true)
	TxBuffer[TxBufferSize++] = __MakeUChar(Command->Destination);
#endif
#if (GSBP__ACTIVATE_SOURCE_FEATURE == true)
	TxBuffer[TxBufferSize++] = __MakeUChar(Command->Source);
#endif
#if (GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE == true)
	TxBuffer[TxBufferSize++] = //TODO
#endif
#if (GSBP__ACTIVATE_16BIT_CMD_FEATURE == true)
			TxBuffer[TxBufferSize++] = __MakeUChar((Command->CommandID >> 8));
#endif
	TxBuffer[TxBufferSize++] = __MakeUChar(Command->CommandID);
#if (GSBP__ACTIVATE_CONTROL_FEATURE == true)
	TxBuffer[TxBufferSize++] = __MakeUChar(Command->Control);
#endif
	TxBuffer[TxBufferSize++] = __MakeUChar(Testbed_Interface::GetPackageNumberLocal(isCommand, true));
	Command->PackageNumberLocal  = Testbed_Interface::GetPackageNumberLocal(isCommand, false);
	Command->PackageNumberGlobal = Testbed_Interface::GetPackageNumberGlobal(isCommand);
#if (GSBP__ACTIVATE_16BIT_PACKAGE_LENGHT_FEATURE == true)
	TxBuffer[TxBufferSize++] = __MakeUChar((Command->DataSize >> 8));
#endif
	TxBuffer[TxBufferSize++] = __MakeUChar(Command->DataSize);

	// header checksum
	ChecksumHeaderTemp = GSBP__UART_HEADER_CHECKSUM_START;
	for(i=1; i<TxBufferSize; i++){
		ChecksumHeaderTemp ^= TxBuffer[i];
		//TODO Checksumme ist nicht gut -> lieber one's sum ....
		//http://betterembsw.blogspot.de/2010/05/which-error-detection-code-should-you.html
	}
	TxBuffer[TxBufferSize++] = ChecksumHeaderTemp;
	Command->ChecksumHeader = ChecksumHeaderTemp;

	if (Command->DataSize > 0){
		// SET DATA
		memcpy( &TxBuffer[TxBufferSize], Command->Data, Command->DataSize);
		TxBufferSize += Command->DataSize;

		// SET Checksum Data
		TxBuffer[TxBufferSize++] = 0x00;      // TODO Checksum
		Command->ChecksumData = 0x00;
	}
	// SET TAIL
	TxBuffer[TxBufferSize++] = GSBP__UART_END_BYTE;


	// ### send command ###
	if ((ret = write(this->fd, TxBuffer, TxBufferSize)) != (int)TxBufferSize)
	{
		printf("\e[1m\e[91m%s ERROR:\e[0m Can't write to %s: %s (%d)\n", this->ID, this->SerialDeviceFileName, strerror(errno), errno);
		return false;
	}

	// save the CMD id
	this->lastCMDSend = Command->CommandID;
	// save a copy if we have to resend the package
	memcpy(this->LastPackageSend, TxBuffer, TxBufferSize);
	this->LastPackageSendSize = TxBufferSize;

	//Debug
#if (GSBP__DEBUG_SENDING_COMMANDS == true)
	Testbed_Interface::PrintPackage(Command);
	printf("\033[2A   Send: ");
	for(i=0; i<TxBufferSize; i++){
		printf("0x%02X ", (uint8_t)TxBuffer[i]);
	}
	printf("\n\n\n");
	fflush(stdout);
#endif

	return true;
}

bool Testbed_Interface::ReSendPackage(void)
{
	if (this->LastPackageSendSize == 0){
		return false;
	}

	// ... TODO
	return true;
}
void Testbed_Interface::ClearPackage(package_t * Package)
{
	memset(Package, 0, sizeof(package_t));
}

bool Testbed_Interface::CheckPackage(package_t *Package)
{
	//TODO
	if (Package->State != PackageIsOk) {
		return false;
	}

	// the package is ok -> act on specific packages
	switch (Package->CommandID){
	case StateACK:
		Testbed_Interface::ext_UpdateState(Package);
		return false;
		break;
	case ErrorCMD:
		Testbed_Interface::PackageHandler_Error(Package);
		return false;
		break;
	case DebugCMD:
		Testbed_Interface::PackageHandler_Debug(Package);
		return false;
		break;
	}

	// call the external function to see if this package should be handled by the external implementation
	if (Testbed_Interface::ext_PackageHandler(Package)){
		return false;
	}

	// debug output
#if (GSBP__DEBUG_RECEIVING_COMMANDS == true)
	Testbed_Interface::PrintPackage(Package);
#endif

	return true;
}


error_t Testbed_Interface::PackageHandler_Error(package_t * Package)
{
	if (Package->DataSize >= 1) {
		error_t ErrorId = (error_t)Package->Data[0];
		// check the error id
		if (ErrorId == NoDefinedErrorCode){
			// no error id set
			printf("%s: Error Package received but there was not error ID!\n\n", this->ID);
			return NoDefinedErrorCode;
		}

		if (ErrorId < MaxErrorsEnumID){
			// the error id is valid
			printf("\e[1m\e[91m%s: Error Package received\e[0m after sending '%s' (ID: %d|0x%02X)!\n   -> ErrorCode: \e[1m\e[91m%s\e[0m (ID: %d (0x%02X))\n", this->ID, ext_GetCommandString((command_t)this->lastCMDSend), (uint16_t)this->lastCMDSend, (uint8_t)this->lastCMDSend, ext_GetErrorString(ErrorId), (uint8_t)ErrorId, (uint8_t)ErrorId);
			if (Package->DataSize > 1) {
				// additional data received -> print Message
				// make sure this qualifies as a string
				Package->Data[Package->DataSize] = 0x00;
				Package->Data[Package->DataSize+1] = 0x00;
				printf("   -> additional Information: %s\n\n", Package->Data);
			}
			else {
				printf("\n");
			}
			return ErrorId;
		} else {
			// the error id is NOT valid
			printf("%s: Error Package received with UNKNOWN error code!\n   ErrorCode: %d (0x%02X)\n\n", this->ID, (int)ErrorId, (uint8_t)ErrorId);
			return ErrorId;
		}
	}
	else {
		printf("%s: Error Package received but there was not valid error ID!\n\n", this->ID);
		return NoDefinedErrorCode;
	}
}

void Testbed_Interface::PackageHandler_Debug(package_t * Package)
{
	// check the error id
	if (Package->DataSize > 0){
		// print Message
		// make sure this qualifies as a string
		Package->Data[Package->DataSize] = 0x00;
		Package->Data[Package->DataSize+1] = 0x00;
		printf("%s: Debug Package received\n   Message: %s\n\n", this->ID, Package->Data);
	}
}



bool Testbed_Interface::ReadPackages(bool doReturnAfterTimeout)
{
	int ret;
	// look this function
	if ( (ret = pthread_mutex_trylock(&(this->ReadPackage_mutex))) == EBUSY) {
		printf("\e[1m\e[91m%s ERROR during package read:\e[0m ReadPackage is looked: %s (%d)\n", this->ID, strerror(ret), ret);
		return false;
	}

	fd_set rfd;
	struct timeval TimeTimeout;
	unsigned char       RxBuffer[GSBP__UART_MAX_PACKAGE_SIZE];
	size_t              RxBufferSize = 0;
	size_t              BytesToRead = 0;
	int                 BytesRead = 0;
	int sel;
	uint8_t ChecksumHeaderTemp = 0x00;

	bool                SearchStartByte = true;
	bool                ReadHeader = false;
	bool                ReadData   = false;
	bool                NewPackage = false;

	memset(RxBuffer, 0, sizeof(RxBuffer));
	FD_ZERO(&rfd);
	FD_SET(this->fd, &rfd);
	TimeTimeout.tv_sec = 0;
	TimeTimeout.tv_usec = GSBP__PACKAGE_READ_TIMEOUT_US;

#if (GSBP__ACTIVATE_16BIT_PACKAGE_LENGHT_FEATURE == true)
	convert16_t temp_datasize;
#endif

	//   struct timeval TimeTimeout, time_start, time_stop;
	//gettimeofday(&time_start,NULL);
	//gettimeofday(&time_stop,NULL);
	//   u1 = time_start.tv_sec*1000 + time_start.tv_usec/1000;
	//   u2 = time_stop.tv_sec*1000 + time_stop.tv_usec/1000;
	//   *time_needed = (int)u2-u1;

	while(this->ReceicePackages || doReturnAfterTimeout)
	{
		// TODO Watchdog??? TODO abbruch nach dem Einlesen aller Bytes von der Schnittstelle (don't use read threat)

		// wait that something is received and check if the TimeTimeout was reached
		FD_SET(this->fd, &rfd);
		TimeTimeout.tv_usec = GSBP__PACKAGE_READ_TIMEOUT_US;
		if ( (sel = select(this->fd+1, &rfd, NULL, NULL, &TimeTimeout)) == 0) {
			// timeout triggered -> check if the package was complete;
			if (!SearchStartByte){
				// the package is incomplete; this should never happen
				// build package from what we have so far
				Testbed_Interface::BuildPackage(RxBuffer, RxBufferSize, PackageIsBroken_IncompleteTimout, 0x00);
				// reset the buffer for the next command
				RxBufferSize = 0;
				BytesToRead = 0;
				SearchStartByte = true;
				ReadHeader = false;
				ReadData = false;
			}
			// wait for a byte or exit -> start at the beginning of the loop
			if (doReturnAfterTimeout){
				// do not continuously read the serial interface; abort if there are no more bytes to read -> break
				break;
			}
			continue;
		} else {
			// the timeout did not occur -> did an error occur?
			if (sel < 0){
				printf("\e[1m\e[91m%s ERROR during package read:\e[0m Wait for a byte: %s (%d)\n", this->ID, strerror(errno), errno);
				// wait for one or more bytes or exit -> start at the beginning of the loop
				continue;
			}
		}

		// read the x bytes available
		if (SearchStartByte){
			// a new package starts -> read only one byte to search for the start byte
			BytesToRead = 1;
		}
		// check if the RxBuffer is large enough
		if ((RxBufferSize+BytesToRead) > GSBP__UART_MAX_PACKAGE_SIZE){
			// the RxBuffer is NOT large enough; this should never happen!
			printf("\e[1m\e[91m%s ERROR during package read:\e[0m RxBuffer is NOT large enough!\n   Requested buffer = %d bytes; available buffer = %d\n", this->ID, (int)(RxBufferSize+BytesToRead), (int)GSBP__UART_MAX_PACKAGE_SIZE);
			// fill the buffer and let the BuildPackage function decide if the package is still usable
			BytesToRead = GSBP__UART_MAX_PACKAGE_SIZE - RxBufferSize;
			SearchStartByte = false;
			ReadHeader = false;
			ReadData = true;
		}
		// read n bytes
		BytesRead = read(this->fd, (void *) &RxBuffer[RxBufferSize], BytesToRead);
		if (BytesRead < 0) {
			printf("\e[1m\e[91m%s ERROR during package read:\e[0m Read n bytes < 0: %s (%d)\n", this->ID, strerror(errno), errno);
			continue;
		}
		// we did read n bytes; set RxBufferSize to the next free value
		RxBufferSize += BytesRead;
		// degrease the BytesToRead counter
		BytesToRead -= BytesRead;
		// see if we are done for now?
		if (BytesToRead == 0) {
			// done to read a specific section -> start or header or data?
			if (SearchStartByte){
				if (RxBuffer[0] == GSBP__UART_START_BYTE){
					// a new package starts -> read the header first - the one start byte
					BytesToRead = GSBP__UART_PACKAGE_HEADER_SIZE -1 ;
					SearchStartByte = false;
					ReadHeader = true;
					ReadData = false;
				} else {
					// this was not the start byte, something went wrong -> at least log the error
					this->StatsGSBP.BytesDiscarded++;
					RxBufferSize = 0;
				}
			}
			else if (ReadHeader) {
				// reading the header is done -> make the checksum and get how many data bytes to read
				// make the checksum without the start byte and the checksum (RxBufferSize-1)
				ChecksumHeaderTemp = GSBP__UART_HEADER_CHECKSUM_START;
				for(uint32_t i=1; i<(RxBufferSize-1); i++){
					ChecksumHeaderTemp ^= RxBuffer[i];
					//TODO Checksumme ist nicht gut -> lieber one's sum ....
					//http://betterembsw.blogspot.de/2010/05/which-error-detection-code-should-you.html
				}
				// check the checksum
				if (RxBuffer[RxBufferSize-1] == ChecksumHeaderTemp) {
					/*
                        // print package contens
                        printf("%s Debug Package:\n Buffer Size: %lu\n     ", this->ID, RxBufferSize );
                        uint32_t i,j=0;
                        for(i=0; i<RxBufferSize; i++){
                        	printf("0x%02X ", RxBuffer[i]);
                        	j++;
                        	if (j==GSBP__DEBUG__PP_N_DATA_BYTES_PER_LINE){
                        		printf("\n     ");
                        		j=0;
                        	}
                        }
                        printf("\n");

                        printf("\n\n");
                        fflush(stdout);
					 */

					// checksum matches -> get the number of bytes to read next
#if (GSBP__ACTIVATE_16BIT_PACKAGE_LENGHT_FEATURE == true)
					temp_datasize.data = 0;
					temp_datasize.c_data[1] = RxBuffer[GSBP__UART_NUMBER_OF_DATA_BYTES__START_BYTE];
					temp_datasize.c_data[0] = RxBuffer[GSBP__UART_NUMBER_OF_DATA_BYTES__START_BYTE+1];
					BytesToRead = temp_datasize.data;
#else
					BytesToRead = (size_t)RxBuffer[GSBP__UART_NUMBER_OF_DATA_BYTES__START_BYTE];
#endif
					if (BytesToRead > 0){
						BytesToRead += GSBP__UART_PACKAGE_TAIL_SIZE;
					} else {
						// TODO TAIL Size unabhäning vom den Daten machen bzw. Checksumsize einführen
						BytesToRead += 1;
					}
					SearchStartByte = false;
					ReadHeader = false;
					ReadData = true;
					continue;
				} else {
					// checksum does not match -> TODO what now? wait 10 us and flush the buffer?
					printf("\e[1m\e[91m%s ERROR during package read:\e[0m Header checksum failed for ACK number %lu (%s (ID = 0x%02X)) (is: 0x%02X; should be: 0x%02X)\n", this->ID, (long unsigned int)Testbed_Interface::GetPackageNumberGlobal(isACK), Testbed_Interface::ext_GetCommandString((command_t)RxBuffer[1]), RxBuffer[1], ChecksumHeaderTemp, RxBuffer[RxBufferSize-1]);
					// update the statistics

					this->StatsGSBP.GlobalRxPackageNumber_BrokenChecksum++;
					// update the package counter because we receive a package, even if it is invalid
					Testbed_Interface::GetPackageNumberLocal(isACK, true);
					// TODO: send "repeat command" command
					//  wait 10 us and flush the buffer? there are data bytes availabe, which could contain a start_byte
					RxBufferSize = 0;
					BytesToRead = 0;
					SearchStartByte = true;
					ReadHeader = false;
					ReadData = false;
					continue;
				}
			}
			else if (ReadData) {
				// reading the data section is done -> build the package (check for the end byte later)
				Testbed_Interface::BuildPackage(RxBuffer, RxBufferSize-1, PackageIsOk, ChecksumHeaderTemp);
				// reset the buffer for the next command
				RxBufferSize = 0;
				BytesToRead = 0;
				SearchStartByte = true;
				ReadHeader = false;
				ReadData = false;
				NewPackage = true;
				continue;
			}
			else {
				printf("\e[1m\e[91m%s ERROR during package read:\e[0m Neither ReadHeader / ReadData active\n", this->ID);
			}
		} // endif (BytesToRead == 0)
	}

	// UnLock the function
	pthread_mutex_unlock(&(this->ReadPackage_mutex));

	return NewPackage;  // return if called from same thread
}


void Testbed_Interface::BuildPackage(unsigned char *RxBuffer, size_t RxBufferSize, packageState_t State, uint8_t ChecksumHeader)
{
	size_t       RxBufferSizeCounter = 0;
	uint8_t      PackageNumberLocalTemp = 0;
	uint32_t     ChecksumDataTemp = 0;

	if (State != PackageIsOk) {
		// the package is broken -> see if we can salvage anything

		// check if measurment ack and if not send "repeate last package" command
		printf("\e[1m\e[91m%s ERROR during package build:\e[0m Package is broken (State = %d)...\n", this->ID, (int)State);
		// resynchronize the local package counter
		this->StatsGSBP.ResetLocalTxPackageNumber = true;;
		return;
	}

	// create a new package for the queue -> allocate memory
	//package_t *Package = (package_t*) malloc(sizeof(package_t));               //TODO: remove
	package_t *Package = (package_t*) Testbed_Interface::UARTmalloc();

	// set default state
	Package->State = PackageIsBroken;
	Package->Typ = isACK;
	do {
		// the header was ok -> get the header data
		if (RxBuffer[RxBufferSizeCounter++] != GSBP__UART_START_BYTE) {
			// TODO
			Package->State = PackageIsBroken_StartByteError;
			printf("\e[1m\e[91m%s ERROR during package build:\e[0m Startbyte does not match (0x%02X)\n", this->ID, RxBuffer[RxBufferSizeCounter-1]);
			// update the statistics
			this->StatsGSBP.GlobalRxPackageNumber_BrokenStructur++;
			// update the package counter because we receive a package, even if it is invalid
			Testbed_Interface::GetPackageNumberLocal(isACK, true);
			break;
		}
#if (GSBP__ACTIVATE_DESTINATION_FEATURE == true)
		Package->Destination = RxBuffer[RxBufferSizeCounter++];
#endif
#if (GSBP__ACTIVATE_SOURCE_FEATURE == true)
		Package->Source = RxBuffer[RxBufferSizeCounter++];
#endif
#if (GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE == true)
		RxBuffer[RxBufferSizeCounter++] = //TODO
#endif
#if (GSBP__ACTIVATE_16BIT_CMD_FEATURE == true)
				convert16_t temp_cmd;
		temp_cmd.data = 0;
		temp_cmd.c_data[1] = RxBuffer[RxBufferSizeCounter++];
		temp_cmd.c_data[0] = RxBuffer[RxBufferSizeCounter++];
		Package->CommandID = (uint16_t)temp_cmd.data;
#else
		Package->CommandID = (size_t)RxBuffer[RxBufferSizeCounter++];
#endif
		// check if CommandID is valied
		if (!Testbed_Interface::ext_IsCommandIDValid((command_t)Package->CommandID)){
			Package->State = PackageIsBroken_InvalidCommandID;
			printf("\e[1m\e[91m%s ERROR during package build:\e[0m Invalid CommandID!!! (%02d|0x%02X)\n",
					this->ID, Package->CommandID, Package->CommandID);
			// update the statistics
			this->StatsGSBP.GlobalRxPackageNumber_BrokenStructur++;
			// update the package counter because we receive a package, even if it is invalid
			Testbed_Interface::GetPackageNumberLocal(isACK, true);
			break;
		}
#if (GSBP__ACTIVATE_CONTROL_FEATURE == true)
		Package->Control = RxBuffer[RxBufferSizeCounter++];
#endif
		PackageNumberLocalTemp = RxBuffer[RxBufferSizeCounter++];
		if (PackageNumberLocalTemp == Testbed_Interface::GetPackageNumberLocal(isACK, false)) {
			Package->PackageNumberLocal = Testbed_Interface::GetPackageNumberLocal(isACK, true);
			Package->PackageNumberGlobal = Testbed_Interface::GetPackageNumberGlobal(isACK);
		} else {
			// error! the package numbers do not match
			// get the expected local package number
			uint8_t LocalRxPackageNumberTemp = Testbed_Interface::GetPackageNumberLocal(isACK, false);
			int32_t PackagesMissing = 0;
			// calculate the missing packages
			if (LocalRxPackageNumberTemp != 0) {
				// calculate the missed packages, except for the communication start
				PackagesMissing = ((int32_t)PackageNumberLocalTemp) - (int32_t)LocalRxPackageNumberTemp;
				this->StatsGSBP.GlobalRxPackageNumber_Missing += abs(PackagesMissing);
			}
			// suppress error messages?
			if (!this->StatsGSBP.ResetLocalTxPackageNumber){
				//  print the error message except we have to (re)synchronise the package numbers at start or after an error
				printf("\e[1m\e[91m%s ERROR during package build:\e[0m Did not receive %d packages! (LocalCounter=%d | PackageCounter=%d)\n",
						this->ID, PackagesMissing, LocalRxPackageNumberTemp, PackageNumberLocalTemp);
			}
			// reset the flag to false
			this->StatsGSBP.ResetLocalTxPackageNumber = false;
			// set the local counter manually to the value of the package
			this->StatsGSBP.LocalRxPackageNumber = PackageNumberLocalTemp;
			Package->PackageNumberLocal = Testbed_Interface::GetPackageNumberLocal(isACK, true);
			Package->PackageNumberGlobal = Testbed_Interface::GetPackageNumberGlobal(isACK);
		}
#if (GSBP__ACTIVATE_16BIT_PACKAGE_LENGHT_FEATURE == true)
		convert16_t temp_datasize;
		temp_datasize.data = 0;
		temp_datasize.c_data[1] = RxBuffer[RxBufferSizeCounter++];
		temp_datasize.c_data[0] = RxBuffer[RxBufferSizeCounter++];
		Package->DataSize = (size_t)temp_datasize.data;
#else
		Package->DataSize = (size_t)RxBuffer[RxBufferSizeCounter++];
		if (Package->DataSize > GSBP__UART_MAX_PACKAGE_SIZE){
			Package->State = PackageIsBroken_IncompleteData;
			printf("\e[1m\e[91m%s ERROR during package build:\e[0m DataSize is to large!!! (DataSize = %d | max = %d)\n",
					this->ID, Package->DataSize, GSBP__UART_MAX_PACKAGE_SIZE);
			// update the statistics
			this->StatsGSBP.GlobalRxPackageNumber_BrokenStructur++;
			// update the package counter because we receive a package, even if it is invalid
			Testbed_Interface::GetPackageNumberLocal(isACK, true);
			break;
		}
#endif
		//TODO check checksum again
		Package->ChecksumHeader = ChecksumHeader;
		RxBufferSizeCounter++;

		// check the data length
		if (RxBufferSize < RxBufferSizeCounter) {
			// the receive buffer is smaller as it should be -> ERROR
			// TODO
			Package->State = PackageIsBroken_IncompleteData;
			printf("\e[1m\e[91m%s ERROR during package build:\e[0m RxBufferSize is to small!!! (ByteSize - ByteSizeCounter = %d)\n",
					this->ID, ((int)RxBufferSize - (int) RxBufferSizeCounter) );
			// update the statistics
			this->StatsGSBP.GlobalRxPackageNumber_BrokenStructur++;
			// update the package counter because we receive a package, even if it is invalid
			Testbed_Interface::GetPackageNumberLocal(isACK, true);
			break;
		}
		else if (RxBufferSize == RxBufferSizeCounter) {
			// no payload?
			if (Package->DataSize != 0){
				// there is a payload defined -> ERROR
				// TODO
				Package->State = PackageIsBroken_IncompleteData;
				printf("\e[1m\e[91m%s ERROR during package build:\e[0m RxBufferSize is to small and there should be data!!! (ByteSize - ByteSizeCounter = %d)\n",
						this->ID, ((int)RxBufferSize - (int) RxBufferSizeCounter) );
				// update the statistics
				this->StatsGSBP.GlobalRxPackageNumber_BrokenStructur++;
				// update the package counter because we receive a package, even if it is invalid
				Testbed_Interface::GetPackageNumberLocal(isACK, true);
				break;
			} else {
				// no payload
				if (RxBuffer[RxBufferSizeCounter] != GSBP__UART_END_BYTE){
					// the last byte is not the UART end byte
					Package->State = PackageIsBroken_EndByteError;
					printf("\e[1m\e[91m%s ERROR during package build:\e[0m The last byte for package %lu (local: %u) is not the expected End byte! -> package is discarged\n   BufferCounter: %lu; BufferSize: %lu; Endbyte package: 0x%02X;  Endbyte Buffer: 0x%02X\n",
							this->ID, (long unsigned int)Package->PackageNumberGlobal, (unsigned int)Package->PackageNumberLocal, (long unsigned int)RxBufferSizeCounter, (long unsigned int)RxBufferSize, RxBuffer[RxBufferSizeCounter], RxBuffer[RxBufferSize]);
					// update the statistics
					this->StatsGSBP.GlobalRxPackageNumber_BrokenStructur++;
					// update the package counter because we receive a package, even if it is invalid
					Testbed_Interface::GetPackageNumberLocal(isACK, true);
					break;
				} else {
					// ###  we are done. ###
					Package->State = PackageIsOk;
					break;
				}
			}
		}
		else {
			// we have a payload -> check the length
			if (RxBufferSize != (RxBufferSizeCounter + Package->DataSize + GSBP__UART_PACKAGE_TAIL_SIZE -1)) {
				// size does not macht -> ERROR
				// TODO
				Package->State = PackageIsBroken_IncompleteData;
				printf("\e[1m\e[91m%s ERROR during package build:\e[0m RxBuffer to small ??? (ByteSize = %d / ByteSizeCounter+DataSize+UART_Tail = %d) (DataSize = %d)\n",
						this->ID, (int)RxBufferSize, (int)((RxBufferSizeCounter + Package->DataSize + GSBP__UART_PACKAGE_TAIL_SIZE)), (int)Package->DataSize );
				// update the statistics
				this->StatsGSBP.GlobalRxPackageNumber_BrokenStructur++;
				// update the package counter because we receive a package, even if it is invalid
				Testbed_Interface::GetPackageNumberLocal(isACK, true);
				break;
			} else
			{
				// the size match -> copy the data
				memcpy(Package->Data, &RxBuffer[RxBufferSizeCounter], Package->DataSize);
				// get the data checksum
				// TODO
				RxBufferSizeCounter += Package->DataSize;
				ChecksumDataTemp = RxBuffer[RxBufferSizeCounter++];
				// Check the END byte
				if (RxBuffer[RxBufferSizeCounter] != GSBP__UART_END_BYTE){
					// size does not macht -> ERROR
					// TODO
					Package->State = PackageIsBroken_EndByteError;
					printf("\e[1m\e[91m%s ERROR during package build:\e[0m The last byte for package %lu (local: %u) is not the expected End byte! -> package is discarged\n   BufferCounter: %lu; BufferSize: %lu; Endbyte package: 0x%02X;  Endbyte Buffer: 0x%02X\n",
							this->ID, (long unsigned int)Package->PackageNumberGlobal, (unsigned int)Package->PackageNumberLocal, (long unsigned int)RxBufferSizeCounter, (long unsigned int)RxBufferSize, RxBuffer[RxBufferSizeCounter], RxBuffer[RxBufferSize]);
					// update the statistics
					this->StatsGSBP.GlobalRxPackageNumber_BrokenStructur++;
					// update the package counter because we receive a package, even if it is invalid
					Testbed_Interface::GetPackageNumberLocal(isACK, true);
					break;
				} else {
					// ###  we are done. ###
					Package->State = PackageIsOk;
					break;
				}
			}
		}
	} while(false);


	// prepare the package for the package queue
	Package->Next = NULL;
	// check the package integrity and act on error conditions
	if (Testbed_Interface::CheckPackage(Package)){
		// package ok -> push the package into the queue for later reading
		Testbed_Interface::PushPackage(Package);
	} else {
		// free the memory
		//free(Package); TODO remove
		// workaround TODO
		if (UARTQueue_Head == 0){
			UARTQueue_Head = GSBP__QUEUE_SIZE_UART_PACKAGES;                                                       // reset the queue
		}
		UARTQueue_Head--;
	}
}


void Testbed_Interface::PushPackage(struct package_t *Package)
{
	// lock the queue
	pthread_mutex_lock(&(this->QueueLock_mutex));

	// make sure Next is Null
	Package->Next = NULL;
	// move the Head and Tail pointer
	if (this->QueueHead == NULL) {
		// there was no head -> this is now the head
		this->QueueHead = Package;
	} else {
		// there is a head -> the old tail will point to this package
		this->QueueTail->Next = Package;
	}
	// this package ist now the tail
	this->QueueTail = Package;
	// increase the queue size
	this->QueueSize++;

	// unlock the queue
	pthread_mutex_unlock(&(this->QueueLock_mutex));
}

bool Testbed_Interface::PopPackage(package_t *Package)
{
	// is there a package to pop?
	if (this->QueueSize == 0) {
		// no -> return false
		return false;
	}

	// lock the queue
	pthread_mutex_lock(&(this->QueueLock_mutex));

	// get the first package
	package_t *P = this->QueueHead;
	// copy the data from Head to the package
	memcpy(Package, P, sizeof(package_t));
	// move the head-pointer to the new head
	this->QueueHead = P->Next;
	// degrease the queue size
	this->QueueSize--;
	if (this->QueueSize == 0) {
		this->QueueTail = NULL;
	}

	// free the head memory
	// free(P); TODO remove
	Testbed_Interface::UARTfree();

	// unlock the queue
	pthread_mutex_unlock(&(this->QueueLock_mutex));

	return true;
}

bool Testbed_Interface::WaitForPackage(uint8_t CMD, int32_t WaitTimeout_us, uint32_t N_MaxOtherPackages, package_t *Package)
{
	// get the packages if the receiver threat is did not do it
#if (GSBP__USE_RECEIVING_THREAT == true)
	if (!this->ReceicerThreatRunning){
		Testbed_Interface::ReadPackages(true);
	}
#else
	Testbed_Interface::ReadPackages(true);
#endif

	bool         PackageFound = false, NoTimeout;
	struct       timeval Time;
	uint64_t     TimeTimeout_us, Time_us;
	uint32_t     PackageCounter = 0;

	// TimeTimeout preparations
	gettimeofday(&Time,NULL);
	Time_us = Time.tv_sec*1000000 + Time.tv_usec;
	// wait timeout?
	if (WaitTimeout_us >= 0){
		Time.tv_usec += WaitTimeout_us;
		TimeTimeout_us = Time.tv_sec*1000000 + Time.tv_usec;
		NoTimeout = false;
	} else {
		TimeTimeout_us = Time_us;
		NoTimeout = true;
	}

	while((TimeTimeout_us > Time_us) || NoTimeout){
		// is there a package? if so retrieve it
		if (Testbed_Interface::PopPackage(Package)){
			PackageCounter++;
			// check if this is the requested cmd
			if (Package->CommandID == CMD) {
				PackageFound = true;
				break;
			} else {
				//TODO better handle this case
				printf("%s Info: WaitForPackage: Receive unexpected package (%s (%02d|0x%02X)) while waiting for package %s (%02d|0x%02X)\n", this->ID, Testbed_Interface::ext_GetCommandString((command_t)Package->CommandID), Package->CommandID, Package->CommandID, Testbed_Interface::ext_GetCommandString((command_t)CMD), CMD, CMD);
				Testbed_Interface::PrintPackage(Package);
				if (PackageCounter >= N_MaxOtherPackages){
					PackageFound = false;
					break;
				}
			}
		} else {
			// wait 5000us
			usleep(500);
		}

		// update the timer
		gettimeofday(&Time,NULL);
		Time_us = Time.tv_sec*1000000 + Time.tv_usec;
	}

	return PackageFound;
}



/* ### #########################################################################
 * Debug / Info functions
 * ### #########################################################################
 */

void Testbed_Interface::PrintPackage(package_t *Package)
{
	char CTyp[50], CState[100], COptions[100], CChecksumData[50];
	// prepare what to print
	switch (Package->Typ){
	case isCommand:   sprintf(CTyp, "CMD"); break;
	case isACK:       sprintf(CTyp, "ACK"); break;
	default:          sprintf(CTyp, "Unknown typ!");
	}
	switch (Package->State){
	case PackageIsOk:                        sprintf(CState, "Ok"); break;
	case PackageIsBroken:                    sprintf(CState, "Broken"); break;
	case PackageIsBroken_EndByteError:       sprintf(CState, "Broken, because the end byte did not match"); break;
	case PackageIsBroken_IncompleteData:     sprintf(CState, "Broken, because the Data was incomplete"); break;
	case PackageIsBroken_IncompleteTimout:   sprintf(CState, "Broken, because not all bytes were transmittet in time"); break;
	default:                                 sprintf(CState, "Unknown state!");
	}
	if ( Package->Typ == isACK ) {
		sprintf(CTyp, "%s -> State: %s", CTyp, CState);
	}
	COptions[0] = 0x00;
#if (GSBP__ACTIVATE_DESTINATION_FEATURE == true || GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE == true)
	sprintf(&COptions[strlen(COptions)], "   Destination: %d\n", Package->Destination);
#endif
#if (GSBP__ACTIVATE_SOURCE_FEATURE == true || GSBP__ACTIVATE_SOURCE_DESTINATION_FEATURE == true)
	sprintf(&COptions[strlen(COptions)], "   Source: %d\n", Package->Source);
#endif
#if (GSBP__ACTIVATE_CONTROL_BYTE_FEATURE == true)
	sprintf(&COptions[strlen(COptions)], "   Control Byte: 0x%02X\n", Package->Control);
#endif
	// TODO Checksum
	sprintf(CChecksumData, "TODO");


	// print package contens
	printf("%s Debug Package:\n   CommandID: %s (%02d|0x%02X)\n   Typ: %s\n%s   ID number Local: %u\n   ID number Global: %lu\n   Checksum Header: 0x%02X\n   Checksum Data: %s\n   Data Size: %d\n     ",
			this->ID, Testbed_Interface::ext_GetCommandString((command_t)Package->CommandID), Package->CommandID, Package->CommandID, CTyp, COptions, (unsigned int)Package->PackageNumberLocal, (long unsigned int)Package->PackageNumberGlobal, Package->ChecksumHeader, CChecksumData, (int)Package->DataSize );
	if ( Package->DataSize > 0 ) {
		uint32_t i,j=0;
		for(i=0; i<Package->DataSize; i++){
			printf("0x%02X ", Package->Data[i]);
			j++;
			if (j==GSBP__DEBUG__PP_N_DATA_BYTES_PER_LINE){
				printf("\n     ");
				j=0;
			}
		}
		printf("\n");
	}
	printf("\n\n");
	fflush(stdout);
}


void Testbed_Interface::PrintStatsGSBP()
{
	// print statistics
	printf("%s Statistics GSBP:\n   Packages received = %lu (missing: %lu | broken checksum: %lu | broken structur: %lu | bytes discarged: %lu | queue size: %u)\n   Packages send = %lu (resend: %lu | errors received: %lu)\n\n",
			this->ID, (long unsigned int)this->StatsGSBP.GlobalRxPackageNumber, (long unsigned int)this->StatsGSBP.GlobalRxPackageNumber_Missing, (long unsigned int)this->StatsGSBP.GlobalRxPackageNumber_BrokenChecksum, (long unsigned int)this->StatsGSBP.GlobalRxPackageNumber_BrokenStructur, (long unsigned int)this->StatsGSBP.BytesDiscarded, (unsigned int)this->QueueSize,
			(long unsigned int)this->StatsGSBP.GlobalTxPackageNumber, (long unsigned int)this->StatsGSBP.GlobalTxPackageNumber_ReSend, (long unsigned int)this->StatsGSBP.GlobalTxPackageNumber_ErrorReceived
	);
	fflush(stdout);
}

void Testbed_Interface::PrintState( void )
{
	char CTime[100];
	struct timeval Now;
	gettimeofday(&Now,NULL);
	// get last time of update
	uint64_t t2 = this->StateEXT.LastUpdate.tv_sec*1000 + (uint64_t)(this->StateEXT.LastUpdate.tv_usec/1000);
	if (t2 == 0){
		sprintf(CTime, "Last Update: NEVER");
	} else {
		uint64_t t1 = Now.tv_sec*1000 + (uint64_t)(Now.tv_usec/1000);
		if ((t1-t2) >= 1000) {
			// print in sec
			sprintf(CTime, "Last Update: %.3f sec ago", ((double)(t1-t2) / 1000.0));
		} else {
			// print in usec
			sprintf(CTime, "Last Update: %d usec ago", (int)(t1-t2));
		}
	}
	//
	// project specific implementation
	printf("%s State: ( %s )\n   External State: %d\n   External Voltage: %.3f\n",
			this->ID, CTime, this->StateEXT.State, this->StateEXT.Voltage
	);

	// print the additional project specific stuff
	Testbed_Interface::ext_PrintState();
}



/* ### #########################################################################
 * THREADS
 * ### #########################################################################
 */
// void *thread_InitSensor(void *data)
// {
//    RW_BT_Interface *sensor =  (RW_BT_Interface *) data;
//    sensor->init_Sensor();
//    pthread_exit(NULL);
// }
//

void *ReceiverThread(void *data)
{
	Testbed_Interface *Device = (Testbed_Interface *) data;
	Device->ReadPackages(false);
	pthread_exit(NULL);
}



/* ### #########################################################################
 * Functions, specific to the external system
 * ### #########################################################################
 */


bool Testbed_Interface::ext_SendInit( initConfig_t *InitConfig )
{
	uint8_t ret, SendCounter;

	package_t P, A;
	// send first command
	Testbed_Interface::ClearPackage(&P);
	Testbed_Interface::ClearPackage(&A);

	memcpy(&this->testbed.config, InitConfig, sizeof(initConfig_t));


	// build init package
	P.Typ = isCommand;
	P.CommandID = InitCMD;

	testbed_initCmd_t *cmd = (testbed_initCmd_t*)P.Data;
	cmd->dataType				= InitConfig->dataPackageType;
	cmd->samplePeriodeMultiplier= InitConfig->samplePeriodeMultiplier;
	cmd->searchWidth			= InitConfig->searchWidth;
	cmd->demuxEnabled 			= InitConfig->demuxEnabled;
	cmd->demuxChannelsActive 	= InitConfig->demuxChannelsActive;
	cmd->demuxChannelsPassive 	= InitConfig->demuxChannelsPassive;
	cmd->thresholdCh1Positiv 	= InitConfig->thresholdCh1Positiv;
	cmd->thresholdCh1Negative 	= InitConfig->thresholdCh1Negative;
	cmd->thresholdCh2Positiv  	= InitConfig->thresholdCh2Positiv;
	cmd->thresholdCh2Negative 	= InitConfig->thresholdCh2Negative;

	P.DataSize = sizeof(testbed_initCmd_t);
	// send package and receive the responce
	SendCounter = GSBP__PACKAGE__N_SEND_RECEIVE_RETRYS_INIT;
	do {
		Testbed_Interface::SendPackage(&P);
		SendCounter--;
	} while (!(ret = Testbed_Interface::WaitForPackage(InitACK, 300*1000, 0, &A)) && (SendCounter > 0));
	if (!ret && SendCounter == 0){
		// did not get at responce
		return false;
	}

	testbed_initAck_t *ack = (testbed_initAck_t*)A.Data;
	if (ack->successful){
		this->testbed.sampleBufferSize = ack->sampleBufferSize;
		this->testbed.sampleFrequency = ack->sampleFrequency;
	} else {
		return false;
	}

	this->DeviceWorking = true;
	return true;
}

bool Testbed_Interface::ext_StartMeasurment(uint16_t NumberOfSamplesToGet)
{
	package_t P;
	Testbed_Interface::ClearPackage(&P);

	P.Typ = isCommand;
	P.CommandID = StartMeasurmentCMD;
	P.DataSize = 2;
	convert16_t temp;
	temp.data = NumberOfSamplesToGet;
	P.Data[0] = temp.c_data[0];
	P.Data[1] = temp.c_data[1];

	Testbed_Interface::SendPackage(&P);
	// reset the time index
	this->testbed.packageCounterStart = 0;

	if (Testbed_Interface::WaitForPackage(StartMeasurmentACK, 100*1000, 10, &P)){
		return true;
	} else {
		return false;
	}
}

bool Testbed_Interface::ext_StopMeasurment(void)
{
	package_t P;
	Testbed_Interface::ClearPackage(&P);

	P.Typ = isCommand;
	P.CommandID = StopMeasurmentCMD;

	Testbed_Interface::SendPackage(&P);

	if (Testbed_Interface::WaitForPackage(StopMeasurmentACK, 100*1000, 100, &P)){
		return true;
	} else {
		return false;
	}
}

bool Testbed_Interface::ext_SendDeInit( void )
{
	uint8_t ret, SendCounter;

	package_t P, A;
	// send first command
	Testbed_Interface::ClearPackage(&P);
	Testbed_Interface::ClearPackage(&A);

	// TODO eigene funktion für das INIT Kommando
	// build init package
	P.Typ = isCommand;
	P.CommandID = DeInitCMD;
	P.DataSize = 0;
	// send package and receive the responce
	SendCounter = GSBP__PACKAGE__N_SEND_RECEIVE_RETRYS;
	do {
		Testbed_Interface::SendPackage(&P);
		SendCounter--;
	} while (!(ret = Testbed_Interface::WaitForPackage(DeInitACK, 300*1000, 0, &A)) && (SendCounter > 0));
	if (!ret && SendCounter == 0){
		// did not get at responce
		return false;
	}
	this->DeviceWorking = false;
	return true;
}

/*
 * Package Handler for all application specific packages which should not be stored in the package queue.
 */
bool Testbed_Interface::ext_PackageHandler(package_t *Package)
{
	if (Package->State != PackageIsOk) {
		return false;
	}

	// the package is ok -> act on specific packages
	switch (Package->CommandID){
	case MeasurmentDataACK:
		//PrintPackage(Package);
		return ext_PackageHandlerMeasPackage(Package);
		break;
	}
	return false;
}

bool Testbed_Interface::ext_PackageHandlerMeasPackage(package_t *Package)
{

	maxAdc_dataPackage_t *data = (maxAdc_dataPackage_t*)Package->Data;
	if (this->testbed.packageCounterStart == 0){
			this->testbed.packageCounterStart = data->counter -1;
	}
	if ((data->counter <= this->testbed.packageCounter)) {
		printf("\nWARNING: data packages counter invalid (is: %u | should be: %u | type: %u)\n", data->counter, this->testbed.packageCounter+1, data->dataType);
	}
	// adjust the counter, so that the time starts from 0 each time a measurement is started
	data->counter -= this->testbed.packageCounterStart;

	switch (data->dataType & TESTBED_PACKAGE_TYPE_MASK){
	case mAdc_packageType_noStimulation:{
		switch(testbed.config.outputType){
		case tb_outputStimDataStructs:
			Testbed_Interface::ext_PackageHandlerMeasPackage_StimStruct_NoStimulation(data);
			break;
		case tb_outputStreamDataCompressed:
			Testbed_Interface::ext_PackageHandlerMeasPackage_StreamCompressed_NoStimulation(data);
			break;
		case tb_outputStreamDataFull:
			Testbed_Interface::ext_PackageHandlerMeasPackage_StreamFull_NoStimulation(data);
			break;
		default:
			// error
			printf("\nWARNING: output type %d unknown\n\n", testbed.config.outputType);
		}
		break; }

	case mAdc_packageType_Stimulation:{
		switch(testbed.config.outputType){
		case tb_outputStimDataStructs:
			Testbed_Interface::ext_PackageHandlerMeasPackage_StimStruct_Stimulation(data, Package->DataSize);
			break;
		case tb_outputStreamDataCompressed:
			Testbed_Interface::ext_PackageHandlerMeasPackage_StreamCompressed_Stimulation(data, Package->DataSize);
			break;
		case tb_outputStreamDataFull:
			Testbed_Interface::ext_PackageHandlerMeasPackage_StreamFull_Stimulation(data, Package->DataSize);
			break;
		default:
			// error
			printf("\nWARNING: output type %d unknown\n\n", testbed.config.outputType);
		}
		break;}
	default:
		printf("\nWARNING: package data type %d unknown\n\n", data->dataType & TESTBED_PACKAGE_TYPE_MASK);
	}

	return true;
}

/*
 * STIM Structs
 */
void Testbed_Interface::ext_PackageHandlerMeasPackage_StimStruct_NoStimulation(maxAdc_dataPackage_t *data)
{
	//printf("NoStim Struct:  %d -> %d\n", data->counter, data->numberOfSamples);

	// lock the queue
	pthread_mutex_lock(&(this->testbed_mutex));

	uint32_t numberOfPackegesRecorded = data->counter - this->testbed.packageCounter;
	this->testbed.packageCounter = data->counter;

	// add the zero samples
	this->testbed.sampleCounter += numberOfPackegesRecorded * this->testbed.sampleBufferSize;

	// has a stim pulse ended?
	if (this->testbed.pulseStarted){
		// the previous was a pulse -> end it now
		this->testbed.pulseStarted = false;
		this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.newData = true;
		this->testbed.indexPulsToFillNext++;
		this->testbed.indexPulsToFillNext &= TESTBED_PULSE_QUEUE_INDEX_MASK;
		// resets
		this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.numberOfSamples = 0;
		this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.activeStimChannels_positive = 0;
		this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.activeStimChannels_negative = 0;
	}

	this->testbed.watchdogTriggered = true;

	// unlock the queue
	pthread_mutex_unlock(&(this->testbed_mutex));
}

void Testbed_Interface::ext_PackageHandlerMeasPackage_StimStruct_Stimulation(maxAdc_dataPackage_t *data, size_t dataSize)
{
	//printf("Stim Struct:  %d -> %d\n", data->counter, data->numberOfSamples);

	// lock the queue
	pthread_mutex_lock(&(this->testbed_mutex));

	uint32_t numberOfPackagesSkiped = data->counter - this->testbed.packageCounter -1; // -1 because the current package is full of ADC samples
	this->testbed.packageCounter = data->counter;

	if (numberOfPackagesSkiped > 0){
		// add the zero samples from the packages the MCU did not send, because there was no stimulation active
		this->testbed.sampleCounter += numberOfPackagesSkiped * this->testbed.sampleBufferSize;

		// has a stim pulse ended?
		if (this->testbed.pulseStarted){
			// the previous was a pulse -> end it now
			this->testbed.pulseStarted = false;
			this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.newData = true;
			this->testbed.indexPulsToFillNext++;
			this->testbed.indexPulsToFillNext &= TESTBED_PULSE_QUEUE_INDEX_MASK;
			// resets
			this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.numberOfSamples = 0;
			this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.activeStimChannels_positive = 0;
			this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.activeStimChannels_negative = 0;
		}
	}

	if (data->numberOfSamples > 0){
		this->testbed.pulseStarted = true;
		this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.startByte[0] = '#';
		this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.startByte[1] = '#';
		this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.startByte[2] = '#';
		this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.pulseNotComplete = 0;
		if (data->dataType & TESTBED_PACKAGE_DATA_OVERRIDEN){
			this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.dataWasOverriden = 1;
		}
		if (data->dataType & TESTBED_PACKAGE_DATA_DISCARDED){
			this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.dataWasOverriden = 2;
		}

		for (uint16_t i = 0; i< data->numberOfSamples; i++){
			this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].stimCurrent[this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.numberOfSamples] = data->data[i].ch1;
			this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].stimVoltage[this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.numberOfSamples] = data->data[i].ch2;
			this->testbed.sampleCounter++;
			this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].sampleNumber[this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.numberOfSamples] = this->testbed.sampleCounter;
			this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].time[this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.numberOfSamples]         = ((float)this->testbed.sampleCounter)/this->testbed.sampleFrequency;
			this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.numberOfSamples++;
			if (this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.numberOfSamples >= TESTBED_NUMBER_OF_SAMPLES_PER_PULS){
				// the sample array is full -> break up the data
				this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.pulseNotComplete = 1;
				// end this pulse
				this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.newData = true;
				this->testbed.indexPulsToFillNext++;
				this->testbed.indexPulsToFillNext &= TESTBED_PULSE_QUEUE_INDEX_MASK;
				this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.numberOfSamples = 0;
				// set up the next one
				this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.startByte[0] = '#';
				this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.startByte[1] = '#';
				this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.startByte[2] = '#';
			}
		}

		if (dataSize > (TESTBED_STIM_DATA_PACKAGE_OFFSET +data->numberOfSamples*sizeof(maxAdc_dataBuffer_t))){

			// TODO struct mit der vollständigen defition + Demux Support
			uint8_t *additionalData = (uint8_t*)&data->data[data->numberOfSamples].ch1;

			uint16_t channelDetect_I_PN = additionalData[1]<< 8;
			channelDetect_I_PN |= additionalData[0];

			if (channelDetect_I_PN != TESTBED_STIM_CHANNEL_DETECT_MCU_ALL && channelDetect_I_PN != 0){
				// TODO fixen/ eigene Funktion    		printf("Channel: %d -> %d|%d -> %d\n", (uint16_t)data->data[data->numberOfSamples].ch1, additionalData[0], additionalData[1], channelDetect_I_PN);
				// positive channels
				if (!(channelDetect_I_PN & TESTBED_STIM_CHANNEL_DETECT_MCU_P1)){
					this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.activeStimChannels_positive |= TESTBED_STIM_CHANNEL_DETECT_C1;
				}
				if (!(channelDetect_I_PN & TESTBED_STIM_CHANNEL_DETECT_MCU_P2)){
					this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.activeStimChannels_positive |= TESTBED_STIM_CHANNEL_DETECT_C2;
				}
				if (!(channelDetect_I_PN & TESTBED_STIM_CHANNEL_DETECT_MCU_P3)){
					this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.activeStimChannels_positive |= TESTBED_STIM_CHANNEL_DETECT_C3;
				}
				if (!(channelDetect_I_PN & TESTBED_STIM_CHANNEL_DETECT_MCU_P4)){
					this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.activeStimChannels_positive |= TESTBED_STIM_CHANNEL_DETECT_C4;
				}
				// negative channels
				if (!(channelDetect_I_PN & TESTBED_STIM_CHANNEL_DETECT_MCU_N1)){
					this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.activeStimChannels_negative |= TESTBED_STIM_CHANNEL_DETECT_C1;
				}
				if (!(channelDetect_I_PN & TESTBED_STIM_CHANNEL_DETECT_MCU_N2)){
					this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.activeStimChannels_negative |= TESTBED_STIM_CHANNEL_DETECT_C2;
				}
				if (!(channelDetect_I_PN & TESTBED_STIM_CHANNEL_DETECT_MCU_N3)){
					this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.activeStimChannels_negative |= TESTBED_STIM_CHANNEL_DETECT_C3;
				}
				if (!(channelDetect_I_PN & TESTBED_STIM_CHANNEL_DETECT_MCU_N4)){
					this->testbed.pulseQueue[this->testbed.indexPulsToFillNext].header.activeStimChannels_negative |= TESTBED_STIM_CHANNEL_DETECT_C4;
				}
			}
		}
		this->testbed.watchdogTriggered = true;
	}

	// unlock the queue
	pthread_mutex_unlock(&(this->testbed_mutex));;
}


/*
 * Stream Compressed data
 */
void Testbed_Interface::ext_PackageHandlerMeasPackage_StreamCompressed_NoStimulation(maxAdc_dataPackage_t *data)
{
	//printf("NoStim StreamCompressed:  %d -> %d\n", data->counter, data->numberOfSamples);

	// lock the queue
	pthread_mutex_lock(&(this->testbed_mutex));

	uint32_t numberOfPackegesRecorded = data->counter - this->testbed.packageCounter;
	this->testbed.packageCounter = data->counter;

	// make the fist sample to a 0 point to have a smooth signal
	this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].voltage = 0.0;
	this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].current = 0.0;
	this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].samplesCounter = this->testbed.sampleCounter +1;
	this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples++].time = ((double)this->testbed.sampleCounter +1)/this->testbed.sampleFrequency;

	// add the zero samples
	this->testbed.sampleCounter += numberOfPackegesRecorded * this->testbed.sampleBufferSize;

	// make the last sample to a 0 point to have a smooth signal
	this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].voltage = 0.0;
	this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].current = 0.0;
	this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].samplesCounter = this->testbed.sampleCounter;
	this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples++].time = ((double)this->testbed.sampleCounter)/this->testbed.sampleFrequency;
	this->testbed.streamData.isNew = true;

	// unlock the queue
	pthread_mutex_unlock(&(this->testbed_mutex));
}

void Testbed_Interface::ext_PackageHandlerMeasPackage_StreamCompressed_Stimulation(maxAdc_dataPackage_t *data, size_t dataSize)
{
	//printf("Stim:  %d -> %d\n", data->counter, data->numberOfSamples);

	// lock the queue
	pthread_mutex_lock(&(this->testbed_mutex));

	uint32_t numberOfPackegesRecorded = data->counter - this->testbed.packageCounter -1; // -1 because the current package is full of ADC samples
	this->testbed.packageCounter = data->counter;

	if (numberOfPackegesRecorded != 0){
		// make the last sample to a 0 point to have a smooth signal
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].voltage = 0.0;
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].current = 0.0;
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].samplesCounter = this->testbed.sampleCounter +1;
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples++].time = ((double)this->testbed.sampleCounter +1)/this->testbed.sampleFrequency;

		// add the zero samples
		this->testbed.sampleCounter += numberOfPackegesRecorded * this->testbed.sampleBufferSize;;

		// make the last sample to a 0 point to have a smooth signal
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].voltage = 0.0;
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].current = 0.0;
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].samplesCounter = this->testbed.sampleCounter;
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples++].time = ((double)this->testbed.sampleCounter)/this->testbed.sampleFrequency;

	}

	if (data->numberOfSamples > 0){
		this->testbed.streamData.isNew = true;
		if (dataSize > (TESTBED_STIM_DATA_PACKAGE_OFFSET +data->numberOfSamples*sizeof(maxAdc_dataBuffer_t))){
			// TODO add Channel info
		}
		for (uint16_t i = 0; i< data->numberOfSamples; i++){
			this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].current = data->data[i].ch1;
			this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].voltage = data->data[i].ch2;
			this->testbed.sampleCounter++;
			this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].samplesCounter = this->testbed.sampleCounter;
			this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples++].time = ((double)this->testbed.sampleCounter)/this->testbed.sampleFrequency;
		}
	}

	// unlock the queue
	pthread_mutex_unlock(&(this->testbed_mutex));
}


/*
 * Stream Full data
 */
void Testbed_Interface::ext_PackageHandlerMeasPackage_StreamFull_NoStimulation(maxAdc_dataPackage_t *data)
{
	//printf("NoStim StreamFull:  %d -> %d\n", data->counter, data->numberOfSamples);

	// lock the queue
	pthread_mutex_lock(&(this->testbed_mutex));

	uint32_t numberOfPackegesRecorded = data->counter - this->testbed.packageCounter;
	this->testbed.packageCounter = data->counter;

	for (uint16_t i=0; i< (numberOfPackegesRecorded * this->testbed.sampleBufferSize); i++){
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].voltage = 0.0;
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].current = 0.0;
		this->testbed.sampleCounter++;
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].samplesCounter = this->testbed.sampleCounter;
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples++].time = ((double)this->testbed.sampleCounter)/this->testbed.sampleFrequency;
	}
	this->testbed.streamData.isNew = true;

	// unlock the queue
	pthread_mutex_unlock(&(this->testbed_mutex));
}

void Testbed_Interface::ext_PackageHandlerMeasPackage_StreamFull_NoStimulationData(maxAdc_dataPackage_t *data)
{
	double voltage = 0.0;
	double current = 0.0;

	//printf("NoStim StreamFull:  %d -> %d", data->counter, data->numberOfSamples);

	// lock the queue
	pthread_mutex_lock(&(this->testbed_mutex));

	if (data->numberOfSamples > 0){
		for (uint16_t i = 0; i< data->numberOfSamples; i++){
			for (uint8_t j = 0; j<MAX1119X_SEARCH_WIDTH; j++){
				this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].current = data->data[i].ch1;
				this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].voltage = data->data[i].ch2;
				this->testbed.sampleCounter++;
				this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].samplesCounter = this->testbed.sampleCounter;
				this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples++].time = ((double)this->testbed.sampleCounter)/this->testbed.sampleFrequency;
			}
		}
		this->testbed.streamData.isNew = true;
	}
	this->testbed.packageCounter = data->counter;

	// unlock the queue
	pthread_mutex_unlock(&(this->testbed_mutex));
}

void Testbed_Interface::ext_PackageHandlerMeasPackage_StreamFull_Stimulation(maxAdc_dataPackage_t *data, size_t dataSize)
{
	//printf("Stim StreamFull:  %d -> %d\n", data->counter, data->numberOfSamples);

	// lock the queue
	pthread_mutex_lock(&(this->testbed_mutex));


	uint32_t numberOfPackegesRecorded = data->counter - this->testbed.packageCounter -1; // -1 because the current package is full of ADC samples
	this->testbed.packageCounter = data->counter;

	if (numberOfPackegesRecorded != 0){
		// make the last sample to a 0 point to have a smooth signal
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].voltage = 0.0;
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].current = 0.0;
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].samplesCounter = this->testbed.sampleCounter +1;
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples++].time = ((double)this->testbed.sampleCounter +1)/this->testbed.sampleFrequency;

		// add the zero samples
		this->testbed.sampleCounter += numberOfPackegesRecorded * this->testbed.sampleBufferSize;;

		// make the last sample to a 0 point to have a smooth signal
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].voltage = 0.0;
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].current = 0.0;
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].samplesCounter = this->testbed.sampleCounter;
		this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples++].time = ((double)this->testbed.sampleCounter)/this->testbed.sampleFrequency;

	}

	if (data->numberOfSamples > 0){
		this->testbed.streamData.isNew = true;
		if (dataSize > (TESTBED_STIM_DATA_PACKAGE_OFFSET +data->numberOfSamples*sizeof(maxAdc_dataBuffer_t))){
			// TODO add Channel info
		}
		for (uint16_t i = 0; i< data->numberOfSamples; i++){
			this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].current = data->data[i].ch1;
			this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].voltage = data->data[i].ch2;
			this->testbed.sampleCounter++;
			this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples].samplesCounter = this->testbed.sampleCounter;
			this->testbed.streamData.samples[this->testbed.streamData.numberOfSamples++].time = ((double)this->testbed.sampleCounter)/this->testbed.sampleFrequency;
		}
	}

	// unlock the queue
	pthread_mutex_unlock(&(this->testbed_mutex));
}

bool Testbed_Interface::ext_GetStreamData( testbed_streamData_t *Data )
{
	bool ret = false; // todo use watchdog var

	// lock the queue
	pthread_mutex_lock(&(this->testbed_mutex));

	if (this->testbed.streamData.isNew){
		memcpy(Data, &this->testbed.streamData, 32+this->testbed.streamData.numberOfSamples*sizeof(stimData_t));
		this->testbed.streamData.isNew = false;
		this->testbed.streamData.numberOfSamples = 0;
		ret = true;
	}

	// unlock the queue
	pthread_mutex_unlock(&(this->testbed_mutex));

	return ret;
}

bool Testbed_Interface::ext_GetStimPuls( testbed_stimulationPulse_t *puls )
{
	// lock the queue
	pthread_mutex_lock(&(this->testbed_mutex));
	bool ret = this->testbed.watchdogTriggered;
	this->testbed.watchdogTriggered = false;
	puls->header.newData = false;

	if (this->testbed.pulseQueue[this->testbed.indexPulsToReturnNext].header.newData){
		memcpy(puls, &this->testbed.pulseQueue[this->testbed.indexPulsToReturnNext], sizeof(testbed_stimulationPulseHeader_t));
		memcpy(puls->sampleNumber, &this->testbed.pulseQueue[this->testbed.indexPulsToReturnNext].sampleNumber,
				sizeof(double)*puls->header.numberOfSamples);
		memcpy(puls->time, &this->testbed.pulseQueue[this->testbed.indexPulsToReturnNext].time,
				sizeof(double)*puls->header.numberOfSamples);
		memcpy(puls->stimCurrent, &this->testbed.pulseQueue[this->testbed.indexPulsToReturnNext].stimCurrent,
				sizeof(double)*puls->header.numberOfSamples);
		memcpy(puls->stimVoltage, &this->testbed.pulseQueue[this->testbed.indexPulsToReturnNext].stimVoltage,
				sizeof(double)*puls->header.numberOfSamples);
		this->testbed.pulseQueue[this->testbed.indexPulsToReturnNext].header.newData = false;
		this->testbed.pulseQueue[this->testbed.indexPulsToReturnNext].header.dataWasOverriden = 0;
		this->testbed.indexPulsToReturnNext++;
		this->testbed.indexPulsToReturnNext &= TESTBED_PULSE_QUEUE_INDEX_MASK;
		ret = true;
	}

	// unlock the queue
	pthread_mutex_unlock(&(this->testbed_mutex));
	return ret;
}

bool Testbed_Interface::ext_GetConfig(testbed_config_t *config)
{
	memcpy(&config->config, &this->testbed.config, sizeof(initConfig_t));
	config->sampleBufferSize = this->testbed.sampleBufferSize;
	config->sampleFrequency = this->testbed.sampleFrequency;
	return true;
}

stateEXT_t Testbed_Interface::ext_GetState( int32_t WaitTimeout_us )
{
	if (WaitTimeout_us > 0){
		package_t P;
		Testbed_Interface::ClearPackage(&P);

		P.Typ = isCommand;
		P.CommandID = StateCMD;
		P.DataSize = 0;

		Testbed_Interface::SendPackage(&P);

		struct       timeval Time;
		uint64_t     TimeTimeout_us, Time_us;
		// TimeTimeout preparations
		gettimeofday(&Time,NULL);
		Time_us = Time.tv_sec*1000000 + (uint64_t)Time.tv_usec;
		TimeTimeout_us = Time_us + (uint64_t)WaitTimeout_us;;

		while(TimeTimeout_us > Time_us){
			// was the state updatet?
			uint64_t Time_LastStateUpdate = this->StateEXT.LastUpdate.tv_sec*1000000 + (uint64_t)this->StateEXT.LastUpdate.tv_usec;
			if ((Time_us-Time_LastStateUpdate) <= 0) {
				// Status was updated -> abort
				break;
			}
			// wait 5000us
			usleep(500);
			// update the timer
			gettimeofday(&Time,NULL);
			Time_us = Time.tv_sec*1000000 + (uint64_t)Time.tv_usec;
		}
	}

	return this->StateEXT;
}

void Testbed_Interface::ext_UpdateState(package_t *Package)
{
	// update the status time
	gettimeofday(&(this->StateEXT.LastUpdate),NULL);
	if (Package->DataSize >= 2) {
		// update the external status
		convert16_t temp0;
		temp0.data = 0;
		temp0.c_data[0] = Package->Data[0];
		temp0.c_data[1] = Package->Data[1];
		this->StateEXT.State = temp0.data;

		if (Package->DataSize >= 6) {
			// update the external supply voltage
			convert32_t temp;
			temp.data = 0;
			temp.c_data[0] = Package->Data[2];
			temp.c_data[1] = Package->Data[3];
			temp.c_data[2] = Package->Data[4];
			temp.c_data[3] = Package->Data[5];
			this->StateEXT.Voltage = ((double)temp.data) / 1000.0;
		}
	}
	/** project specific implementation **/
	/**
         if (Package->DataSize >= 6) {

        }
	 */
}

void Testbed_Interface::ext_PrintState(void)
{
	// print the external, project specific state values
	//printf("   External XXX: \n");

	printf("\n");
	fflush(stdout);
}

bool Testbed_Interface::ext_IsCommandIDValid(command_t CMD)
{
	switch (CMD){
	case MeasurmentDataACK:         return true;
	case EchoCMD:         			return true;
	case EchoACK:         			return true;

	case ResetCMD:                  return true;
	case ResetACK:                  return true;
	case InitCMD:                   return true;
	case InitACK:                   return true;
	case DeInitCMD:                 return true;
	case DeInitACK:                 return true;
	case StartMeasurmentCMD:        return true;
	case StartMeasurmentACK:        return true;
	case StopMeasurmentCMD:         return true;
	case StopMeasurmentACK:         return true;
	case StateCMD:                  return true;
	case StateACK:                  return true;
	case RepeatLastCommandCMD:      return true;
	case RepeatLastCommandACK:      return true;
	case DebugCMD:                  return true;
	case ErrorCMD:                  return true;
	default:                        return false;
	}
}

const char* Testbed_Interface::ext_GetCommandString(command_t CMD)
{
	switch (CMD){
	case MeasurmentDataACK:			return "MeasurmentACK";

	case ResetCMD:                  return "ResetCMD";
	case ResetACK:                  return "ResetACK";
	case InitCMD:                   return "InitCMD";
	case InitACK:                   return "InitACK";
	case DeInitCMD:                 return "DeInitCMD";
	case DeInitACK:                 return "DeInitACK";
	case StartMeasurmentCMD:        return "StartMeasurmentCMD";
	case StartMeasurmentACK:        return "StartMeasurmentACK";
	case StopMeasurmentCMD:         return "StopMeasurmentCMD";
	case StopMeasurmentACK:         return "StopMeasurmentACK";
	case StateCMD:                  return "StatusCMD";
	case StateACK:                  return "StatusACK";
	case RepeatLastCommandCMD:      return "RepeatLastCommandCMD";
	case RepeatLastCommandACK:      return "RepeatLastCommandACK";
	case DebugCMD:                  return "DebugCMD";
	case ErrorCMD:                  return "ErrorCMD";
	default:                        return "Unknown Command String";
	}
}

const char* Testbed_Interface::ext_GetErrorString(error_t Error)
{
	switch ( Error ) {
	case NoDefinedErrorCode:      return "NoDefinedErrorCode";
	case UnknownCMDError:         return "UnknownCMDError";
	case ChecksumMissmatchError:  return "ChecksumMissmatchError";
	case EndByteMissmatchError:   return "EndByteMissmatchError";
	case UARTSizeMissmatchError:  return "UARTSizeMissmatchError";
	case MaxErrorsEnumID:         return "MaxErrorsEnumID";
	default:                      return "Unknow Error String";
	}
}


} // end namespace
