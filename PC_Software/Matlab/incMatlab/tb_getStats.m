function [ pulseStruct ] = tb_getStats( pulseStruct )
%TB_GETSTATS adds statistic information about the found pulses to the pulse struct

if (isfield(pulseStruct, 'pulse'))
    pulse = pulseStruct.pulse;
    sequenceIndex = 1;
    Tsequence = 0;
    stats = struct();
    
    for i = 1:length(pulse)
        s = struct();
        [s, sequenceIndex] = getTimingData(s, i, sequenceIndex, Tsequence);
        Tsequence = s.Tsequence;
      
        
        pulseStruct.pulse(i).stats = s;
        pulseStruct.pulse(i).info.iSequence = sequenceIndex;
        [stats] = updateStats(s, i, stats);
    end

    pulseStruct.stats = stats;
end

    function [s, sequenceIndex] = getTimingData(s, i, sequenceIndexOld, Tsequence)
        s.Tchannel  = 0;
        s.Tprevious = 0;
        s.Tsequence = 0;
        s.Fchannel  = inf;
        s.Fprevious = inf;
        s.Fsequence = inf;
        tstart2 = pulse(i).time(pulse(i).info.parts(1,1));
        if (~isempty(pulse(i).info.channel))
            channel = pulse(i).info.channel(1);
        else
            channel = 0;
        end
        % search the last pulse of this channel -> go back to the start if
        % neccessary; abort as soon as the first pulse for this channel is found
        for j = -1 : -1 : (i-1)*-1
            if (j == -1)
                tstart1 = pulse(i+j).time(pulse(i+j).info.parts(1,1));
                s.Tprevious = tstart2 -tstart1;
                s.Fprevious = 1/s.Tprevious;
                s.Tprevious = s.Tprevious *1000;
            end
            if (channel == 0), break; end
            if (~isempty(pulse(i+j).info.channel))
                if (channel == pulse(i+j).info.channel(1))
                    tstart1 = pulse(i+j).time(pulse(i+j).info.parts(1,1));
                    s.Tchannel = tstart2 -tstart1;
                    s.Fchannel = 1/s.Tchannel;
                    s.Tchannel = s.Tchannel *1000;
                    break;
                end
            end
        end
        
        % check if this pulse is the start of a new sequence
        if (s.Tprevious < 5) % 5 ms
            sequenceIndex = sequenceIndexOld;
            s.Tsequence = Tsequence;
        else
            sequenceIndex = sequenceIndexOld +1;
            % search the last pulse of this sequence
            iStartPulseOldSequence = 1;
            for j = -1 : -1 : (i-1)*-1
                if (pulseStruct.pulse(i+j).info.iSequence < sequenceIndexOld)
                    iStartPulseOldSequence = i+j +1;
                    break;
                end
            end
            tstart1 = pulse(iStartPulseOldSequence).time(pulse(iStartPulseOldSequence).info.parts(1,1));
            s.Tsequence = tstart2 -tstart1;
        end
        s.Fsequence = 1/s.Tsequence;
        s.Tsequence = s.Tsequence *1000;
    end

    function [stats] = updateStats(s, i, stats)
        for field = fieldnames(s)'
            field = char(field);
            stats.(field)(i) = s.(field); 
        end
    end
end

