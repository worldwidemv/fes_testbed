function [ d ] = tb_ppTestbedData( d )
%TB_PPTESTBEDDATA post processing of the TestBed data

if (strcmpi(d.setup.fext, '.mat'))
    return;
end
pulseStruct = struct('pulse', struct());
invalidRawIDs = [];
validRawIDs = [];

disp('TESTBED Data post processing');
% search for a known pulse form
switch (d.setup.pulseType)
    case 0 % no search
        disp('   -> useing the RAW pulses, without any cleanup');
    case 1 % bi-phasic with 100us pause
        disp('   -> searching for bi-phasic stimulation pulses with 100us pause in between');
        disp(['   -> adding ', num2str(d.setup.pulseAddonStart), 'us of data to the start of the pulse and ', num2str(d.setup.pulseAddonEnd), 'us to the end']);
    case 2 % bi-phasic without pause
    otherwise
        error(['Unkown pulse type ', num2str(d.setup.pulseType), '!']);
end
tic();

%% update the pulses / search for the pulse and cleanup the data
startExtended = [];
endExtended = [];
dataInvalidDiscarded = [];
dataInvalidOverriden = [];
for i = 1:length(d.pulseRaw)
    % search the pulse/pulses in the data
    [ pulseStruct, d.setup, isInvalid, startExt, endExt, dataInvDiscarded, dataInvOverriden ] = tbi_updatePulseStruct( pulseStruct, d.setup, d.pulseRaw(i), i);
    dataInvalidDiscarded   = [dataInvalidDiscarded, dataInvDiscarded];
    dataInvalidOverriden   = [dataInvalidOverriden, dataInvOverriden];
    startExtended = [startExtended, startExt];
    endExtended   = [endExtended, endExt];
    if (isInvalid)
        invalidRawIDs(end+1) = i;
        %disp(['      -> RawPulse ', num2str(i), ': Data invalid from ', num2str(d.pulseRaw(i).time(1)), ' to ', num2str(d.pulseRaw(i).time(end)), ' seconds!']);
    else
        validRawIDs(end+1) = i;
    end
    
end
timeNeeded = toc();

%% transfer the found pulses
if (isempty(fieldnames(pulseStruct.pulse)))
    d.nPulses = 0;
else
    d.nPulses = length(pulseStruct.pulse);
    d.pulse = pulseStruct.pulse;
    temp = strrep(num2str(dataInvalidDiscarded,'%u,'), ' ', '');
    disp(['         -> DATA DISCARDED in Pulses: [', temp(1:end-1), ']']);
    temp = strrep(num2str(dataInvalidOverriden,'%u,'), ' ', '');
    disp(['         -> DATA OVERRIDEN in Pulses: [', temp(1:end-1), ']']);
    temp = strrep(num2str(startExtended,'%u,'), ' ', '');
    if (length(temp) > 10)
        disp(['         -> START extended for Pulses: [', temp(1:10), ' ... ', num2str(length(temp) -10), ' more ]']);
    else
        disp(['         -> START extended for Pulses: [', temp(1:end-1), ']']);
    end
    temp = strrep(num2str(endExtended,'%u,'), ' ', '');
    if (length(temp) > 10)
        disp(['         -> END   extended for Pulses: [', temp(1:10), ' ... ', num2str(length(temp) -10), ' more ]']);
    else
        disp(['         -> END   extended for Pulses: [', temp(1:end-1), ']']);
    end
    disp([char(10),'      -> ', num2str(d.nPulses), ' Pulse detected, took ', num2str(timeNeeded),' sec']);
end

%% transfer the invalid pulses
if (~isempty(invalidRawIDs))
    d.nPulsesInvalid = length(invalidRawIDs);
    d.invalidRawPulses = invalidRawIDs;
    d.validRawPulses = validRawIDs;
    disp(['      -> Pulse detection failed for ', num2str(d.nPulsesInvalid), ' RawPulse!']);
end

%% get statistics for the found pulses
%[ d ] = tb_getStats( d );

end

