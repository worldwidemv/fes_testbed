function [ out ] = testbedViewer( in )
%TESTBEDVIEWER shows the GUI for the TestBed data
%   Matlab GUI for the Testbed Viewer

addpath('./incMatlab');

if (exist('in', 'var') && isstruct(in))
    data = in;
else
    [data] = tb_readTestbedDataFile();
    if (data.nPulses > 0)
        % 0=use RawPulses; 1=search bi-phasic with pause; 2=...
        data.setup.pulseType = 1;
        [data] = tb_ppTestbedData(data);
        %[data] = tb_getModelResponce(data);
        %try disp(data.pulse); catch, end
    end
end

out = tb_guiFig(data);
end

