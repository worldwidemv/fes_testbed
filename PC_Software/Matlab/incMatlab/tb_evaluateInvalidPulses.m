function [ stats ] = tb_evaluateInvalidPulses( pulseStruct, useRawData )
%TB_EVALUATEINVALIDDATA goes trought all invalid data sets and gets some
%statistics

lData = pulseStruct.nPulsesInvalid;
v = struct();
v.dMean = zeros(1,lData);
v.dMin  = zeros(1,lData);
v.dMax  = zeros(1,lData);
c = v;

invalidRawPulses = pulseStruct.invalidRawPulses;
for i = 1:lData
    if (useRawData)
        v.dMean(i) = mean(pulseStruct.pulseRaw(invalidRawPulses(i)).stimVoltageRaw);
        v.dMin(i)  = min( pulseStruct.pulseRaw(invalidRawPulses(i)).stimVoltageRaw);
        v.dMax(i)  = max( pulseStruct.pulseRaw(invalidRawPulses(i)).stimVoltageRaw);
        c.dMean(i) = mean(pulseStruct.pulseRaw(invalidRawPulses(i)).stimCurrentRaw);
        c.dMin(i)  = min( pulseStruct.pulseRaw(invalidRawPulses(i)).stimCurrentRaw);
        c.dMax(i)  = max( pulseStruct.pulseRaw(invalidRawPulses(i)).stimCurrentRaw);
    else
        v.dMean(i) = mean(pulseStruct.pulseRaw(invalidRawPulses(i)).stimVoltage);
        v.dMin(i)  = min( pulseStruct.pulseRaw(invalidRawPulses(i)).stimVoltage);
        v.dMax(i)  = max( pulseStruct.pulseRaw(invalidRawPulses(i)).stimVoltage);
        c.dMean(i) = mean(pulseStruct.pulseRaw(invalidRawPulses(i)).stimCurrent);
        c.dMin(i)  = min( pulseStruct.pulseRaw(invalidRawPulses(i)).stimCurrent);
        c.dMax(i)  = max( pulseStruct.pulseRaw(invalidRawPulses(i)).stimCurrent);
    end
end

figure();
plot([v.dMin', v.dMean', v.dMax']);
grid on;
legend('Min', 'Mean', 'Max');
title('Stats for the invalid VOLTAGE data');
xlabel('data set');
ylabel('Voltage [V]');

figure();
plot([c.dMin', c.dMean', c.dMax']);
grid on;
legend('Min', 'Mean', 'Max');
title('Stats for the invalid CURRENT data');
xlabel('data set');
ylabel('Current [mA]');

stats = struct('voltage', v, 'current', c);
