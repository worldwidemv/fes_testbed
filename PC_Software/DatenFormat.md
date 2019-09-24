# Data Format of the Testbed Standalone Programm


## Version 2
* File Header -> 24 bytes
    * '%%TESTBED DATA\n' -> 14 bytes
    * header[14] = TESTBED_DATA_VERSION;
    * header[15] = TESTBED_DATA_TYPE_STIM_STRUCTS;
    * header[16] = 10; // new line
    * header[17] = (uint8_t)((sampleFrequency >>  0) & 0xFF);
    * header[18] = (uint8_t)((sampleFrequency >>  8) & 0xFF);
    * header[19] = (uint8_t)((sampleFrequency >> 16) & 0xFF);
    * header[20] = (uint8_t)((sampleFrequency >> 24) & 0xFF);

* STIM Puls Header
    * Start Bytes '###' -> 3 bytes
    * uint16_t numberOfSamples -> 2 bytes
    * uint8_t  pulseNotComplete -> 1 byte
    * uint8_t  dataWasOverriden -> 1 byte
    * uint8_t  activeStimChannels -> 1 Bytes; positive und negativ
    * uint8_t  activeDemuxChannels_positiv[16] -> 16 Bytes
    * uint8_t  activeDemuxChannels_negativ[16] -> 16 Bytes

* STIM Pulse Data
    * uint64_t sampleNumber -> 4*numberOfSamples Bytes
    * int16_t stimCurrent -> 2*numberOfSamples Bytes; 
    * int16_t stimVoltage -> 2*numberOfSamples Bytes;
