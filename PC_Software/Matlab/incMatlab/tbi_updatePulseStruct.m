function [ pulseStruct, param, invalidData, startExtended, endExtended, dataInvalidDiscarded, dataInvalidOverriden] = tbi_updatePulseStruct( pulseStruct, param, rawPulse, iRawPulse )
%TBI_UPDATEPULSESTRUCT Summary of this function goes here
%   Detailed explanation goes here

dataInvalidDiscarded = [];
dataInvalidOverriden = [];
startExtended = [];
endExtended = [];

% search for a known pulse form
switch (param.pulseType)
    case 0 % no search
        pulses = tbi_searchPulse_UseFullData( rawPulse.stimCurrent, rawPulse.activeChannel.aCH_Stim, param );
    case 1 % bi-phasic with 100us pause
        pulses = tbi_searchPulse_biPhasicWPause( rawPulse.stimCurrent, rawPulse.activeChannel.aCH_Stim, param);
    case 2 % bi-phasic without pause
    otherwise
        error(['Unkown pulse type ', num2str(param.pulseType), '!']);
end

% check if we found anything
invalidData = false;
if (isempty(pulses) || isempty(fieldnames(pulses)))
    % invalid pulse / data found -> mark it, so one can have a look later
    invalidData = true;
    return;
end

% update this pulse and add it to the list
lPulses = length(pulseStruct.pulse);
for j = 1:length(pulses)
    updateTimeNeeded = false;
    startData = [];
    startTime = [];
    iStart = pulses(j).info.boundariesWide(1);
    if (pulses(j).info.boundariesWide(1) <= 0)
        startData = zeros(abs(pulses(j).info.boundariesWide(1)), 1);
        dt = 1/param.f_sample;
        startTime = (rawPulse.time(1) -(abs(pulses(j).info.boundariesWide(1)))*dt):dt:(rawPulse.time(1) -dt);
        iStart = 1;
        updateTimeNeeded = true;
        %disp(['      -> Pulse ', num2str(j + lPulses), ': Pulse START extended']);
        startExtended = [startExtended, (j +lPulses)];
    end
    endData = [];
    endTime = [];
    iEnd = pulses(j).info.boundariesWide(2);
    sC_length = length(rawPulse.stimCurrent);
    if (pulses(j).info.boundariesWide(2) > sC_length)
        endData = zeros((pulses(j).info.boundariesWide(2) -sC_length), 1);
        dt = 1/param.f_sample;
        endTime = (rawPulse.time(end) +dt):dt:(rawPulse.time(end) + (pulses(j).info.boundariesWide(2) -sC_length)*dt);
        iEnd = sC_length;
        updateTimeNeeded = true;
        %disp(['      -> Pulse ', num2str(j +lPulses), ': Pulse END extended']);
        endExtended = [endExtended, (j +lPulses)];
    end
    % update/build time vector
    if (param.relativeTimeVectorInUS)
        % time vetor shall be in us -> build the vector
        pulses(j).time = 0:( length(startTime) + length(endTime) + (iEnd-iStart) );
        pulses(j).time = pulses(j).time - length(startTime);
    else
        % time vector is absolute in seconds
        if (updateTimeNeeded)
            pulses(j).time = [startTime'; rawPulse.time(iStart:iEnd); endTime'];
        else
            % update is not needed -> take it from the data
%            disp([iStart, iEnd, size(rawPulse.time)])
            pulses(j).time = rawPulse.time(iStart:iEnd);
        end
    end
    % update the normal data
    pulses(j).stimCurrent   = [startData;  rawPulse.stimCurrent(iStart:iEnd); endData];
    startDataV = ones(size(startData))* mean(rawPulse.stimVoltage(iStart:iStart+20));
    pulses(j).stimVoltage   = [startDataV; rawPulse.stimVoltage(iStart:iEnd); endData];
    pulses(j).nSample       = [startData;  rawPulse.nSamples(iStart:iEnd); endData];
    pulses(j).nSamples      = length(pulses(j).nSample);
    pulses(j).info.dataWasDiscarded = rawPulse.dataWasDiscarded;
    if (pulses(j).info.dataWasDiscarded)
        %disp(['      -> Pulse ', num2str(j +lPulses), ': this pulse did override some data on the MCU -> SOME DATA WAS LOST']);
        dataInvalidDiscarded = [dataInvalidDiscarded, (j +lPulses)];
    end
    pulses(j).info.dataWasOverriden = rawPulse.dataWasOverriden;
    if (pulses(j).info.dataWasOverriden)
        %disp(['      -> Pulse ', num2str(j +lPulses), ': this pulse did override some data on the MCU -> SOME DATA WAS LOST']);
        dataInvalidOverriden = [dataInvalidOverriden, (j +lPulses)];
    end
    pulses(j).info.sourcePulseRaw = iRawPulse;
    % add pulse to list
    if (lPulses==1 && isempty(fieldnames(pulseStruct.pulse)))
        pulseStruct.pulse = pulses(j);
    else
        pulseStruct.pulse(end+1) = pulses(j);
    end
end

end

