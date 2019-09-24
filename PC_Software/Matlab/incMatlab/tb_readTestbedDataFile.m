function [data, timeNeeded] = tb_readTestbedDataFile(filename, setup)
%Reads the binary TUB Testbed data file and returns the raw data as Matlab struct.
%
%  [data] = readTestbedDataFile(filename) reads
%  the file specified by 'filename' and outputs the raw data as struct, one
%  entry per pulse.

%% defaults
if (~exist('setup', 'var'))
    setup = struct('iStart', 1, 'N', 1200, 'nAll', [], 'pulseType', 0, 'skipDesc', false);
else
    if (~isfield(setup, 'iStart')), setup.iStart = 1; end
    if (~isfield(setup, 'N')),      setup.N = 1200; end
    if (~isfield(setup, 'nAll')), setup.nAll = []; end
    if (~isfield(setup, 'pulseType')), setup.pulseType = 0; end
    if (~isfield(setup, 'skipDesc')), setup.skipDesc = false; end
end

param = struct( ...
    'iStart', setup.iStart, ...
    'iN', setup.N, ...
    'iEnd', setup.iStart +setup.N -1, ...
    'nAll', setup.nAll, ...
    'relativeTimeVectorInUS', false, ...
    'f_sample', 1000000, ...
    'f_downsample', 100000, ...
    'minWidthPulsePart_us', 20, ...
    'maxInnerPulseZeroTime_us', 150, ...
    'threshold', 2.5, ...
    'pulseAddonStart', 100, ...
    'pulseAddonEnd', 300, ...
    'debug', false, ...
    'dataType', [], ...
    'pulseType', setup.pulseType ...
    );
if (isfield(setup, 'fPositionDataFile')), param.fPositionDataFile = setup.fPositionDataFile; end


%% open file
if (~exist('filename', 'var') || isempty(filename) || ~exist(filename, 'file'))
    [FileName,PathName] = uigetfile({'*.dat';'*.mat'},'Select a Testbed data file to convert or a allready converted file.');
    filename = [PathName, FileName];
end
if (isnumeric(filename(1)))
    timeNeeded = 0;
    data = struct('nPulses', 0, 'pulseRaw', []);
    return;
end
[PathName,FileName,ext] = fileparts(filename);
if (strcmpi(ext,'.mat'))
    % the file is already a *.mat file -> just load this file
    tic();
    load(filename); % data comes from the file
    if (~isfield(data.setup, 'fext')), data.setup.fext = '.mat'; end
    timeNeeded = toc();
    return;
end

data = struct();
param.fname = FileName;
param.fpath = PathName;
param.fext  = ext;

[fid, errorMsg] = fopen(filename, 'rb');
if (fid == -1)
    error(['fopen for ', filename, ' failed: ', errorMsg]);
end

disp(['TESTBED Data Conversion', char(10), '   -> converting file "', filename, '"']);

%% DataSet Description
if (~setup.skipDesc)
    param.desc = inputdlg('Description for this Testbed data set:                                    |', 'Data Set Description', 7, {''}, 'on');
end

%% read in the file
tic;

%% check that this the a testbed data file
dataType = fread(fid, 13, 'uint8');
dataTypeStr = char(dataType');
disp(dataTypeStr)
if (strcmp(dataTypeStr, '%Testbed Data'))
    fread(fid, 1, 'uint8');
    param.dataVersion = 1;
else
    if (strcmp(dataTypeStr, '%TESTBED DATA'))
        fread(fid, 1, 'uint8');
        param.dataVersion = fread(fid, 1, 'uint8');
    else
        error(['The file "', filename, ' is not a Testbed data file!']);
    end
end

type = fread(fid, 1, 'uint8');
fread(fid, 1, 'uint8');
param.f_sample = fread(fid, 1, 'uint32');
switch (param.dataVersion)
    case 1
        fread(fid, 4, 'uint8');
    case 2
        disp(fread(fid, 3, 'uint8'));
end

switch (type)
    case 1
        disp('   -> data type "stimStructs" -> creating a struct with the detected stimulation pulses');
        data = read_stimStructs(fid, type, param);
    case 2
        disp('   -> data type "compressed stream data" -> creating a struct with one time, current, and voltage vector');
        data = read_streamDataCompressed(fid, type, param);
    case 3
        disp('   -> data type "compressed stream data" -> creating a struct with one time, current, and voltage vector');
        data = read_streamDataCompressed(fid, type, param);
    otherwise
        error(['Unknown data type ', num2str(type), '!']);
end


%% Close the file
fclose(fid);

%% get statistics
timeNeeded = toc;

switch (type)
    case 1
        disp(['   -> conversion completed: ', num2str(data.nPulses), ' pulses found, took ', num2str(timeNeeded), 'sec']);
    case 2
        disp(['   -> conversion completed: ', num2str(data.nSamples), ' samples found, took ', num2str(timeNeeded), 'sec']);
    case 3
        
    otherwise
        error(['Unknown data type ', num2str(type), '!']);
end


%% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  callbacks and internal functions
%% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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

    function [pulseStruct] = read_stimStructs(fid, type, param)
        pulseStruct = struct('nPulses', 0, 'pulseRaw', struct());
        pulseComplete = true;
        iPulseGlobal = 0;
        iPulse = 0;
        
        if (param.iStart ~= 1 && isfield(param, 'fPositionDataFile'))
            if (param.fPositionDataFile > 0)
                fseek(fid, param.fPositionDataFile, 'bof');
                iPulseGlobal = param.iStart -1;
                disp(['   -> Jumping to pulse ', num2str(param.iStart -1)]);
            end
        end
        
        while (~feof(fid))
            %% read one pulse
            % search the start bytes
            StartBytes = fread(fid, 3, 'uint8');
            if (isempty(StartBytes))
                break;
            end
            if (StartBytes(1) == 35 && StartBytes(3) == 35)
                % start bytes were found
                if (pulseComplete)
                    iPulseGlobal = iPulseGlobal +1;
                    nSamples = 0;
                    nSample = [];
                    stimCurrentRaw = [];
                    stimVoltageRaw = [];
                end
                % get number of samples
                nSamples = nSamples + fread(fid, 1, 'uint16');
                pulseNotComplete = fread(fid, 1, 'uint8');
                if (pulseNotComplete)
                    disp(['   -> pulse ', num2str(iPulse), ' exeeded the max input sample size ...']);
                end
                dataFailures = fread(fid, 1, 'uint8');
                dataWasOverriden = false; dataWasDiscarded = false;
                if (dataFailures == 1), dataWasOverriden = true; end
                if (dataFailures == 2), dataWasDiscarded = true; end
                activeStimChannels_P = fread(fid, 1, 'uint8');
                activeStimChannels_N = fread(fid, 1, 'uint8');
                activeDemuxChannels_P = fread(fid, 16, 'uint8');
                activeDemuxChannels_N = fread(fid, 16, 'uint8');
                nSample    = [nSample;     fread(fid, nSamples, 'uint64')];
                if (param.dataVersion == 1)
                    fread(fid, nSamples, 'float');
                end
                stimCurrentRaw= [stimCurrentRaw; fread(fid, nSamples, 'int16')];
                stimVoltageRaw= [stimVoltageRaw; fread(fid, nSamples, 'int16')];
                
                %% postprocessing of the raw data if the pulse is complete
                if (pulseComplete)
                    if (isempty(stimVoltageRaw) || isempty(stimCurrentRaw)), continue; end
                    if (iPulseGlobal >= param.iStart && iPulseGlobal <= param.iEnd)
                        iPulse = iPulse +1;
                        % convert the raw data
                        [stimVoltage, stimCurrent] = convertData(stimVoltageRaw, stimCurrentRaw);
                        % get the active channels
                        aCH = getActiveChannels(activeStimChannels_P, activeStimChannels_N, activeDemuxChannels_P, activeDemuxChannels_N);
                        % update the data struct
                        pulseStruct.pulseRaw(iPulse).time = nSample ./ param.f_sample;
                        pulseStruct.pulseRaw(iPulse).nSamples = nSample;
                        pulseStruct.pulseRaw(iPulse).stimVoltage = stimVoltage;
                        pulseStruct.pulseRaw(iPulse).stimCurrent = stimCurrent;
                        pulseStruct.pulseRaw(iPulse).stimVoltageRaw = stimVoltageRaw;
                        pulseStruct.pulseRaw(iPulse).stimCurrentRaw = stimCurrentRaw;
                        pulseStruct.pulseRaw(iPulse).activeChannel = aCH;
                        pulseStruct.pulseRaw(iPulse).dataWasDiscarded = dataWasDiscarded;
                        pulseStruct.pulseRaw(iPulse).dataWasOverriden = dataWasOverriden;
                        pulseStruct.pulseRaw(iPulse).stats = struct();
                        param.fPositionDataFile = ftell(fid);
                    else
                        if (iPulseGlobal > param.iEnd)
                            if (~isempty(param.nAll))
                                %% 
                                break;
                            end
                        end
                    end
                end
                
            else
                warning(['Start Bytes were not found after ', num2str(iPulseGlobal), ' pulses!']);
                break;
            end
        end
        
        if (~isfield(pulseStruct, 'pulseRaw'))
            pulseStruct.nPulses = 0;
        else
            pulseStruct.nPulses = length(pulseStruct.pulseRaw);
        end
        if (iPulseGlobal < param.iEnd)
            param.iEnd = iPulseGlobal;
        end
        if (isempty(param.nAll))
            param.nAll = iPulseGlobal;
        end
        pulseStruct.setup = param;
        pulseStruct.setup.dataType = type;
    end

    function [stream] = read_streamDataCompressed(fid, type, param)
        stream = struct('type', type, 'nSamples', 0, 'time', [], 'stimCurrentRaw', [], 'stimVoltageRaw', [], 'nSample', [], 'packageNumber', [], 'activeStimChannel', [], 'activeDemuxChannels', []);
        i = 0;
        
        try
            while (~feof(fid))
                %% read one sample
                i = i +1;
                % get number of samples
                stream.time(i) = fread(fid, 1, 'float');
                stream.stimCurrentRaw(i) = fread(fid, 1, 'int16');
                stream.stimVoltageRaw(i) = fread(fid, 1, 'int16');
                stream.nSample(i) = fread(fid, 1, 'uint64');
                stream.packageNumber(i) = fread(fid, 1, 'uint32');
                %activeStimChannels_P = fread(fid, 1, 'uint8');
                %activeStimChannels_N = fread(fid, 1, 'uint8');
                %activeDemuxChannels_P = fread(fid, 16, 'uint8');
                %activeDemuxChannels_N = fread(fid, 16, 'uint8');
                activeStimChannels_P = 0;
                activeStimChannels_N = 0;
                % get the active channels
                if (bitand(activeStimChannels_P, 1))
                    stream(i).activeStimChannel(end+1) = +1;
                end
                if (bitand(activeStimChannels_N, 1))
                    stream(i).activeStimChannel(end+1) = -1;
                end
                if (bitand(activeStimChannels_P, 2))
                    stream(i).activeStimChannel(end+1) = +2;
                end
                if (bitand(activeStimChannels_N, 2))
                    stream(i).activeStimChannel(end+1) = -2;
                end
                if (bitand(activeStimChannels_P, 4))
                    stream(i).activeStimChannel(end+1) = +3;
                end
                if (bitand(activeStimChannels_N, 4))
                    stream(i).activeStimChannel(end+1) = -3;
                end
                if (bitand(activeStimChannels_P, 8))
                    stream(i).activeStimChannel(end+1) = +4;
                end
                if (bitand(activeStimChannels_N, 8))
                    stream(i).activeStimChannel(end+1) = -4;
                end
            end
        catch
            i = i -1;
            stream.time = stream.time(1:i);
            stream.stimCurrent = stream.stimCurrent(1:i);
            stream.stimVoltage = stream.stimVoltage(1:i);
            stream.nSample = stream.nSample(1:i);
            stream.packageNumber = stream.packageNumber(1:i);
        end
        stream.nSamples = i;
    end
end

%% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  callbacks
%% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

