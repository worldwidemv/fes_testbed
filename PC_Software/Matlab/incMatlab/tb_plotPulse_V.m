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
cmap = lines(length(index));

j = 0;
legendStr = {};
for i = index
    j = j +1;
    
    time = pulse(i).time - pulse(i).time(1);
    plot(time, pulse(i).stimVoltage, 'Color', cmap(j,:));  
    legendStr{end+1} = ['pulse ', num2str(i), ' (ch.:', num2str(pulse(i).channel), ')'];
end

grid on;
hold off;
legend(legendStr, 'Location', 'best');
xlabel('time [us]');
ylabel('stimulation voltage [V]');

end

