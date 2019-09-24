#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <limits.h>

#define TESTBED_LOG_HEADER_SIZE				24
#define TESTBED_DATA_VERSION				2
#define TESTBED_LOG_NAME_RAW				"./Testbed_Log_T%d_%s__%d.dat"
#define TESTBED_LOG_MAX_FILENUMBER			1000

#define TESTBED_DATA_TYPE_STIM_STRUCTS		1
#define TESTBED_DATA_TYPE_STREAM_COMPRESSED 2
#define TESTBED_DATA_TYPE_STREAM_FULL		3

#define FALSE   		0
#define TRUE    		1

#include "Testbed_Interface_V1.h"
#include "TCP_Sender.hpp"

using namespace std;
using namespace ns_Testbed_01;

// variables
bool doAbortProgram;


void abort_program(int sig) {
	doAbortProgram = true;
}

void write_stimStructHeader(FILE *dFile, uint32_t sampleFrequency)
{
	uint8_t header[TESTBED_LOG_HEADER_SIZE];
	sprintf((char*)header, "%%TESTBED DATA\n");
	header[14] = TESTBED_DATA_VERSION;
	header[15] = TESTBED_DATA_TYPE_STIM_STRUCTS;
	header[16] = 10; // new line
	header[17] = (uint8_t)((sampleFrequency >>  0) & 0xFF);
	header[18] = (uint8_t)((sampleFrequency >>  8) & 0xFF);
	header[19] = (uint8_t)((sampleFrequency >> 16) & 0xFF);
	header[20] = (uint8_t)((sampleFrequency >> 24) & 0xFF);

	fwrite(header, sizeof(uint8_t), TESTBED_LOG_HEADER_SIZE, dFile);
}

void send_stimStructHeader(TCP_Sender *TCPSender, uint32_t sampleFrequency)
{
	uint8_t header[TESTBED_LOG_HEADER_SIZE];
	sprintf((char*)header, "%%TESTBED DATA\n");
	header[14] = TESTBED_DATA_VERSION;
	header[15] = TESTBED_DATA_TYPE_STIM_STRUCTS;
	header[16] = 10; // new line
	header[17] = (uint8_t)((sampleFrequency >>  0) & 0xFF);
	header[18] = (uint8_t)((sampleFrequency >>  8) & 0xFF);
	header[19] = (uint8_t)((sampleFrequency >> 16) & 0xFF);
	header[20] = (uint8_t)((sampleFrequency >> 24) & 0xFF);

	TCPSender->setHeader((char*)header, TESTBED_LOG_HEADER_SIZE);
}
void write_stimStruct(FILE *dFile, testbed_stimulationPulse_t *pulse)
{
	if (pulse->header.newData){
		fwrite(&pulse->header, sizeof(uint8_t), (sizeof(testbed_stimulationPulseHeader_t)-sizeof(bool)), dFile);
		fwrite(pulse->sampleNumber, sizeof(uint64_t), pulse->header.numberOfSamples, dFile);
		fwrite(pulse->stimCurrent, sizeof(int16_t), pulse->header.numberOfSamples, dFile);
		fwrite(pulse->stimVoltage, sizeof(int16_t), pulse->header.numberOfSamples, dFile);
	}
	pulse->header.newData = false;
}
void send_stimStruct(TCP_Sender *TCPSender, testbed_stimulationPulse_t *pulse)
{
	if (pulse->header.newData){
		TCPSender->addData( (char*)&pulse->header, (sizeof(testbed_stimulationPulseHeader_t)-sizeof(bool)) );
		TCPSender->addData((char*)pulse->sampleNumber, sizeof(uint64_t)*pulse->header.numberOfSamples);
		TCPSender->addData((char*)pulse->stimCurrent, sizeof(int16_t)*pulse->header.numberOfSamples);
		TCPSender->addData((char*)pulse->stimVoltage, sizeof(int16_t)*pulse->header.numberOfSamples);
	}
	pulse->header.newData = false;
	TCPSender->sendData();
	printf("#### -> send data\n");
}


void write_streamDataHeader(FILE *dFile, uint8_t type, uint32_t sampleFrequency)
{
	uint8_t header[TESTBED_LOG_HEADER_SIZE];
	sprintf((char*)header, "%%Testbed Data\n");
	header[14] = type;
	header[15] = 10; // new line
	fwrite(header, sizeof(uint8_t), TESTBED_LOG_HEADER_SIZE, dFile);

}
void write_streamData(FILE *dFile, testbed_streamData_t *data)
{
	struct __packed {
		double time;
		double current;
		double voltage;
		uint64_t sampleNumber;
		uint32_t packageCounter;
	} dataToWrite;

	if (data->isNew){
		//printf("## %fsec %ld %fV %fV %d %d\n", data->samples[0].time, data->samples[0].samplesCounter, data->samples[0].current, data->samples[0].voltage, data->packageCounter, data->numberOfSamples);
		for (uint32_t i = 0; i< data->numberOfSamples; i++){
			dataToWrite.time = data->samples[i].time;
			dataToWrite.current = data->samples[i].current;
			dataToWrite.voltage = data->samples[i].voltage;
			dataToWrite.sampleNumber = data->samples[i].samplesCounter;
			dataToWrite.packageCounter = data->packageCounter;
			fwrite(&dataToWrite, sizeof(uint8_t), sizeof(dataToWrite), dFile);
		}
	}
	data->isNew = false;
}


int main ( int argc, char *argv[] )
{
	(void) signal(SIGINT, abort_program);

	char *DeviceName;
	int   ValuesToGet, dataType, samplerRateDivider;
	char NameTemp[100], dataTypeStr[50];
	FILE *fd_data;
	struct stat fd_stats;

	struct timeval start,stop,last_read;
	long u1,u2;

	bool useMS = true, dataAvailabe = false;
	testbed_stimulationPulse_t stimPuls;
	testbed_streamData_t streamData;

	if ( argc < 2 ){ // at least 1 argument
		// We print argv[0] assuming it is the program name
		cout<<"\nData logger for FES stimulation\n  usage: "<< argv[0] <<" <devicename> [divider (sample rate = 1MSample/divider)] [seconds to measure OR -value for samples] [data type (stimStructs=1 (default); compressed streamData=2; full streamData=3]\n";
		return -1;
	}
	else {
		// We assume argv[1] is a filename to open
		DeviceName = argv[1];
		if (stat(DeviceName, &fd_stats) == -1){
			fprintf(stderr, "\nError: Device '%s': %s\n\n", DeviceName, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	// optional
	samplerRateDivider = 1; // 1MSample as default
	if ( argc >= 3 ){
		samplerRateDivider = atoi(argv[2]);
		if (samplerRateDivider <= 0 || samplerRateDivider > 255){
			fprintf(stderr, "\nError: the 'sample rate divider' must be between 1 and 255 (sample rate = 1MSample / divider)\n\n");
			exit(EXIT_FAILURE);
		}
	}

	ValuesToGet = 10*60*60; // 10 hours default measurement time
	if ( argc >= 4 ){
		ValuesToGet = atoi(argv[3]);
	}
	if (ValuesToGet < 0){
		// ValuesToGet is in samples
		ValuesToGet = ValuesToGet *-1;
		useMS = false;
	} else {
		// ValuesToGet is in seconds
		if (ValuesToGet == 0){
			// reset the default long measurement time
			ValuesToGet = 10*60*60; // 10 hours default measurement time
		}
		ValuesToGet = ValuesToGet *1000; // convert to ms
		useMS = true;
	}

	dataType = 1; // stimStructs as default
	if ( argc >= 5 ){
		dataType = atoi(argv[4]);
	}

	// build the configuration
	initConfig_t InitConfig;

	switch (dataType){
	case TESTBED_DATA_TYPE_STIM_STRUCTS:
		sprintf(dataTypeStr, "stimStruct");
		InitConfig.dataPackageType = tb_stimDataOnly;
		InitConfig.outputType = tb_outputStimDataStructs;
		break;
	case TESTBED_DATA_TYPE_STREAM_COMPRESSED:
		sprintf(dataTypeStr, "streamCompressed");
		InitConfig.dataPackageType = tb_stimDataOnly;
		InitConfig.outputType = tb_outputStreamDataCompressed;
		break;
	case TESTBED_DATA_TYPE_STREAM_FULL:
		printf("\n\nFULL DATA STREAMS ARE NOT WORKING YET....\n");
		exit(EXIT_FAILURE); // TODO: support full data streams
		sprintf(dataTypeStr, "streamFull");
		InitConfig.dataPackageType = tb_constStream;
		InitConfig.outputType = tb_outputStreamDataFull;
		break;
	default:
		fprintf(stderr, "\nError: 'data type' must be 1=stim data only (as struct), 2=compressed stream data, or 3=full stream data\n\n");
		exit(EXIT_FAILURE);
	}

	// Main program
	// ##############################################################################################
	printf("Data Logger for the Stimulation Testbed\n==============================================\n");
	doAbortProgram = false;

	// INIT
	// Open the Device
	Testbed_Interface *Device = new Testbed_Interface((char*)"STIM Logger", DeviceName);   // Create Device Class

	// START
	gettimeofday(&start,NULL);
	u1 = start.tv_sec*1000 + start.tv_usec/1000;

	InitConfig.demuxEnabled = false;
	InitConfig.demuxChannelsActive = 0;
	InitConfig.demuxChannelsPassive = 0;
	// CH1 = Current
	InitConfig.thresholdCh1Positiv  = 45 + 100;
	InitConfig.thresholdCh1Negative = 45 - 100;
	// CH2 = Voltage
	InitConfig.thresholdCh2Positiv  = 55 + 330;
	InitConfig.thresholdCh2Negative = 55 - 330;

	//InitConfig.thresholdCh2Positiv  = 55 + 2000;
	//InitConfig.thresholdCh2Negative = 55 - 2000;

	InitConfig.searchWidth = 0;		// internal reset to the default
	InitConfig.samplePeriodeMultiplier = (uint8_t) samplerRateDivider;

	// open the device
	if (Device->isDeviceOpen()){
		// initialisation
		if (Device->ext_SendInit(&InitConfig)){
			// initialisation ACK
			gettimeofday(&stop,NULL);
			u2 = stop.tv_sec*1000.0 + stop.tv_usec/1000.0;
			//printf("MAIN: Initialisation successful! (took %fms)\n\n", (double)(u2-u1));
		} else {
			printf("MAIN: Initialisation failed! -> exit\n\n");
			delete Device;
			exit(EXIT_FAILURE);
		}
		// End Init
	} else {
		printf("MAIN: The device '%s' could not be opened! -> exit\n\n", NameTemp);
		exit(EXIT_FAILURE);
	} // end Initialisation


	// Start the Log file
	// -> find filename
	for (uint16_t i = 1; i < TESTBED_LOG_MAX_FILENUMBER; i++){
		sprintf(NameTemp, TESTBED_LOG_NAME_RAW, dataType, dataTypeStr, i);
		if (stat(NameTemp, &fd_stats) == -1){
			if (errno == ENOENT){
				break;
			} else {
				fprintf(stderr, "\nError: Output file '%s': %s\n\n", NameTemp, strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
	}
	fd_data = fopen(NameTemp, "w");
	if (fd_data < NULL){
		fprintf(stderr, "\nError: Output file '%s': %s\n\n", NameTemp, strerror(errno));
		exit(EXIT_FAILURE);
	}


	TCP_Sender *TCPSender = new TCP_Sender(33111);
	TCPSender->poll();

	// -> write the header
	testbed_config_t TestbedConfig;
	Device->ext_GetConfig(&TestbedConfig);
	uint32_t sampleFequency = (uint32_t)TestbedConfig.sampleFrequency;
	switch (dataType){
	case TESTBED_DATA_TYPE_STIM_STRUCTS:
		write_stimStructHeader(fd_data, sampleFequency);
		send_stimStructHeader(TCPSender, sampleFequency);
		break;
	case TESTBED_DATA_TYPE_STREAM_COMPRESSED:
		write_streamDataHeader(fd_data, TESTBED_DATA_TYPE_STREAM_COMPRESSED, sampleFequency);
		break;
	case TESTBED_DATA_TYPE_STREAM_FULL:
		write_streamDataHeader(fd_data, TESTBED_DATA_TYPE_STREAM_FULL, sampleFequency);
		break;
	}


	usleep(200000); // wait 200ms
	// send StartMeasurment command
	if (!Device->ext_StartMeasurment(0)){
		printf("MAIN: MeasurmentStart ACK missing ...\n");
	}


	/*
	 * MEASUREMENT LOOP
	 */
	printf("\n   --> writing the data to %s\n", NameTemp);
	gettimeofday(&stop,NULL);
	last_read = stop;
	u1 = stop.tv_sec*1000 + stop.tv_usec/1000;
	printf("   -->  0 %% done ....");
	fflush(stdout);

	unsigned int counter = 0, dataCounter = 0, packageCounter = 0;
	long int time_old = stop.tv_sec -1;
	int prozent, prozent_old=0, watchdog = 0;

	while( (counter <= (unsigned int)ValuesToGet) && !doAbortProgram )
	{
		gettimeofday(&stop,NULL);
		u2 = stop.tv_sec*1000 + stop.tv_usec/1000;
		if (useMS){
			counter = (unsigned int)(u2-u1);
		} else {
			counter = dataCounter;
		}

		do {
			switch (dataType){
			case TESTBED_DATA_TYPE_STIM_STRUCTS:
				dataAvailabe = Device->ext_GetStimPuls(&stimPuls);
				dataCounter = stimPuls.header.numberOfSamples;
				if (stimPuls.header.newData){
					packageCounter++;
					write_stimStruct(fd_data, &stimPuls);
					stimPuls.header.newData = true;
					send_stimStruct(TCPSender, &stimPuls);
				}
				break;
			case TESTBED_DATA_TYPE_STREAM_COMPRESSED:
			case TESTBED_DATA_TYPE_STREAM_FULL:
				dataAvailabe = Device->ext_GetStreamData(&streamData);
				dataCounter = streamData.numberOfSamples;
				if (streamData.isNew){
					packageCounter++;
					write_streamData(fd_data, &streamData);
				}
				break;
			}
			if (dataAvailabe){
				watchdog = 0;
			}
		} while (dataAvailabe);

		prozent = (int)((100.0/(double)ValuesToGet)*(double)counter);
		if ((prozent != prozent_old) || (time_old != stop.tv_sec)) {
			printf("\r   --> %2d ms/samples (%d%%; Packages: %d) | %1.2F sec ....", counter, prozent, packageCounter, (((double)(u2-u1)) / 1000.0));
			fflush(stdout);
			prozent_old = prozent;
			time_old = stop.tv_sec;
		}

		watchdog++;
		if (watchdog >= 200){
			printf("\n\e[1m\e[91mMAIN: Watchdog triggered:\e[0m Did NOT received any data for 2 sec! -> ABORT");
			break;
		}

		// run ASIO callbacks
		TCPSender->poll();

		usleep(10000); // 10ms
	}
	printf("\n\n");


	// send StopMeasurment command
	if (!Device->ext_StopMeasurment()){
		printf("MAIN: MeasurmentStop ACK missing ...\n");
	}

	// send DeInit command
	if (!Device->ext_SendDeInit()){
		printf("MAIN: Device deinitialision ACK missing ...\n");
	}
	// free memory
	delete TCPSender;
	delete Device;

	// END
	fflush(fd_data);
	fclose(fd_data);

	gettimeofday(&stop,NULL);
	u2 = stop.tv_sec*1000 + stop.tv_usec/1000;

	printf("\nExit ---> Execution Time: %f seconds\n",(u2-u1)/1000.0 );
	exit(EXIT_SUCCESS);
}
