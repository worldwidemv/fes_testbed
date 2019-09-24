function [ pulsesFound ] = tbi_searchPulse_biPhasicWPause( current, channel, param)
%TBI_SEARCHPULSE_BIPHASICWPAUSE searches for single bi-phasic stimulation pulses, with a pause
%   The input (current signal) is evaluated in case there are multiple pulses wihtin the data.
%   A struct with the basic pulse structure is returned.

pulsesFound = struct();

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

dt = 1/param.f_sample;
n = floor(param.f_sample / param.f_downsample);
minWidthPulsePartSamples = floor( ((param.minWidthPulsePart_us/param.f_sample) / dt) / n );
maxInnerPulsePauseSamples = floor(  ((param.maxInnerPulseZeroTime_us/param.f_sample) / dt) / n );

% downsample the current vector and get the differences between each sample
curr = downsample(current, n); lCurr = length(curr);
curr_diff = [0; diff(curr)];

% get the index of the steps
stepsP = find(curr_diff > param.threshold);
stepsN = find(curr_diff < param.threshold*-1);
stepsAll = sort([stepsP', stepsN']);

% go through the steps and find the actual pulse parts
state = 0; % 0 = Zero; -1 = Negative; +1 = Positive
iStart = 0;
pulses = [];
pulseParts = [];
for i = 1:length(stepsAll)
    switch (state)
        case +1
            % positive pulse part is active -> search for the end
            if (curr(min([(stepsAll(i)+0) lCurr])) < param.threshold)
                state = 0;
                pulseParts(end+1, 1:2) = [iStart, stepsAll(i)];
                [pulses, pulsesFound] = addPulsePart(pulses, pulsesFound, [iStart, stepsAll(i)], +1, minWidthPulsePartSamples, maxInnerPulsePauseSamples);
                % check if we go strate to a negative pulse part
                if (curr(stepsAll(i)) < param.threshold*-1)
                    state = -1;
                    iStart = stepsAll(i);
                end
            end
        case -1
            % negative pulse part is active -> search for the end
            if (curr(stepsAll(i)) > param.threshold*-1)
                state = 0;
                pulseParts(end+1, 1:2) = [iStart, stepsAll(i)];
                [pulses, pulsesFound] = addPulsePart(pulses, pulsesFound, [iStart, stepsAll(i)], -1, minWidthPulsePartSamples, maxInnerPulsePauseSamples);
                % check if we go strate to a positive pulse part
                if (curr(stepsAll(i)) > param.threshold)
                    state = +1;
                    iStart = stepsAll(i);
                end
            end
        otherwise
            % no pulse part is active -> search for the start of the next one
            if (curr(stepsAll(i)) > param.threshold)
                state = +1;
                iStart = stepsAll(i);
                continue;
            end
            if (curr(stepsAll(i)) < param.threshold*-1)
                state = -1;
                iStart = stepsAll(i);
                continue;
            end
    end
end
if (isempty(fieldnames(pulsesFound)))
    pulsesFound = struct();
    return;
end

% find the real boundaries in the hi-res signal
pulsesFull = zeros(size(pulses));
current_length = length(current);
for i = 1:length(pulsesFound)
    % go through all pulses
    for j = 1:size(pulsesFound(i).info.parts,1)
        % go through all pulse parts
        for k = 1:2
            % get the index for the current jumps in the hi-res signal
            % -> search around the low-res jump
            i_start = ((pulsesFound(i).info.parts(j,k)-2)*n) +1;
            i_end   = i_start + 2*n -2;
            %pulsesFull(i,j) = find(abs(diff(current( max([0, i_start]):min([i_end, current_length]) ))) > param.threshold, 1) + i_start;
            [~, temp] = max(abs(diff(current( max([0, i_start]):min([i_end, current_length]) )) ));
            pulsesFound(i).info.parts(j,k) = temp + i_start;
        end
    end
    pulsesFull(i,1) = pulsesFound(i).info.parts(1,1);
    pulsesFull(i,2) = pulsesFound(i).info.parts(end,2);
end
pulsesThight = pulsesFull;

% add a little bit surrounding data for context
for i = 1:size(pulsesFull,1)
    % add N samples
    pulsesFull(i,1) = pulsesFull(i,1) -param.pulseAddonStart;
    pulsesFull(i,2) = pulsesFull(i,2) +param.pulseAddonEnd;
end

% debug plots
if (param.debug)
    figure(); hold on;
    t1 = 0 :(dt): ((dt)*length(current) -dt);
    t2 = downsample(t1, n);
    plot(t1, current, 'r-');
    plot(t2, curr, 'm-');
    plot(t2, curr_diff, 'b-');
    pulsePartsPlot = reshape(pulseParts', [1,numel(pulseParts)]);
    plot(t2(pulsePartsPlot), zeros(size(pulsePartsPlot)), 'g*');
    pulsesThightPlot = reshape(pulsesThight', [1,numel(pulsesThight)]);
    plot(t1(pulsesThightPlot), zeros(size(pulsesThightPlot)), 'r*');
    for i = 1:size(pulsesFull,1)
        try
            plot(t1(pulsesFull(i,:)), ones(size(pulsesFull(i,:)))*-1, 'cd-');
        catch
            startTime = [];
            iStart = 1; %pulsesFull(i,1);
            if (pulsesFull(i,1) < 0)
                dt = 1/param.f_sample;
                startTime = (t1(1) -(abs(pulsesFull(i,1))+1)*dt):dt:(t1(1) -dt);
                iStart = 1;
            end
            endTime = [];
            iEnd = pulsesFull(i,2);
            sC_length = length(current);
            if (pulsesFull(i,2) > sC_length)
                dt = 1/param.f_sample;
                endTime = (t1(end) +dt):dt:(t1(end) + (pulsesFull(i,2) -sC_length +1)*dt);
                iEnd = sC_length;
            end
            t1temp = [startTime, t1(iStart:iEnd), endTime];
            plot(t1temp(iStart,iEnd), ones(size(pulsesFull(i,:)))*-1, 'cd-');
        end
    end
    
    legend('current (f_sample)', 'current (f_downsample)', 'current edges (diff)', 'all pulse part edges', 'pulse edges', 'entire pulse');
    grid on;
end

% build return struct
for i = 1:length(pulsesFound)
    pulsesFound(i).info.boundariesWide = pulsesFull(i,:);
    pulsesFound(i).info.boundariesThight = pulsesThight(i,:);
    % get the absolute/global index to the pulse specific index
    pulsesFound(i).info.parts(:,1:2) = pulsesFound(i).info.parts(:,1:2) - pulsesFound(i).info.parts(1,1) +param.pulseAddonStart;
    pulsesFound(i).info.offsets = [ param.pulseAddonStart param.pulseAddonEnd];
    % add the stimulation channels; channels is a vector with at least
    % 2*numberOfPulses entries -> always two entries belong to one pulse ...
    if (~isempty(channel))
        iChannelEnd = min([2*i; length(channel)]);
        temp = unique(abs( channel( max([1; (iChannelEnd-1)]): iChannelEnd) ));
        if (length(temp) == 1)
            % length is 1 -> input and output channel are identical
            pulsesFound(i).info.channel = temp;
        else
            % length is NOT 1 -> input and output channel are different
            pulsesFound(i).info.channel = channel( max([1; (iChannelEnd-1)]): iChannelEnd);
        end
    end
end

    function [pulses, pulsesFound] = addPulsePart(pulses, pulsesFound, pulse, type, minWidthPulsePart, maxInnerPulsePause)
        if ((pulse(2) -pulse(1)) >= minWidthPulsePart)
            % the pulse part is widther as the minimum -> add it to the last pulse or start a new pulse
            if (isempty(pulses))
                % the pulses matrix is empty -> this must be the start of a new pulse
                pulses(end+1,1:2) = pulse;
                pulsesFound(1).info = struct();
                pulsesFound(end).nSamples = 0;
                pulsesFound(end).time = [];
                pulsesFound(end).stimCurrent = [];
                pulsesFound(end).stimVoltage = [];
                pulsesFound(end).nSample = [];
                pulsesFound(end).info.type = 1; % -> bi-phasic with pause
                pulsesFound(end).info.sourcePulseRaw = 0;
                pulsesFound(end).info.boundariesWide = [];
                pulsesFound(end).info.boundariesThight = [];
                pulsesFound(end).info.channel = [];
                pulsesFound(end).info.offsets = [0, 0];
                pulsesFound(end).info.parts = [pulse, type];
                pulsesFound(end).info.dataWasDiscarded = false;
                pulsesFound(end).info.dataWasOverriden = false;
                pulsesFound(end).info.demuxUsed = false;
                pulsesFound(end).info.misc = struct();
                pulsesFound(end).stats = struct();
                return;
            end
            if ((pulse(1) -pulses(end,2)) <= maxInnerPulsePause)
                % the distance between the last point of the last pulse
                % part and the star of this pulse part is smaller thant the maxInnerPulsePause
                % -> add this pulse part to the last pulse
                pulses(end,2) = pulse(2);
                pulsesFound(end).info.parts(end+1,1:3) = [pulse, type];
            else
                % the distance between the last point of the last pulse
                % part and the star of this pulse part is greater thant the maxInnerPulsePause
                % -> this is a new pulse
                pulses(end+1,1:2) = pulse;
                pulsesFound(end+1).info.parts = [pulse, type];
            end
        end
    end
end

