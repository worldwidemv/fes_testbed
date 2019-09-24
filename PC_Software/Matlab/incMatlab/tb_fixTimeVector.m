function [ d ] = tb_fixTimeVector( d )
%TB_FIXTIMEVECTOR recalculates the time vector
for iP = 1:length(d.pulseRaw)
    d.pulseRaw(iP).time = d.pulseRaw(iP).nSamples./d.setup.f_sample;
end

end

