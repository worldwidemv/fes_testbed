function varargout = tb_guiFig(varargin)
% TB_GUIFIG MATLAB code for tb_guiFig.fig
%      TB_GUIFIG, by itself, creates a new TB_GUIFIG or raises the existing
%      singleton*.
%
%      H = TB_GUIFIG returns the handle to a new TB_GUIFIG or the handle to
%      the existing singleton*.
%
%      TB_GUIFIG('CALLBACK',hObject,eventData,handles,...) calls the local
%      function named CALLBACK in TB_GUIFIG.M with the given input arguments.
%
%      TB_GUIFIG('Property','Value',...) creates a new TB_GUIFIG or raises the
%      existing singleton*.  Starting from the left, property value pairs are
%      applied to the GUI before tb_guiFig_OpeningFcn gets called.  An
%      unrecognized property name or invalid value makes property application
%      stop.  All inputs are passed to tb_guiFig_OpeningFcn via varargin.
%
%      *See GUI Options on GUIDE's Tools menu.  Choose "GUI allows only one
%      instance to run (singleton)".
%
% See also: GUIDE, GUIDATA, GUIHANDLES

% Edit the above text to modify the response to help tb_guiFig

% Last Modified by GUIDE v2.5 04-Sep-2019 00:24:48

% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
    'gui_Singleton',  gui_Singleton, ...
    'gui_OpeningFcn', @tb_guiFig_OpeningFcn, ...
    'gui_OutputFcn',  @tb_guiFig_OutputFcn, ...
    'gui_LayoutFcn',  [] , ...
    'gui_Callback',   []);
if nargin && ischar(varargin{1})
    gui_State.gui_Callback = str2func(varargin{1});
end

if nargout
    [varargout{1:nargout}] = gui_mainfcn(gui_State, varargin{:});
else
    gui_mainfcn(gui_State, varargin{:});
end
% End initialization code - DO NOT EDIT


% --- Executes just before tb_guiFig is made visible.
function tb_guiFig_OpeningFcn(hObject, eventdata, handles, varargin)
% This function has no output args, see OutputFcn.
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% varargin   command line arguments to tb_guiFig (see VARARGIN)

% Choose default command line output for tb_guiFig
handles = guidata(hObject);
handles.output = hObject;

if (length(varargin) > 0) && (isstruct(varargin{1}) || iscell(varargin{1}))
    handles.data = varargin{1};
else
    %% convert the raw data directly
    handles.data = tb_readTestbedDataFile();
end

handles.data_fname = 'unknown';
handles.data_fpath  = '';
if (handles.data.nPulses == 0)
    %PathName = fileparts(mfilename('fullpath'));
    %handles.data = load([PathName, filesep, 'dummyData.mat']); 
end
if (isfield(handles.data, 'setup'))
    if (isfield(handles.data.setup, 'filename'))
        handles.data_fname = handles.data.setup.filename;
    end
    if (isfield(handles.data.setup, 'path'))
        handles.data_fpath = handles.data.setup.path;
    end
end
handles.output = handles.data;
% Update handles structure
guidata(hObject, handles);

% UIWAIT makes tb_guiFig wait for user response (see UIRESUME)
% uiwait(handles.figure1);


% --- Outputs from this function are returned to the command line.
function varargout = tb_guiFig_OutputFcn(hObject, eventdata, handles)
% varargout  cell array for returning output args (see VARARGOUT);
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Get default command line output from handles structure
varargout{1} = handles.output;

% settings
handles.plot.cmap = colormap(lines(150));
handles.settings.i_start = 1;
handles.settings.i_stop = 2;
handles.settings.window_position = 1;
handles.settings.nPulses = 1;
handles.settings.channels = 0;
handles.settings.maxCurrent = -1;
handles.settings.showPulseParts = false;
handles.settings.showModelSim = false;

handles.settings.downsamplingrate = 100;

handles.settings.showOverview = true;
handles.settings.viewLiveData = false;
handles.settings.viewLiveDataSound = false;
set(handles.showOverview,'Value', 1);
set(handles.showOverview,'BackgroundColor', [0.8900    0.8900    0.8900]);

% sound feedback
PathName = fileparts(mfilename('fullpath'));
handles.pingSound = load([PathName, filesep, 'pingSound.mat']);
handles.pingSound.counter = 0;
handles.pingSound.counterDefault = 23;
handles.pingSound.counterDefault2 = 23;
% update the data
handles = updateData(handles);
guidata(hObject, handles);
updateSliderSettings(hObject);
updatePlots(hObject, true);
% update the dataSet GUI elements
updateHandles(hObject);

% --- Executes on slider movement.
function wpos_Callback(hObject, eventdata, handles)
% hObject    handle to wpos (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'Value') returns position of slider
%        get(hObject,'Min') and get(hObject,'Max') to determine range of slider


% --- Executes during object creation, after setting all properties.
function wpos_CreateFcn(hObject, eventdata, handles)
% hObject    handle to wpos (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end
addlistener(hObject,'ContinuousValueChange',@(hObject, event) updateWindows(hObject, eventdata, handles));


function nPulsesEdit_Callback(hObject, eventdata, handles)
% hObject    handle to nPulsesEdit (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of nPulsesEdit as text
%        str2double(get(hObject,'String')) returns contents of nPulsesEdit as a double
handles = guidata(hObject);
handles.settings.nPulses = str2double(get(hObject,'String'));
% update the data
guidata(hObject, handles);
updatePlots(hObject, true);


% --- Executes during object creation, after setting all properties.
function nPulsesEdit_CreateFcn(hObject, eventdata, handles)
% hObject    handle to nPulsesEdit (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


function chEdit_Callback(hObject, eventdata, handles)
% hObject    handle to chEdit (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of chEdit as text
%        str2double(get(hObject,'String')) returns contents of chEdit as a double
handles = guidata(hObject);
handles.settings.channels = eval(get(hObject,'String'));

% update the data
handles = updateData(handles);
guidata(hObject, handles);
updateSliderSettings(hObject);
updatePlots(hObject, true);


% --- Executes during object creation, after setting all properties.
function chEdit_CreateFcn(hObject, eventdata, handles)
% hObject    handle to chEdit (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


function maxCurrent_Callback(hObject, eventdata, handles)
% hObject    handle to maxCurrent (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
handles = guidata(hObject);
handles.settings.maxCurrent = eval(get(hObject,'String'));

% update the data
handles = updateData(handles);
guidata(hObject, handles);
updateSliderSettings(hObject);
updatePlots(hObject, true);


% --- Executes during object creation, after setting all properties.
function maxCurrent_CreateFcn(hObject, eventdata, handles)
% hObject    handle to maxCurrent (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on button press in showPulseParts.
function showPulseParts_Callback(hObject, eventdata, handles)
% hObject    handle to showPulseParts (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
handles = guidata(hObject);
handles.settings.showPulseParts = logical(get(hObject,'Value'));

% update the data
guidata(hObject, handles);
updatePlots(hObject, true);


% --- Executes on button press in showModelSim.
function showModelSim_Callback(hObject, eventdata, handles)
% hObject    handle to showModelSim (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
handles = guidata(hObject);
handles.settings.showModelSim = logical(get(hObject,'Value'));

% update the data
guidata(hObject, handles);
updatePlots(hObject, true);


% --- Executes on button press in showOverview.
function showOverview_Callback(hObject, eventdata, handles)
% hObject    handle to showOverview (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of showOverview
handles = guidata(hObject);
handles.settings.showOverview = boolean(get(hObject,'Value'));
if (handles.settings.showOverview)
    set(hObject,'BackgroundColor', [0.8900    0.8900    0.8900]);
else
    set(hObject,'BackgroundColor', [0.9400    0.9400    0.9400]);
end
% update the data
guidata(hObject, handles);
updatePlots(hObject, true);


% --- Executes on button press in viewLiveData.
function viewLiveData_Callback(hObject, eventdata, handles)
% hObject    handle to viewLiveData (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
handles = guidata(hObject);
handles.settings.viewLiveData = boolean(get(hObject,'Value'));
if (handles.settings.viewLiveData)
    set(hObject,'BackgroundColor', [0.8900    0.8900    0.8900]);
    handles.settings.showOverview = false;
    handles.showOverview.Value = 0;
    handles.showOverview.BackgroundColor = [0.9400    0.9400    0.9400];
    % play sound?
    soundSel = questdlg('Should a PING sound be played if a new pulse arrives?', ...
        'Sound Notification', ...
        'YES, use Audio Feedback','Do NOT play Audio Feedback','Do NOT play Audio Feedback');
    % Handle response
    switch soundSel
        case 'YES, use Audio Feedback'
            handles.settings.viewLiveDataSound = true;
        case 'Do NOT play Audio Feedback'
            handles.settings.viewLiveDataSound = false;
        case ''
            handles.settings.viewLiveDataSound = false;
    end
    % update data struct
    PathName = fileparts(mfilename('fullpath'));
    handles.data = load([PathName, filesep, 'dummyData.mat']);
    handles = updateData(handles);
    handles.wpos.Value = 1;
    % Live Testbed object
    handles.dataSourceLive = tb_receiveTestbedData();
    % save data and update the plots
    guidata(hObject, handles);
    updateSliderSettings(hObject);
    updatePlots(hObject, true);
    updateHandles(hObject);
    
    % update the handles and add the timer
    handles = guidata(hObject);
    handles.dataTimer = timer();
    handles.dataTimer.Period = 0.09;
    handles.dataTimer.ExecutionMode = 'fixedRate';
    handles.dataTimer.TimerFcn = @dataTimerCallback;
    handles.dataTimer.StartDelay = 0.5;
    handles.dataTimer.UserData = hObject;
    guidata(hObject, handles);
    start(handles.dataTimer);
else
    % stop the live data
    stop(handles.dataTimer);
    handles.dataSourceLive.delete();
    % restore the Overview
    set(hObject,'BackgroundColor', [0.9400    0.9400    0.9400]);
    handles.settings.showOverview = true;
    handles.showOverview.Value = 1;
    handles.showOverview.BackgroundColor = [0.8900    0.8900    0.8900];
    dsr = handles.settings.downsamplingrate*2;
    % update the data
    if (handles.data.nPulses > 1)
        handles.data.pulse(1).time =  handles.data.pulse(2).time(1:dsr) - diff(handles.data.pulse(2).time(1:2))*dsr;
        handles.data.pulse(1).nSample = handles.data.pulse(2).nSample(1:dsr) -dsr;
    else
        handles.data.pulse(1).time =  (1:dsr)/1000;
        handles.data.pulse(1).nSample = 1:dsr;
    end
    handles.data.pulse(1).stimCurrent = zeros(dsr,1);
    handles.data.pulse(1).stimVoltage = zeros(dsr,1);
    handles.data.pulse(1).nSamples = dsr;
    handles.data.setup.iEnd = handles.data.nPulses;
    handles.data.setup.nAll = handles.data.nPulses;
    
    handles = updateData(handles);
    guidata(hObject, handles);
    updateSliderSettings(hObject);
    updatePlots(hObject, true);
    % update the dataSet GUI elements
    updateHandles(hObject);
    
end

function dataTimerCallback(obj, event)
handles = guidata(obj.UserData);
NPulsesOld = handles.data.nPulses;
handles.data = handles.dataSourceLive.updateTBdata(handles.data);

if (handles.data.nPulses > NPulsesOld)
    try
        handles.wpos.Value = 1;
        handles.settings.window_position = 2;
        handles.data.pulse(1) = handles.data.pulse(handles.data.nPulses);
        playSound = false; % play sound and make sure it gets not too much if more pulses come in
        if (handles.settings.viewLiveDataSound)
            if (handles.pingSound.counter <= 0)
                playSound = true;
                handles.pingSound.counter = handles.pingSound.counterDefault;
                handles.pingSound.counterDefault = round(handles.pingSound.counterDefault * 1.3);
            else
                handles.pingSound.counter = handles.pingSound.counter -1;
            end
        end
        guidata(obj.UserData, handles);
        updatePlots(obj.UserData, false);
        handles.textPulse.String = ['Pulse ', num2str(handles.data.nPulses)];
        if (playSound)
            sound(handles.pingSound.ping, handles.pingSound.Fs);
        end
    catch ME
        rethrow(ME);
    end
else
    if (handles.pingSound.counter ~= 0 || handles.pingSound.counterDefault ~= handles.pingSound.counterDefault2)
        handles.pingSound.counterDefault = handles.pingSound.counterDefault2;
        handles.pingSound.counter = handles.pingSound.counter -5;
        guidata(obj.UserData, handles);
    end
end


% --- Executes on button press in saveData.
function saveData_Callback(hObject, eventdata, handles)
% hObject    handle to saveData (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
handles = guidata(hObject);

[dataAmount] = listdlg('PromptString','Amount of Data to Save:',...
    'SelectionMode','single', 'ListSize', [590, 150], 'InitialValue', 3, ...
    'ListString',{  'Full RawData Set -> everything INcluding RAW INT16 data (100%)', ...
    'Full Data Set -> everything EXcuding the RAW INT16 Data (~8% reduction)', ...
    'Pulse Data Set -> all the data for the detected pulses (~54% reduction)', ...
    'Minimal Pulse Data Set -> the essential data for the detected pulses (~58% reduction)'});
if (isempty(dataAmount)), disp('   -> aborted'); return; end

[~,fname,~] = fileparts(handles.data_fname) ;
[filename, pathname] = uiputfile( {'*.mat'}, 'Save as', [handles.data_fpath, filesep, fname]);
if (isempty(filename)), disp('   -> aborted'); return; end

data = handles.data;
disp('Working ...');
if (dataAmount >= 4)
    % Minimal Pulse Data Set -> remove nonessential fields
    pulseNew = rmfield(data.pulse(1), 'nSample');
    for i = 2:length(data.pulse)
        pulseNew(i) = rmfield(data.pulse(i), 'nSample');
    end
    data.pulse = pulseNew;
end
if (dataAmount >= 3)
    % Pulse Data Set -> remove nonessential fields
    data = rmfield(data, 'pulseRaw');
end
if (dataAmount == 2)
    % Full Data Set -> remove nonessential fields
    pulseRawNew = rmfield(data.pulseRaw(1), {'stimVoltageRaw', 'stimCurrentRaw'});
    for i = 2:length(data.pulseRaw)
        pulseRawNew(i) = rmfield(data.pulseRaw(i), {'stimVoltageRaw', 'stimCurrentRaw'});
    end
    data.pulseRaw = pulseRawNew;
end
if (dataAmount >= 1)
    % Full RawData Set -> do notting
end

save([pathname, filename], 'data','-v7.3');
disp(['Data saved to:', char(10), '   -> ', pathname, filename]);

% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  ContinuousValueChange Callback functions
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% slider change callback -> is calles if the window size slider is moved
% update the window size information and all gui elements
function updateWindows(hObject, eventdata, handles)
% is called every time the window size slider is moved
% update the data
updatePlots(hObject, false);


% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Internal functions
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%% update the data to display, based on the settings
function handles = updateData(handles)
% resets
handles.time = [];
handles.curr = [];
handles.volt = [];
handles.time_window = {};
handles.curr_window = {};
handles.volt_window = {};
handles.channelIDs = [];

dsr = handles.settings.downsamplingrate;

% build overview data
if ( isempty(handles.data) || ~isfield(handles.data, 'pulse') || isempty(fieldnames((handles.data.pulse))) )
    handles.time = 0:0.1:10;
    handles.curr = zeros(size(handles.time));
    handles.volt = zeros(size(handles.time));
    handles.data_size = 0;
    disp([char(10), char(10), '   -> NO DATA TO DISPLAY ...']);
else
    if (handles.settings.channels <= 0)
        % show all channels
        handles.data_size = 0;
        for i=1:length(handles.data.pulse)
            handles.time = [handles.time; handles.data.pulse(i).time(1:dsr:end)];
            handles.curr = [handles.curr; handles.data.pulse(i).stimCurrent(1:dsr:end)];
            handles.curr(end) = 0;
            %handles.volt = [handles.volt; handles.data.pulse(i).stimVoltage(1:dsr:end)];
        end
        handles.data_size = handles.data.nPulses;
        handles.channelIDs = 1:handles.data_size;
    else
        % show only certan channels
        handles.data_size = 0;
        for i=1:length(handles.data.pulse)
            if (~isempty(intersect(handles.data.pulse(i).info.channel, handles.settings.channels)))
                handles.time = [handles.time; handles.data.pulse(i).time(1:dsr:end)];
                handles.curr = [handles.curr; handles.data.pulse(i).stimCurrent(1:dsr:end)];
                handles.volt = [handles.volt; handles.data.pulse(i).stimVoltage(1:dsr:end)];
                handles.data_size = handles.data_size +1;
                handles.channelIDs(end+1) = i;
            end
        end
    end
end


%% update the slider gui elements
function updateSliderSettings(hObject)
handles = guidata(hObject);
if (handles.data_size == 0), set(handles.wpos, 'Enable', 'off'); return; end
major_step = max([0.000001,  1/handles.data_size]);
minor_step = max([0.0000001, 1/handles.data_size]);
% update the window pos
set(handles.wpos, 'Max', handles.data_size);
set(handles.wpos, 'Min', 1);
set(handles.wpos, 'SliderStep', [minor_step major_step]);
if (handles.settings.window_position > get(handles.wpos, 'Max'))
    handles.settings.window_position = get(handles.wpos, 'Max');
end
set(handles.wpos, 'Value', handles.settings.window_position);
if (handles.data.nPulses <= 1)
    set(handles.wpos, 'Enable', 'off');
else
    set(handles.wpos, 'Enable', 'on');
end
% update the data
guidata(hObject, handles);


%% update the text elements
function updateHandles(hObject)
h = guidata(hObject);
%% text handles
% Stats
stats = 'NO DATA TO DISPLAY';
if (h.settings.viewLiveData )
    stats = ['LIVE DATA'];
else
    if (isfield(h.data, 'setup') && isfield(h.data.setup, 'iStart'))
    stats = ['Pulse ', num2str(h.data.setup.iStart), ' - ',...
        num2str(h.data.setup.iEnd), ' from ', num2str(h.data.setup.nAll),...
        ' Pulses; '];
    end
end
h.textStats.String = stats;

%% DataSet handles
if (isfield(h.data, 'setup') && isfield(h.data.setup, 'nAll') && ~isempty(h.data.setup.nAll))
    if ((h.data.setup.iStart ~=1) || (h.data.setup.iEnd < h.data.setup.nAll))
        % multiple data sets
        if (h.data.setup.iStart ~=1)
            h.pDataSet.Enable = 'on';
        else
            h.pDataSet.Enable = 'off';
        end
        h.nPulsesDataSet.Enable = 'on';
        if (h.data.setup.iEnd < h.data.setup.nAll)
            h.nDataSet.Enable = 'on';
        else
            h.nDataSet.Enable = 'off';
        end
    else
        h.pDataSet.Enable = 'off';
        h.nPulsesDataSet.Enable = 'off';
        h.nDataSet.Enable = 'off';
    end
else
    h.pDataSet.Enable = 'off';
    h.nPulsesDataSet.Enable = 'off';
    h.nDataSet.Enable = 'off';
end


%% updates gui elements
function updatePlots(hObject, do_plot)
handles = guidata(hObject);
if (handles.data_size == 0)
    % return if no data is loaded
    axes(handles.axes_curr);
    cla;
    axes(handles.axes_volt);
    cla;
    return
end
%update settings
window_position = round(get(handles.wpos, 'Value'));
if (~do_plot && window_position == handles.settings.window_position)
    return;
end
handles.settings.window_position = window_position;

% update the boundaries
handles.settings.i_start = handles.settings.window_position;
handles.settings.i_stop  = handles.settings.i_start -(handles.settings.nPulses-1);
pulses = handles.settings.i_stop : handles.settings.i_start;
pulses(find(pulses >0)) = handles.channelIDs(pulses(find(pulses >0)));
handles.time_window = {};
handles.curr_window = {};
handles.volt_window = {};
for i = 1:length(pulses)
    % window data -> get the selected data + time
    if (handles.settings.showOverview)
        if (pulses(i)<=0), continue; end
        handles.time_window{end+1} = handles.data.pulse(pulses(i)).time;
        %handles.curr_window{end+1} = handles.data.pulse(pulses(i)).stimCurrent;
        handles.curr_window{end+1} = getValidCurrent( handles.data.pulse(pulses(i)).stimCurrent, handles.data.pulse(pulses(i)).stats, handles.settings.maxCurrent);
        handles.volt_window{end+1} = handles.data.pulse(pulses(i)).stimVoltage;
    else
        if (pulses(i)<=0)
            handles.time_window{i} = handles.data.pulse(pulses(end)).time -handles.data.pulse(pulses(end)).time( handles.data.pulse(pulses(end)).info.offsets(1) );
            handles.curr_window{i} = zeros(size(handles.data.pulse(pulses(end)).time));
            handles.volt_window{i} = handles.curr_window{i};
        else
            handles.time_window{i} = handles.data.pulse(pulses(i)).time -handles.data.pulse(pulses(i)).time( handles.data.pulse(pulses(i)).info.offsets(1) );
            %handles.curr_window{i} = handles.data.pulse(pulses(i)).stimCurrent;
            handles.curr_window{i} = getValidCurrent( handles.data.pulse(pulses(i)).stimCurrent, handles.data.pulse(pulses(i)).stats, handles.settings.maxCurrent);
            handles.volt_window{i} = handles.data.pulse(pulses(i)).stimVoltage;
        end
    end
end
if (isfield(handles.data.pulse(pulses(end)), 'info'))
    handles.parts_window = handles.data.pulse(pulses(end)).info.parts;
else
    handles.parts_window = [];
end
if (handles.settings.showModelSim && isfield(handles.data.pulse(pulses(end)), 'model') && isfield(handles.data.pulse(pulses(end)).model, 'Vsim'))
    handles.voltSim_window = handles.data.pulse(pulses(end)).model.Vsim;
else
    handles.voltSim_window = [];
end

% update the data
guidata(hObject, handles);

% update the plot
if (do_plot)
    % make new plots
    doPlots(hObject, handles, pulses);
else
    % update the data that is displayed in the plots
    doPlotsUpdates(hObject, handles, pulses)
end

function current = getValidCurrent(current, stats, maxCurrent)
if (maxCurrent == -1)
    
else
    current(find(current > abs(maxCurrent)))    = abs(maxCurrent);
    current(find(current < abs(maxCurrent)*-1)) = abs(maxCurrent)*-1;
end

function doPlots(hObject, handles, pulses)
%% the funtion is called from the gui -> paint directly

% current
axes(handles.axes_curr);
cla; hold on;
legStr = {};
lD = length(handles.time_window) +1;
if (handles.settings.showOverview)
    handles.plot.curr_data = plot(handles.axes_curr, handles.time, handles.curr, 'Color', handles.plot.cmap(1,:));
    legStr{end+1} = ['all ', num2str(handles.data.nPulses), ' pulses of ', strrep(handles.data_fname, '_', '\_')];
    handles.plot.curr_window{1} = plot(handles.axes_curr, handles.time_window{end}, handles.curr_window{end}, 'r-');
    legStr{end+1} = 'selected pulse';
    xlim([min(handles.time), max(handles.time)]);
else
    pulsesRel = 0:length(pulses)-1;
    for i = length(handles.time_window):-1:1
        handles.plot.curr_window{i} = plot(handles.axes_curr, handles.time_window{i}, handles.curr_window{i}, 'Color', handles.plot.cmap(lD-i,:));
        temp = ''; if (pulsesRel(i) ~= 0), temp = [' -', num2str(pulsesRel(i))]; end
        legStr{i} = ['current of pulse X', temp];
    end
    handles.axes_curr.XLimMode = 'auto';
end
grid on; hold off;
legend(legStr);

% voltage
axes(handles.axes_volt);
cla; hold on;
legStr = {};
if (handles.settings.showOverview)
    handles.plot.volt_window{1} = plot(handles.axes_volt, handles.time_window{end}, handles.curr_window{end}, 'Color', handles.plot.cmap(1,:));
    legStr{end+1} = 'current of the selected pulse';
    handles.plot.volt_window{2} = plot(handles.axes_volt, handles.time_window{end}, handles.volt_window{end}, 'Color', handles.plot.cmap(2,:));
    legStr{end+1} = 'voltage of the selected pulse';
    %xlim([min(handles.time_window{end}), max(handles.time_window{end})]);
    handles.textPulse.String = ['Pulse ', num2str(pulses(end))];
else
    pulsesRel = 0:length(pulses)-1;
    for i = length(handles.time_window):-1:1
        handles.plot.volt_window{i} = plot(handles.axes_volt, handles.time_window{i}, handles.volt_window{i}, 'Color', handles.plot.cmap(lD-i,:));
        temp = ''; if (pulsesRel(i) ~= 0), temp = [' -', num2str(pulsesRel(i))]; end
        legStr{i} = ['voltage of pulse X', temp];
    end
    handles.axes_volt.XLimMode = 'auto';
end

% Pulse Info
handles.textPulse.String = ['Pulse ', num2str(pulses(end))];
if (~handles.settings.showOverview)
    temp = num2str(pulses(find(pulses(1:end-1)>0)));
    if (~isempty(temp))
        temp = [' (+', temp, ')'];
    end
    handles.textPulse2.String = temp;
end
% Model Simulation Voltage
if (handles.settings.showModelSim && ~isempty(handles.voltSim_window))
    handles.plot.voltSim_window = plot(handles.axes_volt, handles.time_window{end}, handles.voltSim_window, 'g-.');
    legStr{end+1} = 'simulated voltage based on the skin model';
end
grid on; hold off;
legend(legStr);
% Timing marks
if (handles.settings.showPulseParts)
    if (handles.settings.showOverview)
        handles.plot.parts = addTimingPlots(handles, handles.axes_volt);
    else
        % volt first so the order is identical for both cases
        handles.plot.parts = addTimingPlots(handles, handles.axes_volt);
        handles.plot.parts = [handles.plot.parts,  addTimingPlots(handles, handles.axes_curr)];
    end
end
% update the data
guidata(hObject, handles);

function h = addTimingPlots(handles, a)
h = {};
axes(a); hold on;
yl = a.YLim';
for i = 1:size(handles.parts_window,1)
    h{i} = struct();
    h{i}.a = plot(ones(2,1)*handles.time_window{end}(handles.parts_window(i,1)), yl, 'r--');
    h{i}.b = plot(ones(2,1)*handles.time_window{end}(handles.parts_window(i,2)), yl, 'r--');
end
hold off


function doPlotsUpdates(hObject, handles, pulses)
%% the funtion is called from an Action-Handler -> paint indiret
try
    if (handles.settings.showOverview)
        % curent
        set(handles.plot.curr_window{1}, 'xdata', handles.time_window{end});
        set(handles.plot.curr_window{1}, 'ydata', handles.curr_window{end});
        % voltage
        handles.textPulse.String = ['Pulse ', num2str(pulses(end))];
        set(handles.plot.volt_window{1}, 'xdata', handles.time_window{end});
        set(handles.plot.volt_window{1}, 'ydata', handles.curr_window{end});
        set(handles.plot.volt_window{2}, 'xdata', handles.time_window{end});
        set(handles.plot.volt_window{2}, 'ydata', handles.volt_window{end});
    else
        for i = 1:length(handles.time_window)
            % curent
            set(handles.plot.curr_window{i}, 'xdata', handles.time_window{i});
            set(handles.plot.curr_window{i}, 'ydata', handles.curr_window{i});
            % voltage
            set(handles.plot.volt_window{i}, 'xdata', handles.time_window{i});
            set(handles.plot.volt_window{i}, 'ydata', handles.volt_window{i});
        end
        temp = num2str(pulses(find(pulses(1:end-1)>0)));
        if (~isempty(temp))
            temp = [' (+', temp, ')'];
        end
        handles.textPulse.String = ['Pulse ', num2str(pulses(end))];
        handles.textPulse2.String = temp;
    end
    % Model Simulation Voltage
    if (handles.settings.showModelSim)
        if (~isempty(handles.voltSim_window))
            set(handles.plot.voltSim_window, 'xdata', handles.time_window{end});
            set(handles.plot.voltSim_window, 'ydata', handles.voltSim_window);
        else
            set(handles.plot.voltSim_window, 'xdata', handles.time_window{end});
            set(handles.plot.voltSim_window, 'ydata', zeros(size(handles.time_window{end})));
        end
    end
    % Timing marks
    if (handles.settings.showPulseParts)
        % clear data
        for j = 1:length(handles.plot.parts)
            set( handles.plot.parts{j}.a, 'ydata', [0;0] );
            set( handles.plot.parts{j}.b, 'ydata', [0;0] );
        end
        k = 1;
        for j = 1:(length(handles.plot.parts)/size(handles.parts_window,1))
            if (j == 1)
                yl = handles.axes_volt.YLim';
            else
                yl = handles.axes_curr.YLim';
            end
            for i = 1:size(handles.parts_window,1)
                set( handles.plot.parts{k}.a, 'xdata', ones(2,1)*handles.time_window{end}(handles.parts_window(i,1)) );
                set( handles.plot.parts{k}.a, 'ydata', yl );
                set( handles.plot.parts{k}.b, 'xdata', ones(2,1)*handles.time_window{end}(handles.parts_window(i,2)) );
                set( handles.plot.parts{k}.b, 'ydata', yl );
                k = k+1;
            end
        end
    end
catch ME
    disp(['Error updating current / voltage plots ...', char(10), 'ID: ' ME.identifier])
    %rethrow(ME)
end
% update the data
guidata(hObject, handles);



function nPulsesDataSet_Callback(hObject, eventdata, handles)
% hObject    handle to nPulsesDataSet (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'String') returns contents of nPulsesDataSet as text
%        str2double(get(hObject,'String')) returns contents of nPulsesDataSet as a double
handles.nPulsesDataSet.Value = eval(handles.nPulsesDataSet.String);
N = handles.nPulsesDataSet.Value;
iStart = handles.data.setup.iStart;
getNewDataSet(hObject, handles, iStart, N, handles.data.setup);


% --- Executes during object creation, after setting all properties.
function nPulsesDataSet_CreateFcn(hObject, eventdata, handles)
% hObject    handle to nPulsesDataSet (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on button press in pDataSet.
function pDataSet_Callback(hObject, eventdata, handles)
% hObject    handle to pDataSet (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
N = handles.nPulsesDataSet.Value;
iStart = max([1, (handles.data.setup.iStart -N)]);
getNewDataSet(hObject, handles, iStart, N, handles.data.setup);

% --- Executes on button press in nDataSet.
function nDataSet_Callback(hObject, eventdata, handles)
% hObject    handle to nDataSet (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
N = handles.nPulsesDataSet.Value;
iStart = min([(handles.data.setup.iEnd +1), handles.data.setup.nAll]);
getNewDataSet(hObject, handles, iStart, N, handles.data.setup);

% --- Executes on button press in buildOverview.
function buildOverview_Callback(hObject, eventdata, handles)
% hObject    handle to buildOverview (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
if ((handles.data.setup.iStart ~=1) || (handles.data.setup.iEnd < handles.data.setup.nAll))
    dataSetSel = questdlg('Do you want the build the overview over this data set, or over ALL data sets?', ...
        'Select Data Source', ...
        'This Data Set','ALL Data Sets','This Data Set');
    % Handle response
    switch dataSetSel
        case 'This Data Set'
            useEntireDataSet = false;
        case 'ALL Data Sets'
            useEntireDataSet = true;
        case ''
            return;
    end
else
    useEntireDataSet = false;
end
tb_buildOverview( handles.data, true, useEntireDataSet );


function ds = getNewDataSet(hObject, handles, iStart, N, param)
disp(['TESTBED Get Pulse ', num2str(iStart), ' - ' num2str(iStart +N -1),' from ', num2str(param.nAll), ' Pulses:']);
desc = param.desc;
setup = struct('iStart', iStart, 'N', N, 'nAll', param.nAll, 'pulseType', param.pulseType, 'skipDesc', true);
if (isfield(param, 'fPositionDataFile')), setup.fPositionDataFile = param.fPositionDataFile; end
ds = tb_readTestbedDataFile([param.fpath, filesep, param.fname, param.fext], setup );
ds = tb_ppTestbedData(ds);
ds.setup.desc = desc;

% update the data
handles.data = ds;
handles.data_fname = 'unknown';
handles.data_fpath  = '';
if (isfield(handles.data, 'setup'))
    if (isfield(handles.data.setup, 'filename'))
        handles.data_fname = handles.data.setup.filename;
    end
    if (isfield(handles.data.setup, 'path'))
        handles.data_fpath = handles.data.setup.path;
    end
end
handles.output = handles.data;
% update the data
handles = updateData(handles);
guidata(hObject, handles);
updateSliderSettings(hObject);
updatePlots(hObject, true);
% update the dataSet GUI elements
updateHandles(hObject);
