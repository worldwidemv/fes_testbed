function [ pulsesFound ] = tbi_searchPulse_UseFullData( current, channel, param)
%TBI_SEARCHPULSE create the data structure to copy one pulse

%% defaults / defines
if (~exist('param', 'var'))
    param = struct( 'f_sample', 1000000, ...
        'f_downsample', 100000, ...
        'minWidthPulsePart_us', 20, ...
        'maxInnerPulseZeroTime_us', 150, ...
        'threshold', 2.5, ...
        'pulseAddonStart', 200, ...
        'pulseAddonEnd', 200, ...
        'debug', false);
end

iStart = 1;
iEnd   = length(current);

pulsesFound = struct();
pulsesFound.nSamples = iEnd;
pulsesFound.time = [];
pulsesFound.stimCurrent = [];
pulsesFound.stimVoltage = [];
pulsesFound.nSample = [];
pulsesFound.info.type = 0; % -> full data
pulsesFound.info.sourcePulseRaw = 0;
pulsesFound.info.boundariesWide = [iStart, iEnd];
pulsesFound.info.boundariesThight = [iStart, iEnd];
pulsesFound.info.channel = [];
pulsesFound.info.offsets = [1, 0];
pulsesFound.info.parts = [iStart, iEnd, 0];
pulsesFound.info.dataWasDiscarded = false;
pulsesFound.info.dataWasOverriden = false;
pulsesFound.info.demuxUsed = false;
pulsesFound.info.misc = struct();
pulsesFound.stats = struct();

% add the stimulation channels; channels is a vector with at least
% 2*numberOfPulses entries -> always two entries belong to one pulse ...
if (~isempty(channel))
    iChannelEnd = min([2; length(channel)]);
    temp = unique(abs( channel( max([1; (iChannelEnd-1)]): iChannelEnd) ));
    if (length(temp) == 1)
        % length is 1 -> input and output channel are identical
        pulsesFound.info.channel = temp;
    else
        % length is NOT 1 -> input and output channel are different
        pulsesFound.info.channel = channel( max([1; (iChannelEnd-1)]): iChannelEnd);
    end
end

end

