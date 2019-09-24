function tb_plotPulse_V( pulse, index, fig )
%TB_PLOTPULSE plot one pulse
%   Detailed explanation goes here

if (isempty(pulse))
    warning('NO pulse found!');
    return;
end

if (~exist('index', 'var') || isempty(index))
    index = 1:length(pulse);
else 
    index = intersect([1:length(pulse)], index);
end

if (~exist('fig', 'var') || isempty(fig))
    fig = figure();
end

figure(fig);
clf('reset');
hold on;
%cmap = colormap(parula(length(index)));
cmap = colormap(lines(length(index)));

j = 0;
legendStr = {};
for i = index
    j = j +1;
    if (~isfield(pulse(i), 'info')), pulse(i).info.channel = []; end
    time = (pulse(i).time);% - pulse(i).time( pulse(i).info.boundariesThight(1))) *1000000;
    plot(time, pulse(i).stimCurrent, '-');%, 'Color', cmap(j,:));  
    legendStr{end+1} = ['pulse ', num2str(i), ' (current; ch.:', num2str(pulse(i).info.channel), ')'];
    plot(time, pulse(i).stimVoltage, '-.');%, 'Color', cmap(j,:));  
    legendStr{end+1} = ['pulse ', num2str(i), ' (voltage; ch.:', num2str(pulse(i).info.channel), ')'];
end

grid on;
hold off;
legend(legendStr, 'Location', 'best');
xlabel('time [us]');
ylabel('stimulation current [*; mA] and voltage [.; V]');

end

