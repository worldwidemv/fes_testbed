function [ this ] = tb_receiveTestbedData( )
this = create_Object();

    function o = create_Object()
        o = struct();
        %% public/external API
        o.connectToTB       = @connectToTB;
        o.disConnectFromTB  = @disConnectFromTB;
        o.updateTBdata      = @updateTBdata;
        o.delete            = @deleteObj;
        
        
        o.setup = struct();
        o.setup.ip_address = '127.0.0.1';
        o.setup.port = 33111;
        
        o.state = struct();
        o.state.connected = false;
        o.state.headerReceived = false;
        o.state.warningDisplayed = false;
        
        o.tcpCon = [];
    end

    function ret = connectToTB()
        ret = false;
        try
            this.tcpCon = tcpclient(this.setup.ip_address, this.setup.port);
            this.state.connected = true;
            ret = true;
        catch ME
            if (strcmpi(ME.identifier, 'MATLAB:networklib:tcpclient:cannotCreateObject'))
                % ignore error , just keep trying
                if (~this.state.warningDisplayed)
                    warning(['Connection to ', this.setup.ip_address, ' could not be established!']);
                    this.state.warningDisplayed = true;
                end
                this.state.connected = false;
            else
                disp(['ID: ' ME.identifier])
                rethrow(ME)
            end
        end
    end

    function disConnectFromTB()
        if (this.state.connected)
            this.tcpCon.delete();
        end
        this.state.connected = false;
        this.state.headerReceived = false;
    end

    function deleteObj()
        disConnectFromTB();
    end


    function d = updateTBdata(d)
        nPulsesOld = d.nPulses;
        try
            if (~this.state.connected)
                % not connected -> connect to the network interface
                if (connectToTB())
                    d = readTestbedHeader(d);
                end
            else
                % connected -> read the data from the network interface
                if (this.state.headerReceived)
                    d= readTestbedData(d);
                else
                    d = readTestbedHeader(d);
                end
            end
            
            if (d.nPulses > nPulsesOld)
                d = simplePostProcessing( d, nPulsesOld +1 );
            end            
        catch ME
            disp(['ID: ' ME.identifier])
            rethrow(ME)
        end
    end



%% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  callbacks and internal functions
%% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    function d = readTestbedHeader(d)
        
        param = d.setup;
        param.fname = 'LiveData';
        param.fpath = ['http://', this.setup.ip_address, ':', this.setup.port, '/'];
        param.iStart = 1;
        param.iEnd = 100;
        param.nAll = [];
        if (isfield(param, 'fPositionDataFile'))
            param = rmfield(param, 'fPositionDataFile');
        end
        
        %% check that this the a testbed data file
        dataType = read(this.tcpCon, 13, 'uint8');
        dataTypeStr = char(dataType);
        if (strcmp(dataTypeStr, '%TESTBED DATA'))
            read(this.tcpCon, 1, 'uint8');
            param.dataVersion = read(this.tcpCon, 1, 'uint8');
        else
            error(['The LIVE data is not a Testbed data file version 2 or above!']);
        end
        param.dataType = read(this.tcpCon, 1, 'uint8');
        read(this.tcpCon, 1, 'uint8');
        param.f_sample = read(this.tcpCon, 1, 'uint32');
        switch (param.dataVersion)
            case 2
                read(this.tcpCon, 3, 'uint8');
        end
        this.state.headerReceived = true;
        
        if (param.dataType == 1)
            disp([char(10), 'TESTBED LIVE DATA: connected to ', this.setup.ip_address]);
            d.setup = param;
            d = readTestbedData(d);
        else
            error('Only StimStructs are supported in LiveData mode!');
        end
    end

    function d = readTestbedData(d)
        pulseComplete = true;
        iPulse = 0;
        
        %% read one container
        % search the start bytes
        while (this.tcpCon.BytesAvailable > 3)
            bytesAvailable = this.tcpCon.BytesAvailable;
            StartBytes = read(this.tcpCon, 3, 'uint8');
            if (isempty(StartBytes))
                disp('no data ...');
                return;
            end
            
            if (StartBytes(1) == 35 && StartBytes(3) == 35)
                try
                    % start bytes were found
                    % make shure we have enougth data
                    %while (bytesAvailable < (5 + nSamples*4*gData.TBData.setup.nCh))
                    %    pause(0.005);
                    %    gData.TBData.setup.timeoutCounter = gData.TBData.setup.timeoutCounter -1;
                    %    if (gData.TBData.setup.timeoutCounter <= 0), break; end
                    %    bytesAvailable = this.tcpCon.BytesAvailable;
                    %end
                    %gData.TBData.setup.timeoutCounter = 20;
                    
                    % start bytes were found
                    if (pulseComplete)
                        nSamples = 0;
                        nSample = [];
                        stimCurrentRaw = [];
                        stimVoltageRaw = [];
                    end
                    % get number of samples
                    nSamples = nSamples + uint32(read(this.tcpCon, 1, 'uint16'));
                    pulseNotComplete = read(this.tcpCon, 1, 'uint8');
                    if (pulseNotComplete)
                        disp(['   -> pulse ', num2str(iPulse), ' exeeded the max input sample size ...']);
                    end
                    dataFailures = read(this.tcpCon, 1, 'uint8');
                    dataWasOverriden = false; dataWasDiscarded = false;
                    if (dataFailures == 1), dataWasOverriden = true; end
                    if (dataFailures == 2), dataWasDiscarded = true; end
                    activeStimChannels_P = read(this.tcpCon, 1, 'uint8');
                    activeStimChannels_N = read(this.tcpCon, 1, 'uint8');
                    activeDemuxChannels_P = read(this.tcpCon, 16, 'uint8');
                    activeDemuxChannels_N = read(this.tcpCon, 16, 'uint8');
                    nSample    = [nSample;     read(this.tcpCon, nSamples, 'uint64')];
                    stimCurrentRaw= [stimCurrentRaw; read(this.tcpCon, nSamples, 'int16')];
                    stimVoltageRaw= [stimVoltageRaw; read(this.tcpCon, nSamples, 'int16')];
                    
                    %% postprocessing of the raw data if the pulse is complete
                    if (pulseComplete)
                        if (isempty(stimVoltageRaw) || isempty(stimCurrentRaw)), continue; end
                        d.nPulses = d.nPulses +1;
                        % convert the raw data
                        [stimVoltage, stimCurrent] = convertData(stimVoltageRaw, stimCurrentRaw);
                        % get the active channels
                        aCH = getActiveChannels(activeStimChannels_P, activeStimChannels_N, activeDemuxChannels_P, activeDemuxChannels_N);
                        % update the data struct
                        d.pulseRaw(d.nPulses).time = double(nSample') ./ double(d.setup.f_sample);
                        d.pulseRaw(d.nPulses).nSamples = nSample';
                        d.pulseRaw(d.nPulses).stimVoltage = stimVoltage';
                        d.pulseRaw(d.nPulses).stimCurrent = stimCurrent';
                        d.pulseRaw(d.nPulses).stimVoltageRaw = stimVoltageRaw';
                        d.pulseRaw(d.nPulses).stimCurrentRaw = stimCurrentRaw';
                        d.pulseRaw(d.nPulses).activeChannel = aCH;
                        d.pulseRaw(d.nPulses).dataWasDiscarded = dataWasDiscarded;
                        d.pulseRaw(d.nPulses).dataWasOverriden = dataWasOverriden;
                        d.pulseRaw(d.nPulses).stats = struct();
                    end
                catch ME
                    %disp(['ID: ' ME.identifier])
                    disp(['#### Incomplete ReadIn: nBytesAvailabe:', num2str(bytesAvailable), '; nBytesSoll:', num2str(5 +nSamples*10), ...
                        ' -->  nSamples:', num2str(nSamples)])
                    rethrow(ME);
                end
            else
                warning(['Start Bytes were not found after ', num2str(d.nPulses), ' pulses!']);
                break;
            end
        end
    end

    function [voltage, current] = convertData(voltageRaw, currentRaw)
        % Convert the int16 data into double
        TESTBED_CONVERT_ADC_DATA__INPUT_VOLTAGE_MAX     = 2.5;
        TESTBED_CONVERT_ADC_DATA__ADC_MAX_CODE          = 65535.0;
        TESTBED_CONVERT_ADC_DATA__LSB_TO_V              = (TESTBED_CONVERT_ADC_DATA__INPUT_VOLTAGE_MAX / TESTBED_CONVERT_ADC_DATA__ADC_MAX_CODE);
        
        TESTBED_CONVERT_ADC_DATA__I_R                   = 6.79;
        TESTBED_CONVERT_ADC_DATA__SCALE_FACTOR_I        = ((1/TESTBED_CONVERT_ADC_DATA__I_R) * 1000); % current in mA
        TESTBED_CONVERT_ADC_DATA__V_R1                  = 1492.0;
        TESTBED_CONVERT_ADC_DATA__V_R2                  = 9.981;
        TESTBED_CONVERT_ADC_DATA__SCALE_FACTOR_V        = ((TESTBED_CONVERT_ADC_DATA__V_R1 + TESTBED_CONVERT_ADC_DATA__V_R2) / TESTBED_CONVERT_ADC_DATA__V_R2);
        
        TESTBED_OFFSET_CURRENT      = 45.5;
        TESTBED_OFFSET_VOLTAGE      = 55.5;
        % ch1 - stim current
        current = double(currentRaw - TESTBED_OFFSET_CURRENT) * TESTBED_CONVERT_ADC_DATA__LSB_TO_V * TESTBED_CONVERT_ADC_DATA__SCALE_FACTOR_I;
        % ch2 - stim voltage
        voltage = double(voltageRaw - TESTBED_OFFSET_VOLTAGE) * TESTBED_CONVERT_ADC_DATA__LSB_TO_V * TESTBED_CONVERT_ADC_DATA__SCALE_FACTOR_V;
    end

    function activeChannels = getActiveChannels(activeStimChannels_P, activeStimChannels_N, activeDemuxChannels_P, activeDemuxChannels_N)
        activeStimChannel = [];
        % get the active channels
        for i=1:4
            if (bitand(activeStimChannels_P, pow2(i-1)))
                activeStimChannel(end+1) = +i;
            end
            if (bitand(activeStimChannels_N, pow2(i-1)))
                activeStimChannel(end+1) = i *-1;
            end
        end
        activeChannels = struct('aCH_Stim', activeStimChannel, 'aCH_Demux', []);
    end

    function [ d ] = simplePostProcessing( d, iStart )
        % update the pulses / search for the pulse and cleanup the data
        for i = iStart:length(d.pulseRaw)
            % search the pulse/pulses in the data
            d = updateFullPulseStruct( d, i );
        end
    end

    function [ d ] = updateFullPulseStruct( d, iRawPulse )
        pulses = tbi_searchPulse_UseFullData( d.pulseRaw(iRawPulse).stimCurrent, d.pulseRaw(iRawPulse).activeChannel.aCH_Stim, d.setup );
        % add pulse to list
        if (iRawPulse==1 && isempty(fieldnames(d.pulse)))
            d.pulse = pulses(1);
        else
            d.pulse(iRawPulse) = pulses(1);
        end
        d.pulse(iRawPulse).time          = d.pulseRaw(iRawPulse).time;
        d.pulse(iRawPulse).stimCurrent   = d.pulseRaw(iRawPulse).stimCurrent;
        d.pulse(iRawPulse).stimVoltage   = d.pulseRaw(iRawPulse).stimVoltage;
        d.pulse(iRawPulse).nSample      = d.pulseRaw(iRawPulse).nSamples;
        d.pulse(iRawPulse).info.dataWasDiscarded = d.pulseRaw(iRawPulse).dataWasDiscarded;
        d.pulse(iRawPulse).info.dataWasOverriden = d.pulseRaw(iRawPulse).dataWasOverriden;
        d.pulse(iRawPulse).info.sourcePulseRaw = iRawPulse;
    end




end