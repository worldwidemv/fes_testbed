
function this =  xemg_StimYo2Interface()
global gGUI gData;
this = createStimYoII_Object();

    function o = createStimYoII_Object()
        o = struct();
        %% public/external API
        o.connectToEMG = @connectToEMG;
        o.disConnectFromEMG = @disConnectFromEMG;
        o.updateFilterSettings = @updateFilterSettings;
        o.overrideFdata = @overrideFdata;
        o.setFilterData = @setFilterData;
        
        o.delete            = @deleteObj;
        
        % update the EMG data
        gData.EMGData = struct();
        gData.EMGData.setup = struct();
        gData.EMGData.time = [];
        gData.EMGData.data = {};
        gData.EMGData.fdata = {};
        gData.EMGData.stimMarker = [];
        gData.EMGData.numberOfSamples = -1;
        
        o.setup = struct();
        o.setup.ip_address = '127.0.0.1';
        o.setup.port = 33133;
        o.setup.filterData = false;
        
        o.state = struct();
        o.state.connected = false;
        o.state.warningDisplayed = false;
        
        o.tcpCon = [];
        
        % [b,a] = ellip(n,Rp,Rs,Wp,ftype) designs a lowpass, highpass, bandpass, or bandstop elliptic filter
        % ellip(filter_param(1),filter_param(2),filter_param(3),f_highpass/(f_emg_sampling/2),'high');
        % -> [6,3,50]
        % n - filter_order = 6 / RP - Peak-to-peak passband ripple = 3 / Rs - Stopband attenuation = 50
        nStartOffset = 7;
        o.onlineFilter = emgFilterSetup(2000, 4, 1:(200+nStartOffset), [3,3,65]);
        o.onlineFilter.nStartOffset = nStartOffset;
        
        o.dataTimer = timer();
        o.dataTimer.Period = 0.09;
        o.dataTimer.ExecutionMode = 'fixedRate';
        o.dataTimer.TimerFcn = @dataTimerCallback;
        o.dataTimer.StartDelay = 1.5;
        start(o.dataTimer);
    end

    function ret = connectToEMG()
        ret = false;
        try
            this.tcpCon = tcpclient(this.setup.ip_address, this.setup.port);
            this.state.connected = true;
            gGUI.EMG.state.connected = this.state.connected;
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

    function disConnectFromEMG()
        if (this.state.connected)
            this.tcpCon.delete();
        end
        this.state.connected = false;
        gGUI.EMG.state.connected = this.state.connected;
    end

    function deleteObj()
        stop(gGUI.EMG.dataTimer);
        disConnectFromEMG();
    end

    function updateFilterSettings(f_sampling, f_hp, nStartOffset, filter_param)
        this.onlineFilter = emgFilterSetup(f_sampling, f_hp, 1:(200+nStartOffset), filter_param);
        this.onlineFilter.nStartOffset = nStartOffset;
    end

    function overrideFdata(f_sampling)
        gData.EMGData.setup.f_data = f_sampling;
        tP = 1/gData.EMGData.setup.f_data;
        gData.EMGData.setup.tP = tP;
        gData.EMGData.setup.timeTemp = 0 : tP : 1;
        gData.EMGData.time = [-tP*this.onlineFilter.nStartOffset: tP: (tP*gData.EMGData.numberOfSamples -this.onlineFilter.nStartOffset)];
    end

    function setFilterData(filterData)
        this.setup.filterData = filterData;
    end
%%
%% Custom callback function used by this TAB

    function dataTimerCallback(obj, event)
        
        try
            if (~this.state.connected)
                % not connected -> connect to the network interface
                if (connectToEMG())
                    readStimYo2Header();
                    gGUI.tabs.Plot.display();
                    gGUI.tabs.Plot.startTimer();
                end
            else
                % connected -> read the data from the network interface
                readStimYo2Data();
            end
            
        catch ME
            disp(['ID: ' ME.identifier])
            rethrow(ME)
        end
        
    end


%%
%% Helper functions for this TAB

    function readStimYo2Header()
        
        %% defaults
        param = struct( ...
            'chEnabled', [false, false, false, false, false, false, false, false], ...
            'gain', [0, 0, 0, 0, 0, 0, 0, 0], ...
            'f_data', 0, ...
            'addRawData', false, ...
            'addStimMarker', false, ...
            'outputInVolt', false, ...
            'debug', false ...
            );
        
        %% check that this the a StimYoII data file
        %disp(this.tcpCon.BytesAvailable)
        dataType = read(this.tcpCon, 15, 'uint8');
        dataTypeStr = char(dataType);
        if (~strcmp(dataTypeStr, '%StimyoIII Data'))
            error(['The live data is not a StimyoIII data stream! (ID: "', dataTypeStr, '")']);
        end
        % read 5 dummy bytes
        read(this.tcpCon, 5, 'uint8');
        % read option bytes
        sampleRate = read(this.tcpCon, 1, 'uint8');
        switch sampleRate
            case 0
                f_data = 32000;
            case 1
                f_data = 16000;
            case 2
                f_data = 8000;
            case 3
                f_data = 4000;
            case 4
                f_data = 2000;
            case 5
                f_data = 1000;
            case 6
                f_data = 500;
            otherwise
                error(['Unknown sample rate: ', num2str(sampleRate), '!']);
        end
        if (param.f_data == 0)
            param.f_data = f_data;
        end
        param.addRawData   = boolean(read(this.tcpCon, 1, 'uint8'));
        param.addStimMarker= boolean(read(this.tcpCon, 1, 'uint8'));
        param.outputInVolt = boolean(read(this.tcpCon, 1, 'uint8'));
        % read 7 dummy bytes
        read(this.tcpCon, 6, 'uint8');
        % read 8 channel enabled bytes
        for iCh = 1:8
            param.chEnabled(iCh) = boolean(read(this.tcpCon, 1, 'uint8'));
        end
        % read 8 channel gain bytes
        for iCh = 1:8
            gain = read(this.tcpCon, 1, 'uint8');
            switch gain
                case 16
                    param.gain(iCh) = 1;
                case 32
                    param.gain(iCh) = 2;
                case 48
                    param.gain(iCh) = 3;
                case 64
                    param.gain(iCh) = 4;
                case 0
                    param.gain(iCh) = 6;
                case 80
                    param.gain(iCh) = 8;
                case 96
                    param.gain(iCh) = 12;
                otherwise
                    error(['Unknown gain ', num2str(gain), ' for channel  ', num2str(iCh), '!']);
            end
        end
        % read 18 dummy bytes
        read(this.tcpCon, 18, 'uint8');
        
        %% gData
        gData.EMGData = struct(); %'setup', [], 'data', {}, 'numberOfSamples', 0, 'timeoutCounter', 200);
        gData.EMGData.setup = param;
        gData.EMGData.numberOfSamples = this.onlineFilter.nStartOffset;
        gData.EMGData.offset = [0,0,0,0,0,0,0,0];
        
        jCh = [];
        for j = 1:8
            if (gData.EMGData.setup.chEnabled(j))
                jCh(end +1) = j;
                gData.EMGData.data{j} = zeros(1,this.onlineFilter.nStartOffset);
                gData.EMGData.fdata{j} = zeros(1,this.onlineFilter.nStartOffset);
            end
        end
        gData.EMGData.stimMarker = zeros(1,this.onlineFilter.nStartOffset);
        
        gData.EMGData.setup.nCh = length(jCh);
        gData.EMGData.setup.jCh = jCh;
        gData.EMGData.setup.timeoutCounter = 20;
        tP = 1/gData.EMGData.setup.f_data;
        gData.EMGData.setup.tP = tP;
        gData.EMGData.setup.timeTemp = 0 : tP : 1;
        gData.EMGData.time = [-tP*this.onlineFilter.nStartOffset: tP: -tP];
        % update filter settings
        gGUI.tabs.Setup.updateSamplingRate(param.f_data);
    end

    function readStimYo2Data()
        jCh = gData.EMGData.setup.jCh;
        
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
                    % get number of samples
                    nSamples = uint32(read(this.tcpCon, 1, 'uint16'));
                    % make shure we have enougth data
                    %while (bytesAvailable < (5 + nSamples*4*gData.EMGData.setup.nCh))
                    %    pause(0.005);
                    %    gData.EMGData.setup.timeoutCounter = gData.EMGData.setup.timeoutCounter -1;
                    %    if (gData.EMGData.setup.timeoutCounter <= 0), break; end
                    %    bytesAvailable = this.tcpCon.BytesAvailable;
                    %end
                    %gData.EMGData.setup.timeoutCounter = 20;
                    
                    if (gData.EMGData.setup.addStimMarker)
                        gData.EMGData.stimMarker(gData.EMGData.numberOfSamples+1 : gData.EMGData.numberOfSamples+nSamples) = read(this.tcpCon, nSamples, 'uint8');
                    end
                    for iP = 1:length(jCh)
                        gData.EMGData.data{jCh(iP)}(gData.EMGData.numberOfSamples+1 : gData.EMGData.numberOfSamples+nSamples) = double(read(this.tcpCon, nSamples, 'single'));
                        if (this.setup.filterData)
                            gData.EMGData.fdata{jCh(iP)}(gData.EMGData.numberOfSamples+1 : gData.EMGData.numberOfSamples+nSamples) = emgFilter(...
                                gData.EMGData.data{jCh(iP)}(gData.EMGData.numberOfSamples+1 -this.onlineFilter.nStartOffset : gData.EMGData.numberOfSamples+nSamples), this.onlineFilter);
                            %gData.EMGData.fdata{jCh(iP)}(gData.EMGData.numberOfSamples+nSamples) = 0;
                        else
                            gData.EMGData.fdata{jCh(iP)}(gData.EMGData.numberOfSamples+1 : gData.EMGData.numberOfSamples+nSamples) = 0;
                        end
                    end
                    gData.EMGData.time(gData.EMGData.numberOfSamples+1 : gData.EMGData.numberOfSamples+nSamples) = ...
                        gData.EMGData.setup.timeTemp(1 : nSamples) + gData.EMGData.setup.tP + gData.EMGData.time(end);
                    gData.EMGData.numberOfSamples = gData.EMGData.numberOfSamples +nSamples;
                catch ME
                    %disp(['ID: ' ME.identifier])
                    disp(['#### Incomplete ReadIn: nBytesAvailabe:', num2str(bytesAvailable), '; nBytesSoll:', num2str(5 + nSamples*4*gData.EMGData.setup.nCh), ...
                        ' -->  nSamples:', num2str(nSamples)])
                    rethrow(ME);
                    % make shure the data is the same length
                    if (gData.EMGData.numberOfSamples > 0)
                        for iP = 1:length(jCh)
                            gData.EMGData.data{jCh(iP)} = gData.EMGData.data{jCh(iP)}(1 : gData.EMGData.numberOfSamples);
                        end
                    end
                    warning('Incomplete ReadIn...');
                end
            else
                warning(['Start Bytes were not found after ', num2str(gData.EMGData.numberOfSamples), ' samples! ("', char(StartBytes), '")']);
            end
        end
    end


end