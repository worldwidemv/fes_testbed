function [ ov ] = tb_buildOverview( ds, plotOverview, useEntireDataSet )
%TB_BUILDOVERVIEW build an overview over the data set

if (~exist('plotOverview', 'var')), plotOverview = false; end
if (~exist('useEntireDataSet', 'var')), useEntireDataSet = false; end

ov = struct();
ov.validChannels = [];
ov.time = [];
allChannels = [];
run = true;
setupOriginal = ds.setup;
nOverview = 10000;

if (useEntireDataSet)
    if (ds.setup.iStart ~= 1)
        ds = getNewDataSet(1, nOverview, setupOriginal);
    end
end

while run
    disp('BUILD OVERVIEW: processing data set ....');
    allChannels = getChannels(ds, allChannels);
    ov.validChannels = allChannels;
    
    startTime = round(ds.pulse(1).time(1) *10) /10;
    endTime   = round(ds.pulse(end).time(end) *10) /10;
    time = startTime:0.1:endTime;
    for ch = allChannels
        current{ch} = zeros(size(time));
        voltage{ch} = zeros(size(time));
        duration{ch} = zeros(size(time));
        period{ch} = zeros(size(time));
    end
    
    %% go through the pulses
    t = mean([ds.pulse(1).time(1), ds.pulse(1).time(end)]);
    iOV_old = getIndex(time, t);
    tS = getTimeslotStruct(allChannels);
    for iP = 1:length(ds.pulse)
        t = mean([ds.pulse(iP).time(1), ds.pulse(iP).time(end)]);
        iOV = getIndex(time, t);
        if (isempty(ds.pulse(iP).info.channel))
            channel = 1;
        else
            channel = ds.pulse(iP).info.channel(1);
        end
        if (iOV > iOV_old)
            % the old timeslot is done -> save the mean values
            for ch = allChannels
                if (~isempty(tS.Imax{ch})),   current{ch}(iOV_old)  = mean(tS.Imax{ch}); end
                if (~isempty(tS.Vmax{ch})),   voltage{ch}(iOV_old)  = mean(tS.Vmax{ch}); end
                if (~isempty(tS.PW{ch})),     duration{ch}(iOV_old) = mean(tS.PW{ch}); end
                if (~isempty(tS.Period{ch})), period{ch}(iOV_old)   = mean(diff(tS.Period{ch})) *1000;end
            end
            iOV_old = iOV;
            tS = getTimeslotStruct(allChannels);
        end
        
        % this timeslot is still active -> save  the data of this timeslot
        tS.Imax{channel}(end+1)  = max(abs(ds.pulse(iP).stimCurrent));
        tS.Vmax{channel}(end+1)  = max(abs(ds.pulse(iP).stimVoltage));
        tS.PW{channel}(end+1)    = ds.pulse(iP).info.parts(2) - ds.pulse(iP).info.parts(1);
        tS.Period{channel}(end+1)= ds.pulse(iP).time( ds.pulse(iP).info.boundariesThight(1) );
    end
    
    % save the results from the last pulse
    for ch = allChannels
        if (~isempty(tS.Imax{ch})),   current{ch}(iOV_old)  = mean(tS.Imax{ch}); end
        if (~isempty(tS.Vmax{ch})),   voltage{ch}(iOV_old)  = mean(tS.Vmax{ch}); end
        if (~isempty(tS.PW{ch})),     duration{ch}(iOV_old) = mean(tS.PW{ch}); end
        if (~isempty(tS.Period{ch})), period{ch}(iOV_old)   = mean(diff(tS.Period{ch})) *1000;end
        if (isnan(period{ch}))
            temp = zeros(size(ds.pulse));
            for iP = 1:length(ds.pulse)
                temp(iP) =  ds.pulse(iP).time( ds.pulse(iP).info.boundariesThight(1) );
            end
            newPeriod = mean(diff(temp)) *1000;
            period{ch} = ones(size(period{ch})) * newPeriod;
        end
    end
    
    %% done -> save the data in the structure
    iFull = find(current{allChannels(1)} ~= 0);
    time = time(iFull);
    iNewStart = length(ov.time) +1;
    ov.time = [ov.time, time];
    iNewEnd = length(ov.time);
    
    for ch = allChannels
        ov.current{ch}(iNewStart:iNewEnd)  = current{ch}(iFull);
        ov.voltage{ch}(iNewStart:iNewEnd)  = voltage{ch}(iFull);
        ov.duration{ch}(iNewStart:iNewEnd) = duration{ch}(iFull);
        ov.period{ch}(iNewStart:iNewEnd)   = period{ch}(iFull);
    end
    
    if (useEntireDataSet)
        if (ds.setup.iEnd < ds.setup.nAll)
            ds = getNewDataSet(ds.setup.iEnd +1, nOverview, setupOriginal);
            setupOriginal.fPositionDataFile = ds.setup.fPositionDataFile;
        else
            run = false;
        end
    else
        run = false;
    end
end
ov.setup = ds.setup;


if (plotOverview)
    doPlotOverview(ov);
end


    function ch = getChannels(ds, ch)
        if (~exist('ch', 'var')), ch = 1; end
        if (isempty(ch)), ch = 1; end
        for i = 1:length(ds.pulse)
            ch = union(ch, ds.pulse(i).info.channel);
        end
    end

    function iOV = getIndex(time, t)
        [~, iOV] = min(abs(time -t));
    end

    function temp = getTimeslotStruct(ch)
        temp = struct();
        chCell = {}; chCell{max(ch)} = [];
        
        temp.Imax = chCell;
        temp.Vmax = chCell;
        temp.PW = chCell;
        temp.Period = chCell;
    end

    function doPlotOverview(ov)
        f = figure();
        f.Position(3:4) = [1300, 850];
        a1 = subplot(4,1,1); a1.Position = [0.05, 0.76, 0.94, 0.2];
        %xlabel('time [sec]');
        legStr = {};
        for iCH = ov.validChannels
            legStr{end+1} = ['Channel ', num2str(iCH)];
            plot(ov.time, ov.current{iCH});
        end
        title(['Overview over "', ov.setup.fname,'"'], 'interpreter', 'none');
        ylabel('max. Current [mA]');
        legend(legStr);
        grid on;
        
        a2 = subplot(4,1,2); a2.Position = [0.05, 0.52, 0.94, 0.2];
        for iCH = ov.validChannels
            plot(ov.time, ov.voltage{iCH});
        end
        ylabel('max. Voltage [V]');
        %xlabel('time [sec]');
        legend(legStr);
        grid on;
        
        
        a3 = subplot(4,1,3); a3.Position = [0.05, 0.28, 0.94, 0.2];
        for iCH = ov.validChannels
            plot(ov.time, ov.duration{iCH});
        end
        ylabel('duration / PW [usec]');
        %xlabel('time [sec]');
        legend(legStr);
        grid on;
        
        
        a4 = subplot(4,1,4); a4.Position = [0.05, 0.045, 0.94, 0.2];
        for iCH = ov.validChannels
            plot(ov.time, ov.period{iCH});
        end
        ylabel('period [msec]');
        xlabel('time [sec]');
        legend(legStr);
        grid on;
    end

    function ds = getNewDataSet(iStart, N, param)
        disp([char(10), 'BUILD OVERVIEW -> Pulse ', num2str(iStart), ' - ' num2str(iStart +N -1),' from ', num2str(param.nAll), ' Pulses:']);
        setup = struct('iStart', iStart, 'N', N, 'nAll', param.nAll, 'pulseType', param.pulseType, 'skipDesc', true);
        if (isfield(param, 'fPositionDataFile')), setup.fPositionDataFile = param.fPositionDataFile; end
        ds = tb_readTestbedDataFile([param.fpath, filesep, param.fname, param.fext], setup );
        ds = tb_ppTestbedData(ds);
    end
end

