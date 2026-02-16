//-----------------------------------------------------------------------------
// PluginHost.cpp
//-----------------------------------------------------------------------------

#include "PluginHost.h"
#include "Utilities.h"

#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <iostream>
#include <algorithm>

//-----------------------------------------------------------------------------
// constructor/destructor
//-----------------------------------------------------------------------------
CK_DLL_CTOR(pluginhost_ctor);
CK_DLL_DTOR(pluginhost_dtor);

//-----------------------------------------------------------------------------
// parameter functions
//-----------------------------------------------------------------------------
CK_DLL_MFUN(pluginhost_setParam);
CK_DLL_MFUN(pluginhost_getParam);
CK_DLL_MFUN(pluginhost_getParamName);
CK_DLL_MFUN(pluginhost_getParamLabel);
CK_DLL_MFUN(pluginhost_getParamDisplay);
CK_DLL_MFUN(pluginhost_numParams);
CK_DLL_MFUN(pluginhost_numNonMidiParams);
CK_DLL_MFUN(pluginhost_findParam);

//-----------------------------------------------------------------------------
// program functions
//-----------------------------------------------------------------------------
CK_DLL_MFUN(pluginhost_numPrograms);
CK_DLL_MFUN(pluginhost_program);
CK_DLL_MFUN(pluginhost_getProgram);
CK_DLL_MFUN(pluginhost_programName);

//-----------------------------------------------------------------------------
// other functions
//-----------------------------------------------------------------------------
CK_DLL_MFUN(pluginhost_load);
CK_DLL_MFUN(pluginhost_name);
CK_DLL_MFUN(pluginhost_vendor);
CK_DLL_MFUN(pluginhost_saveState);
CK_DLL_MFUN(pluginhost_loadState);
CK_DLL_MFUN(pluginhost_showEditor);
CK_DLL_MFUN(pluginhost_hideEditor);
CK_DLL_MFUN(pluginhost_asyncEventRunning);
CK_DLL_MFUN(pluginhost_waitForAsyncEvents);
CK_DLL_MFUN(pluginhost_setForceSynchronous);
CK_DLL_MFUN(pluginhost_getForceSynchronous);
CK_DLL_MFUN(pluginhost_setBlockSize);
CK_DLL_MFUN(pluginhost_getBlockSize);
CK_DLL_MFUN(pluginhost_latency);
CK_DLL_MFUN(pluginhost_setBypass);
CK_DLL_MFUN(pluginhost_getBypass);
CK_DLL_MFUN(pluginhost_reset);
CK_DLL_MFUN(pluginhost_numInputs);
CK_DLL_MFUN(pluginhost_numOutputs);
CK_DLL_MFUN(pluginhost_setRealtime);
CK_DLL_MFUN(pluginhost_getRealtime);

//-----------------------------------------------------------------------------
// playhead functions
//-----------------------------------------------------------------------------
CK_DLL_MFUN(pluginhost_bpm);
CK_DLL_MFUN(pluginhost_getBpm);
CK_DLL_MFUN(pluginhost_timeSig);
CK_DLL_MFUN(pluginhost_pos);
CK_DLL_MFUN(pluginhost_getPos);
CK_DLL_MFUN(pluginhost_playing);
CK_DLL_MFUN(pluginhost_getPlaying);
CK_DLL_MFUN(pluginhost_recording);
CK_DLL_MFUN(pluginhost_getRecording);
CK_DLL_MFUN(pluginhost_lastBarPos);
CK_DLL_MFUN(pluginhost_getLastBarPos);
CK_DLL_MFUN(pluginhost_looping);
CK_DLL_MFUN(pluginhost_getLooping);
CK_DLL_MFUN(pluginhost_loopPoints);
CK_DLL_MFUN(pluginhost_loopStart);
CK_DLL_MFUN(pluginhost_getLoopStart);
CK_DLL_MFUN(pluginhost_loopEnd);
CK_DLL_MFUN(pluginhost_getLoopEnd);

//-----------------------------------------------------------------------------
// MIDI functions
//-----------------------------------------------------------------------------
CK_DLL_MFUN(pluginhost_noteOn);
CK_DLL_MFUN(pluginhost_noteOn_default);
CK_DLL_MFUN(pluginhost_noteOff);
CK_DLL_MFUN(pluginhost_noteOff_default);
CK_DLL_MFUN(pluginhost_allNotesOff);
CK_DLL_MFUN(pluginhost_allNotesOff_default);
CK_DLL_MFUN(pluginhost_pitchBend);
CK_DLL_MFUN(pluginhost_pitchBend_default);
CK_DLL_MFUN(pluginhost_aftertouch);
CK_DLL_MFUN(pluginhost_aftertouch_default);
CK_DLL_MFUN(pluginhost_aftertouchChannel);
CK_DLL_MFUN(pluginhost_aftertouchChannel_default);
CK_DLL_MFUN(pluginhost_controlChange);
CK_DLL_MFUN(pluginhost_controlChange_default);
CK_DLL_MFUN(pluginhost_midiMsg);
CK_DLL_MFUN(pluginhost_addQWERTYMidiInput);
CK_DLL_MFUN(pluginhost_removeQWERTYMidiInput);
CK_DLL_MFUN(pluginhost_toggleQWERTYMidiInput);

//-----------------------------------------------------------------------------
// tick function
//-----------------------------------------------------------------------------
CK_DLL_TICKF(pluginhost_tick);

// data offset for internal class
t_CKINT pluginhost_data_offset = 0;


//-----------------------------------------------------------------------------
// PluginHost Implementation
//-----------------------------------------------------------------------------

//-------------------------------------------------------------------------
// constructor/destructor
//-------------------------------------------------------------------------
PluginHost::PluginHost( t_CKFLOAT fs )
:
  m_renderBuffer(maxChannels, 16),
  m_inputBuffer(maxChannels, maxBufferSize + 1),
  m_outputBuffer(maxChannels, maxBufferSize + 1)
{
    m_srate = fs;
    // default block size
    m_blockSize = 16;
    // resize render buffer to match default block size
    m_renderBuffer.setSize(maxChannels, m_blockSize);
    m_renderBuffer.clear();
    
    // register plugin formats
    m_formatManager.addDefaultFormats();
}

PluginHost::~PluginHost()
{
    // wait for any pending async events just in case
    waitForAsyncEvents(100);

    // delete the plugin instance on the message thread
    if (m_plugin)
    {
        // detach playhead before destruction as m_playHead will be destroyed
        // should maybe extend the lifetime of the playhead instead
        m_plugin->setPlayHead(nullptr);

        // destroy the plugin on the main thread
        std::shared_ptr<juce::AudioPluginInstance> plugin = std::move(m_plugin);
        juce::MessageManager::callAsync([plugin]() {});
    }

    // destroy qwerty window if it exists
    if (m_qwertyWindow)
    {
        std::shared_ptr<QWERTYMidiWindow> window = std::move(m_qwertyWindow);
        juce::MessageManager::callAsync([window]() {});
    }
}

//-------------------------------------------------------------------------
// tick
//-------------------------------------------------------------------------
void PluginHost::tick( SAMPLE * in, SAMPLE * out, int nframes )
{
    constexpr int numChannels = maxChannels;

    // fine when there is no contention
    juce::SpinLock::ScopedLockType lock(m_audioLock);

    // advance playhead if playing
    constexpr bool advancePlayhead = false;
    if (advancePlayhead && m_playHead.getPlaying())
    {
        const double bpm = m_playHead.getBpm();
        const double samplesPerBeat = (m_srate * 60.0) / bpm;
        // very susceptible to floating point errors - a tempo map is really needed for proper playhead support...
        const double ppqDelta = nframes / samplesPerBeat;

        m_playHead.setPpqPosition(m_playHead.getPpqPosition() + ppqDelta);
        m_playHead.setTimeInSamples(m_playHead.getTimeInSamples() + nframes);
        m_playHead.setTimeInSeconds(m_playHead.getTimeInSeconds() + (double)nframes / m_srate);
    }

    if (nframes == m_blockSize)
    {
        // clear old output midi
        m_outputMidi.clear();
        if (m_inputMidi.getNumEvents() > 0)
        {
            m_outputMidi.addEvents(m_inputMidi, 0, m_blockSize, 0);
            m_inputMidi.clear();
        }

        // inject keyboard MIDI
        m_keyboardState.processNextMidiBuffer(m_outputMidi, 0, nframes, true);

        if (m_plugin)
        {
            // de-interleave input to m_renderBuffer
            for(int c = 0; c < numChannels; c++)
            {
                float* dest = m_renderBuffer.getWritePointer(c);
                for(int f = 0; f < nframes; f++)
                    dest[f] = in[f * numChannels + c];
            }

            // check the number of channels that a plugin actually wants (some might require sidechain inputs)
            const int totalNumChannels = std::max(m_plugin->getTotalNumInputChannels(), m_plugin->getTotalNumOutputChannels());
            // currently we don't do anything to accomodate this, but we eventually will make sure plugins get the channels they want
            if (totalNumChannels > maxChannels)
                std::cout << "PluginHost: Channel mismatch, this might cause issues..." << std::endl;

            m_plugin->processBlock(m_renderBuffer, m_outputMidi);

            // interleave output from m_renderBuffer
            for(int c = 0; c < numChannels; c++)
            {
                const float* src = m_renderBuffer.getReadPointer(c);
                for(int f = 0; f < nframes; f++)
                    out[f * numChannels + c] = src[f];
            }
        }
        else
        {
            // passthrough
            for(int i = 0; i < nframes * numChannels; i++)
                out[i] = in[i];
        }
        return;
    }

    for(int f = 0; f < nframes; f++)
    {
        float inputs[numChannels];
        for(int c = 0; c < numChannels; c++)
            inputs[c] = in[f * numChannels + c];
        m_inputBuffer.push(inputs, numChannels);

        // check if we have enough samples to process a block
        if (m_inputBuffer.getAvailableSamples() >= m_blockSize)
        {
            if (m_inputBuffer.pop(m_renderBuffer))
            {
                // clear old output midi
                m_outputMidi.clear();
                if (m_inputMidi.getNumEvents() > 0)
                {
                    m_outputMidi.addEvents(m_inputMidi, 0, m_blockSize, 0);
                    m_inputMidi.clear();
                }

                // inject keyboard MIDI
                m_keyboardState.processNextMidiBuffer(m_outputMidi, 0, m_blockSize, true);

                if (m_plugin)
                {
                    // check the number of channels that a plugin actually wants (some might require sidechain inputs)
                    const int totalNumChannels = std::max(m_plugin->getTotalNumInputChannels(), m_plugin->getTotalNumOutputChannels());
                    // currently we don't do anything to accomodate this, but we eventually will make sure plugins get the channels they want
                    if (totalNumChannels > maxChannels)
                        std::cout << "PluginHost: Channel mismatch, this might cause issues..." << std::endl;

                    m_plugin->processBlock(m_renderBuffer, m_outputMidi);
                }
                
                m_outputBuffer.push(m_renderBuffer);
            }
        }

        float outputs[numChannels];
        m_outputBuffer.pop(outputs, numChannels);
        
        for(int c = 0; c < numChannels; c++)
            out[f * numChannels + c] = outputs[c];
    }
}

//-------------------------------------------------------------------------
// parameter accessors
//-------------------------------------------------------------------------
int PluginHost::getNumParams()
{
    if (!m_plugin) return 0;
    return m_plugin->getParameters().size();
}

int PluginHost::getNumNonMidiParams()
{
    if (!m_plugin) return 0;
    int count = 0;
    auto& params = m_plugin->getParameters();
    for (auto* p : params)
    {
        if (!p->getName(128).startsWith("MIDI CC"))
            count++;
    }
    return count;
}

std::string PluginHost::getParamName(int index)
{
    if (!m_plugin) return "";
    auto& params = m_plugin->getParameters();
    if (index < 0 || index >= params.size()) return "";
    return params[index]->getName(128).toStdString();
}

float PluginHost::getParam(int index)
{
    if (!m_plugin) return 0.0f;
    auto& params = m_plugin->getParameters();
    if (index < 0 || index >= params.size()) return 0.0f;
    return params[index]->getValue();
}

float PluginHost::setParam(int index, float val)
{
    if (!m_plugin) return val;
    auto& params = m_plugin->getParameters();
    if (index < 0 || index >= params.size()) return val;
    params[index]->setValue(val);
    return val;
}

int PluginHost::findParam(const std::string& name)
{
    if (!m_plugin) return -1;
    auto& params = m_plugin->getParameters();
    for (int i = 0; i < params.size(); ++i)
    {
        if (params[i]->getName(128).toStdString() == name)
            return i;
    }
    return -1;
}

std::string PluginHost::getParamLabel(int index)
{
    if (!m_plugin) return "";
    auto& params = m_plugin->getParameters();
    if (index < 0 || index >= params.size()) return "";
    return params[index]->getLabel().toStdString();
}

std::string PluginHost::getParamDisplay(int index)
{
    if (!m_plugin) return "";
    auto& params = m_plugin->getParameters();
    if (index < 0 || index >= params.size()) return "";
    return params[index]->getCurrentValueAsText().toStdString();
}

//-------------------------------------------------------------------------
// metadata
//-------------------------------------------------------------------------
std::string PluginHost::getName() const
{
    return m_plugin ? m_plugin->getName().toStdString() : "";
}

std::string PluginHost::getVendor() const
{
    return m_plugin ? m_plugin->getPluginDescription().manufacturerName.toStdString() : "";
}

//-------------------------------------------------------------------------
// load / state
//-------------------------------------------------------------------------
void PluginHost::loadPlugin(const std::string& path)
{
    juce::File file(path);
    if (!file.exists())
    {
        std::cout << "PluginHost: File does not exist: " << path << std::endl;
        return;
    }

    callOnMainThread([this, file, context = createAsyncEventContext()]
    {
        juce::AudioPluginFormat* format = nullptr;
        for (int i = 0; i < m_formatManager.getNumFormats(); ++i)
        {
            auto* f = m_formatManager.getFormat(i);
            std::cout << f->getName() << std::endl;
            if (f->fileMightContainThisPluginType(file.getFullPathName()))
            {
                format = f;
                break;
            }
        }

        if (!format)
        {
            std::cout << "PluginHost: No format found for file " << file.getFileName() << std::endl;
            return;
        }

        juce::OwnedArray<juce::PluginDescription> descriptions;
        // use KnownPluginList to scan and add the file
        m_knownPluginList.scanAndAddFile(file.getFullPathName(), false, descriptions, *format);
        
        if (descriptions.size() == 0)
        {
            std::cout << "PluginHost: No plugin descriptions found in file." << std::endl;
            return;
        }

        std::cout << "PluginHost: Found " << descriptions.size() << " plugin descriptions. Loading the first one..." << std::endl;
        
        // destroy existing plugin asynchronously
        {
            std::shared_ptr<juce::AudioPluginInstance> plugin = std::move(m_plugin);
            juce::MessageManager::callAsync([plugin]() {});
        }

        const auto callback = [this, context](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error)
        {
            if (!instance)
            {
                std::cout << "PluginHost: Failed to load plugin: " << error << std::endl;
                return;
            }

            {
                instance->prepareToPlay(m_srate, m_blockSize);
                instance->setPlayHead(&m_playHead);

                // request normal stereo layout
                juce::AudioProcessor::BusesLayout normalLayout;
                normalLayout.inputBuses.add(juce::AudioChannelSet::stereo());
                normalLayout.outputBuses.add(juce::AudioChannelSet::stereo());
                
                if (instance->checkBusesLayoutSupported(normalLayout))
                    instance->setBusesLayout(normalLayout);
                else
                {
                    // the plugin doesn't like the normal layout it is going to force some other layout
                }

                m_plugin = std::move(instance);
            }
            std::cout << "PluginHost: Successfully loaded: " << m_plugin->getName() << std::endl;

            constexpr bool displayEditor = false;
            if (displayEditor)
                showEditor();
        };

        // create the plugin instance asynchronously
        format->createPluginInstanceAsync(*descriptions[0], m_srate, m_blockSize, callback);
    });

    // if we are forcing synchronicity, wait for the plugin to load
    if (m_forceSynchronous)
        waitForAsyncEvents();
}

void PluginHost::showEditor()
{
    callOnMainThread([this, context = createAsyncEventContext()]
    {
        // if the editor already exists, just bring it to the front
        if (m_editor)
        {
            m_editor->toFront(true);
            return;
        }
        
        // make sure that there is a plugin and that the plugin has an editor
        if (!m_plugin || !m_plugin->hasEditor())
            return;
        
        // create the editor
        if (auto* editor = m_plugin->createEditorIfNeeded())
        {
            ensureForegroundProcess();

            // wrap the editor in a window
            auto* window = new PluginEditorWindow(*this, editor);
            window->addToDesktop();
            window->toFront(true);
            window->onClose = [this]() { m_editor.reset(); };
            m_editor.reset(window);
        }
    });
}

void PluginHost::hideEditor()
{
    callOnMainThread([this, context = createAsyncEventContext()] { m_editor.reset(); });
}

void PluginHost::saveState(const std::string& path)
{
    callOnMainThread([this, path, context = createAsyncEventContext()]
    {
        if (!m_plugin)
        {
            std::cout << "PluginHost: No plugin loaded." << std::endl;
            return;
        }

        juce::MemoryBlock destData;
        m_plugin->getStateInformation(destData);

        juce::File file(path);
        if (file.replaceWithData(destData.getData(), destData.getSize()))
            std::cout << "PluginHost: State saved to " << path << std::endl;
        else
            std::cout << "PluginHost: Failed to save state to " << path << std::endl;
    });
}

void PluginHost::loadState(const std::string& path)
{
    callOnMainThread([this, path, context = createAsyncEventContext()]
    {
        if (!m_plugin)
        {
            std::cout << "PluginHost: No plugin loaded." << std::endl;
            return;
        }

        juce::File file(path);
        if (!file.existsAsFile())
        {
            std::cout << "PluginHost: File does not exist: " << path << std::endl;
            return;
        }

        juce::MemoryBlock destData;
        if (file.loadFileAsData(destData))
        {
            m_plugin->setStateInformation(destData.getData(), (int)destData.getSize());
            std::cout << "PluginHost: State loaded from " << path << std::endl;
        }
        else
            std::cout << "PluginHost: Failed to load state from " << path << std::endl;
    });
}

//-------------------------------------------------------------------------
// async / sync
//-------------------------------------------------------------------------
bool PluginHost::asyncEventRunning() const
{
    return m_asyncEventCount > 0;
}

void PluginHost::waitForAsyncEvents(int timeoutMs) const
{
    const auto start = std::chrono::steady_clock::now();
    while (m_asyncEventCount > 0)
    {
        if (timeoutMs >= 0)
        {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
            if (elapsed > timeoutMs)
                break;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void PluginHost::setForceSynchronous(bool b)
{
    m_forceSynchronous = b;
}

bool PluginHost::getForceSynchronous() const
{
    return m_forceSynchronous;
}

//-------------------------------------------------------------------------
// processing config
//-------------------------------------------------------------------------
void PluginHost::setBlockSize(int size)
{
    if (size <= 0) return;

    callOnMainThread([this, size, context = createAsyncEventContext()]
    {
        juce::SpinLock::ScopedLockType lock(m_audioLock);
        m_blockSize = std::min(size, maxBufferSize);
        m_renderBuffer.setSize(maxChannels, m_blockSize);

        if (m_plugin)
            m_plugin->prepareToPlay(m_srate, m_blockSize);
    });
}

int PluginHost::getBlockSize() const
{
    return m_blockSize;
}

int PluginHost::getLatency() const
{
    return m_plugin ? m_plugin->getLatencySamples() : 0;
}

void PluginHost::setBypass(bool b)
{
    if (m_plugin) m_plugin->suspendProcessing(b);
}

bool PluginHost::getBypass() const
{
    return m_plugin ? m_plugin->isSuspended() : false;
}

void PluginHost::reset()
{
    if (m_plugin) m_plugin->reset();
}

int PluginHost::getNumInputs() const
{
    return m_plugin ? m_plugin->getTotalNumInputChannels() : 0;
}

int PluginHost::getNumOutputs() const
{
    return m_plugin ? m_plugin->getTotalNumOutputChannels() : 0;
}

void PluginHost::setRealtime(bool b)
{
    if (m_plugin) m_plugin->setNonRealtime(!b);
}

bool PluginHost::isRealtime() const
{
    return m_plugin ? !m_plugin->isNonRealtime() : false;
}

//-------------------------------------------------------------------------
// program functions
//-------------------------------------------------------------------------
int PluginHost::getNumPrograms()
{
    if (!m_plugin) return 0;
    return m_plugin->getNumPrograms();
}

int PluginHost::getCurrentProgram()
{
    if (!m_plugin) return 0;
    return m_plugin->getCurrentProgram();
}

void PluginHost::setCurrentProgram(int index)
{
    if (!m_plugin) return;
    if (index < 0 || index >= m_plugin->getNumPrograms()) return;

    callOnMainThread([this, index, context = createAsyncEventContext()]
    {
        m_plugin->setCurrentProgram(index);
    });
}

std::string PluginHost::getProgramName(int index)
{
    if (!m_plugin) return "";
    if (index < 0 || index >= m_plugin->getNumPrograms()) return "";
    return m_plugin->getProgramName(index).toStdString();
}

//-------------------------------------------------------------------------
// playHead accessors
//-------------------------------------------------------------------------
float PluginHost::setBpm(float b) { m_playHead.setBpm(b); return b; }
float PluginHost::getBpm() { return m_playHead.getBpm(); }
void PluginHost::setTimeSig(int n, int d) { m_playHead.setTimeSignature(n, d); }
float PluginHost::setPos(float p) { m_playHead.setPpqPosition(p); return p; }
float PluginHost::getPos() { return m_playHead.getPpqPosition(); }
int PluginHost::setPlaying(int p) { m_playHead.setPlaying(p); return p; }
int PluginHost::getPlaying() { return m_playHead.getPlaying(); }
int PluginHost::setRecording(int r) { m_playHead.setRecording(r != 0); return r; }
int PluginHost::getRecording() { return m_playHead.getRecording() ? 1 : 0; }
float PluginHost::setLastBarPos(float p) { m_playHead.setPpqPositionOfLastBarStart(p); return p; }
float PluginHost::getLastBarPos() { return (float)m_playHead.getPpqPositionOfLastBarStart(); }
int PluginHost::setLooping(int l) { m_playHead.setIsLooping(l != 0); return l; }
int PluginHost::getLooping() { return m_playHead.getIsLooping() ? 1 : 0; }
void PluginHost::setLoopPoints(float start, float end) { m_playHead.setLoopPoints(start, end); }
float PluginHost::setLoopStart(float s) { m_playHead.setLoopStart(s); return s; }
float PluginHost::getLoopStart() { return (float)m_playHead.getLoopStart(); }
float PluginHost::setLoopEnd(float e) { m_playHead.setLoopEnd(e); return e; }
float PluginHost::getLoopEnd() { return (float)m_playHead.getLoopEnd(); }

//-------------------------------------------------------------------------
// MIDI functions
//-------------------------------------------------------------------------
void PluginHost::noteOn(int noteNumber, float velocity, int channel)
{
    channel = std::clamp(channel, 1, 16);
    addMidiEvent(juce::MidiMessage::noteOn(channel, (juce::uint8)noteNumber, velocity));
}

void PluginHost::noteOff(int noteNumber, int channel)
{
    channel = std::clamp(channel, 1, 16);
    addMidiEvent(juce::MidiMessage::noteOff(channel, (juce::uint8)noteNumber, (juce::uint8)0));
}

void PluginHost::allNotesOff(int channel)
{
    channel = std::clamp(channel, 1, 16);
    addMidiEvent(juce::MidiMessage::allNotesOff(channel));
}

void PluginHost::pitchBend(float value, int channel)
{
    channel = std::clamp(channel, 1, 16);
    int wheelValue = (int)((value + 1.0f) * 8191.5f);
    wheelValue = std::clamp(wheelValue, 0, 16383);
    addMidiEvent(juce::MidiMessage::pitchWheel(channel, wheelValue));
}

void PluginHost::aftertouch(int noteNumber, float pressure, int channel)
{
    channel = std::clamp(channel, 1, 16);
    addMidiEvent(juce::MidiMessage::aftertouchChange(channel, (juce::uint8)noteNumber, (juce::uint8)(pressure * 127.0f)));
}

void PluginHost::aftertouchChannel(float pressure, int channel)
{
    channel = std::clamp(channel, 1, 16);
    addMidiEvent(juce::MidiMessage::channelPressureChange(channel, (juce::uint8)(pressure * 127.0f)));
}

void PluginHost::controlChange(int controlNumber, int value, int channel)
{
    channel = std::clamp(channel, 1, 16);
    addMidiEvent(juce::MidiMessage::controllerEvent(channel, controlNumber, (juce::uint8)value));
}

void PluginHost::midiMsg(int byte1, int byte2, int byte3)
{
    addMidiEvent(juce::MidiMessage(byte1, byte2, byte3));
}

void PluginHost::addMidiEvent(const juce::MidiMessage& msg)
{
    int timestamp = m_inputBuffer.getAvailableSamples();
    timestamp = std::max(0, std::min(m_blockSize - 1, timestamp));
    m_inputMidi.addEvent(msg, timestamp);
}

void PluginHost::addQWERTYMidiInput()
{
    callOnMainThread([this, context = createAsyncEventContext()]
    {
        if (m_qwertyWindow)
        {
            m_qwertyWindow->toFront(true);
            return;
        }

        ensureForegroundProcess();
        m_qwertyWindow.reset(new QWERTYMidiWindow(m_keyboardState, [this]() { m_qwertyWindow.reset(); }));
    });
}

void PluginHost::removeQWERTYMidiInput()
{
    callOnMainThread([this, context = createAsyncEventContext()]
    {
        m_qwertyWindow.reset();
    });
}

void PluginHost::toggleQWERTYMidiInput()
{
    if (m_qwertyWindow)
        removeQWERTYMidiInput();
    else
        addQWERTYMidiInput();
}

std::shared_ptr<PluginHost::AsyncEventContext> PluginHost::createAsyncEventContext()
{
    return std::make_shared<AsyncEventContext>(*this);
}

void PluginHost::ensureForegroundProcess()
{
#if JUCE_MAC
    static bool isDockIconVisible = false;
    if (!isDockIconVisible)
    {
        // Ensure the app is transformed to a foreground process (ChucK is a CLI app by default).
        // Not strictly necessary, but nice to have menu menu bar and dock icon, etc. - like ChuGl.
        isDockIconVisible = true;
    }
#endif
}

void PluginHost::callOnMainThread(std::function<void()> func)
{
    if (m_forceSynchronous)
        callOnMessageThreadSync(func);
    else
        callOnMessageThread(func);
}


//-----------------------------------------------------------------------------
// Main Thread Hook
//-----------------------------------------------------------------------------
t_CKBOOL CK_DLL_CALL pluginhost_main_hook( void * bindle )
{
    static bool juceInitialized = false;
    if(!juceInitialized)
    {
        // initialize JUCE Message Manager
        juce::MessageManager::getInstance();

#if JUCE_MAC
        // Ensure the app is transformed to a foreground process (ChucK is a CLI app by default).
        // Not strictly necessary, but nice to have menu menu bar and dock icon, etc. - like ChuGl.
        // juce::Process::setDockIconVisible(true);
#endif

        juceInitialized = true;
    }

#if JUCE_MODAL_LOOPS_PERMITTED
    // pump the message loop briefly to process events
    constexpr int ms = 1; // should really be 0, but it doesn't seem to work if it's 0...
    juce::MessageManager::getInstance()->runDispatchLoopUntil(1); 
#else
    std::static_assert<false>; // this doesn't work
    juce::MessageManager::getInstance()->runDispatchLoop();
#endif

    return TRUE;
}

t_CKBOOL CK_DLL_CALL pluginhost_main_quit( void * bindle )
{
    // clean up JUCE Message Manager
    juce::MessageManager::deleteInstance();
    return TRUE;
}


//-----------------------------------------------------------------------------
// query function: define the chugin interface
//-----------------------------------------------------------------------------
CK_DLL_QUERY( PluginHost )
{
    QUERY->setname(QUERY, "PluginHost");

    QUERY->begin_class(QUERY, "PluginHost", "UGen");
    QUERY->doc_class(QUERY, "A host for external plugins.");

    //-------------------------------------------------------------------------
    // constructor/destructor
    //-------------------------------------------------------------------------
    QUERY->add_ctor(QUERY, pluginhost_ctor);
    QUERY->add_dtor(QUERY, pluginhost_dtor);

    //-------------------------------------------------------------------------
    // tick
    //-------------------------------------------------------------------------
    QUERY->add_ugen_funcf(QUERY, pluginhost_tick, NULL, PluginHost::maxChannels, PluginHost::maxChannels);

    //-------------------------------------------------------------------------
    // parameter functions
    //-------------------------------------------------------------------------
    QUERY->add_mfun(QUERY, pluginhost_setParam, "float", "param");
    QUERY->add_arg(QUERY, "int", "index");
    QUERY->add_arg(QUERY, "float", "value");
    QUERY->doc_func(QUERY, "Set parameter value.");

    QUERY->add_mfun(QUERY, pluginhost_getParam, "float", "param");
    QUERY->add_arg(QUERY, "int", "index");
    QUERY->doc_func(QUERY, "Get parameter value.");

    QUERY->add_mfun(QUERY, pluginhost_getParamName, "string", "paramName");
    QUERY->add_arg(QUERY, "int", "index");
    QUERY->doc_func(QUERY, "Get parameter name.");

    QUERY->add_mfun(QUERY, pluginhost_getParamLabel, "string", "paramLabel");
    QUERY->add_arg(QUERY, "int", "index");
    QUERY->doc_func(QUERY, "Get parameter label.");

    QUERY->add_mfun(QUERY, pluginhost_getParamDisplay, "string", "paramDisplay");
    QUERY->add_arg(QUERY, "int", "index");
    QUERY->doc_func(QUERY, "Get parameter display value.");

    QUERY->add_mfun(QUERY, pluginhost_numParams, "int", "numParams");
    QUERY->doc_func(QUERY, "Get number of parameters.");

    QUERY->add_mfun(QUERY, pluginhost_numNonMidiParams, "int", "numNonMidiParams");
    QUERY->doc_func(QUERY, "Get number of non-MIDI parameters.");

    QUERY->add_mfun(QUERY, pluginhost_findParam, "int", "findParam");
    QUERY->add_arg(QUERY, "string", "name");
    QUERY->doc_func(QUERY, "Find parameter index by name.");

    //-------------------------------------------------------------------------
    // program functions
    //-------------------------------------------------------------------------
    QUERY->add_mfun(QUERY, pluginhost_numPrograms, "int", "numPrograms");
    QUERY->doc_func(QUERY, "Get number of programs.");

    QUERY->add_mfun(QUERY, pluginhost_program, "int", "program");
    QUERY->add_arg(QUERY, "int", "index");
    QUERY->doc_func(QUERY, "Set current program index.");

    QUERY->add_mfun(QUERY, pluginhost_getProgram, "int", "program");
    QUERY->doc_func(QUERY, "Get current program index.");

    QUERY->add_mfun(QUERY, pluginhost_programName, "string", "programName");
    QUERY->add_arg(QUERY, "int", "index");
    QUERY->doc_func(QUERY, "Get program name.");
    
    //-------------------------------------------------------------------------
    // other functions
    //-------------------------------------------------------------------------
    QUERY->add_mfun(QUERY, pluginhost_load, "void", "load");
    QUERY->add_arg(QUERY, "string", "path");
    QUERY->doc_func(QUERY, "Load a plugin from a file path.");

    QUERY->add_mfun(QUERY, pluginhost_name, "string", "name");
    QUERY->doc_func(QUERY, "Get the plugin name.");

    QUERY->add_mfun(QUERY, pluginhost_vendor, "string", "vendor");
    QUERY->doc_func(QUERY, "Get the plugin vendor/manufacturer name.");

    QUERY->add_mfun(QUERY, pluginhost_saveState, "void", "saveState");
    QUERY->add_arg(QUERY, "string", "path");
    QUERY->doc_func(QUERY, "Save plugin state to a file.");

    QUERY->add_mfun(QUERY, pluginhost_loadState, "void", "loadState");
    QUERY->add_arg(QUERY, "string", "path");
    QUERY->doc_func(QUERY, "Load plugin state from a file.");

    QUERY->add_mfun(QUERY, pluginhost_showEditor, "void", "showEditor");
    QUERY->doc_func(QUERY, "Show the plugin editor window.");

    QUERY->add_mfun(QUERY, pluginhost_hideEditor, "void", "hideEditor");
    QUERY->doc_func(QUERY, "Hide the plugin editor window.");

    QUERY->add_mfun(QUERY, pluginhost_asyncEventRunning, "int", "asyncEventRunning");
    QUERY->doc_func(QUERY, "Check if an async event is running.");

    QUERY->add_mfun(QUERY, pluginhost_waitForAsyncEvents, "void", "waitForAsyncEvents");
    QUERY->doc_func(QUERY, "Wait for all async events to finish. WARNING: This is not realtime safe and should only be used in non-realtime contexts (such as setup) or for debugging.");

    QUERY->add_mfun(QUERY, pluginhost_setForceSynchronous, "int", "forceSynchronous");
    QUERY->add_arg(QUERY, "int", "b");
    QUERY->doc_func(QUERY, "Set whether to force synchronous execution of main thread events. If true, there is no need to wait on asynchronous events, but audio processing may block.");

    QUERY->add_mfun(QUERY, pluginhost_getForceSynchronous, "int", "forceSynchronous");
    QUERY->doc_func(QUERY, "Get whether synchronous execution of main thread events is forced.");

    QUERY->add_mfun(QUERY, pluginhost_setBlockSize, "int", "blockSize");
    QUERY->add_arg(QUERY, "int", "size");
    QUERY->doc_func(QUERY, "Set the block size for plugin processing. This introduces a delay in exchange for more efficient processing.");

    QUERY->add_mfun(QUERY, pluginhost_getBlockSize, "int", "blockSize");
    QUERY->doc_func(QUERY, "Get the block size for plugin processing.");

    QUERY->add_mfun(QUERY, pluginhost_latency, "int", "latency");
    QUERY->doc_func(QUERY, "Get plugin latency in samples.");

    QUERY->add_mfun(QUERY, pluginhost_setBypass, "int", "bypass");
    QUERY->add_arg(QUERY, "int", "b");
    QUERY->doc_func(QUERY, "Set whether the plugin is bypassed.");

    QUERY->add_mfun(QUERY, pluginhost_getBypass, "int", "bypass");
    QUERY->doc_func(QUERY, "Get whether the plugin is bypassed.");

    QUERY->add_mfun(QUERY, pluginhost_reset, "void", "reset");
    QUERY->doc_func(QUERY, "Reset the plugin's internal state.");

    QUERY->add_mfun(QUERY, pluginhost_numInputs, "int", "numInputs");
    QUERY->doc_func(QUERY, "Get total number of input channels.");

    QUERY->add_mfun(QUERY, pluginhost_numOutputs, "int", "numOutputs");
    QUERY->doc_func(QUERY, "Get total number of output channels.");

    QUERY->add_mfun(QUERY, pluginhost_setRealtime, "int", "realtime");
    QUERY->add_arg(QUERY, "int", "b");
    QUERY->doc_func(QUERY, "Set whether the plugin operates in realtime mode.");

    QUERY->add_mfun(QUERY, pluginhost_getRealtime, "int", "realtime");
    QUERY->doc_func(QUERY, "Get whether the plugin operates in realtime mode.");

    //-------------------------------------------------------------------------
    // playhead functions
    //-------------------------------------------------------------------------
    QUERY->add_mfun(QUERY, pluginhost_bpm, "float", "bpm");
    QUERY->add_arg(QUERY, "float", "bpm");
    QUERY->doc_func(QUERY, "Set BPM.");

    QUERY->add_mfun(QUERY, pluginhost_getBpm, "float", "bpm");
    QUERY->doc_func(QUERY, "Get BPM.");

    QUERY->add_mfun(QUERY, pluginhost_timeSig, "void", "timeSig");
    QUERY->add_arg(QUERY, "int", "numerator");
    QUERY->add_arg(QUERY, "int", "denominator");
    QUERY->doc_func(QUERY, "Set time signature.");

    QUERY->add_mfun(QUERY, pluginhost_pos, "float", "pos");
    QUERY->add_arg(QUERY, "float", "ppq");
    QUERY->doc_func(QUERY, "Set position in PPQ.");

    QUERY->add_mfun(QUERY, pluginhost_getPos, "float", "pos");
    QUERY->doc_func(QUERY, "Get position in PPQ.");

    QUERY->add_mfun(QUERY, pluginhost_playing, "int", "playing");
    QUERY->add_arg(QUERY, "int", "playing");
    QUERY->doc_func(QUERY, "Set playing status.");

    QUERY->add_mfun(QUERY, pluginhost_getPlaying, "int", "playing");
    QUERY->doc_func(QUERY, "Get playing status.");

    QUERY->add_mfun(QUERY, pluginhost_recording, "int", "recording");
    QUERY->add_arg(QUERY, "int", "recording");
    QUERY->doc_func(QUERY, "Set recording status.");

    QUERY->add_mfun(QUERY, pluginhost_getRecording, "int", "recording");
    QUERY->doc_func(QUERY, "Get recording status.");

    QUERY->add_mfun(QUERY, pluginhost_lastBarPos, "float", "lastBarPos");
    QUERY->add_arg(QUERY, "float", "ppq");
    QUERY->doc_func(QUERY, "Set last bar position in PPQ.");

    QUERY->add_mfun(QUERY, pluginhost_getLastBarPos, "float", "lastBarPos");
    QUERY->doc_func(QUERY, "Get last bar position in PPQ.");

    QUERY->add_mfun(QUERY, pluginhost_looping, "int", "looping");
    QUERY->add_arg(QUERY, "int", "looping");
    QUERY->doc_func(QUERY, "Set looping status.");

    QUERY->add_mfun(QUERY, pluginhost_getLooping, "int", "looping");
    QUERY->doc_func(QUERY, "Get looping status.");

    QUERY->add_mfun(QUERY, pluginhost_loopPoints, "void", "loopPoints");
    QUERY->add_arg(QUERY, "float", "startPPQ");
    QUERY->add_arg(QUERY, "float", "endPPQ");
    QUERY->doc_func(QUERY, "Set loop start and end positions in PPQ.");

    QUERY->add_mfun(QUERY, pluginhost_loopStart, "float", "loopStart");
    QUERY->add_arg(QUERY, "float", "ppq");
    QUERY->doc_func(QUERY, "Set loop start position in PPQ.");

    QUERY->add_mfun(QUERY, pluginhost_getLoopStart, "float", "loopStart");
    QUERY->doc_func(QUERY, "Get loop start position in PPQ.");

    QUERY->add_mfun(QUERY, pluginhost_loopEnd, "float", "loopEnd");
    QUERY->add_arg(QUERY, "float", "ppq");
    QUERY->doc_func(QUERY, "Set loop end position in PPQ.");

    QUERY->add_mfun(QUERY, pluginhost_getLoopEnd, "float", "loopEnd");
    QUERY->doc_func(QUERY, "Get loop end position in PPQ.");

    //-------------------------------------------------------------------------
    // MIDI functions
    //-------------------------------------------------------------------------
    QUERY->add_mfun(QUERY, pluginhost_noteOn, "void", "noteOn");
    QUERY->add_arg(QUERY, "int", "note");
    QUERY->add_arg(QUERY, "float", "velocity");
    QUERY->add_arg(QUERY, "int", "channel");
    QUERY->doc_func(QUERY, "Send a MIDI Note On message. Channel is 1-16.");

    QUERY->add_mfun(QUERY, pluginhost_noteOn_default, "void", "noteOn");
    QUERY->add_arg(QUERY, "int", "note");
    QUERY->add_arg(QUERY, "float", "velocity");
    QUERY->doc_func(QUERY, "Send a MIDI Note On message on default channel 0.");

    QUERY->add_mfun(QUERY, pluginhost_noteOff, "void", "noteOff");
    QUERY->add_arg(QUERY, "int", "note");
    QUERY->add_arg(QUERY, "int", "channel");
    QUERY->doc_func(QUERY, "Send a MIDI Note Off message. Channel is 1-16.");

    QUERY->add_mfun(QUERY, pluginhost_noteOff_default, "void", "noteOff");
    QUERY->add_arg(QUERY, "int", "note");
    QUERY->doc_func(QUERY, "Send a MIDI Note Off message on default channel 0.");

    QUERY->add_mfun(QUERY, pluginhost_allNotesOff, "void", "allNotesOff");
    QUERY->add_arg(QUERY, "int", "channel");
    QUERY->doc_func(QUERY, "Send a MIDI All Notes Off message. Channel is 1-16.");

    QUERY->add_mfun(QUERY, pluginhost_allNotesOff_default, "void", "allNotesOff");
    QUERY->doc_func(QUERY, "Send a MIDI All Notes Off message on default channel 0.");

    QUERY->add_mfun(QUERY, pluginhost_pitchBend, "void", "pitchBend");
    QUERY->add_arg(QUERY, "float", "value");
    QUERY->add_arg(QUERY, "int", "channel");
    QUERY->doc_func(QUERY, "Send a MIDI Pitch Bend message (-1.0 to 1.0). Channel is 1-16.");

    QUERY->add_mfun(QUERY, pluginhost_pitchBend_default, "void", "pitchBend");
    QUERY->add_arg(QUERY, "float", "value");
    QUERY->doc_func(QUERY, "Send a MIDI Pitch Bend message (-1.0 to 1.0) on default channel 0.");

    QUERY->add_mfun(QUERY, pluginhost_aftertouch, "void", "aftertouch");
    QUERY->add_arg(QUERY, "int", "note");
    QUERY->add_arg(QUERY, "float", "pressure");
    QUERY->add_arg(QUERY, "int", "channel");
    QUERY->doc_func(QUERY, "Send a MIDI Polyphonic Aftertouch message. Channel is 1-16.");

    QUERY->add_mfun(QUERY, pluginhost_aftertouch_default, "void", "aftertouch");
    QUERY->add_arg(QUERY, "int", "note");
    QUERY->add_arg(QUERY, "float", "pressure");
    QUERY->doc_func(QUERY, "Send a MIDI Polyphonic Aftertouch message on default channel 0.");

    QUERY->add_mfun(QUERY, pluginhost_aftertouchChannel, "void", "aftertouchChannel");
    QUERY->add_arg(QUERY, "float", "pressure");
    QUERY->add_arg(QUERY, "int", "channel");
    QUERY->doc_func(QUERY, "Send a MIDI Channel Aftertouch message. Channel is 1-16.");

    QUERY->add_mfun(QUERY, pluginhost_aftertouchChannel_default, "void", "aftertouchChannel");
    QUERY->add_arg(QUERY, "float", "pressure");
    QUERY->doc_func(QUERY, "Send a MIDI Channel Aftertouch message on default channel 0.");

    QUERY->add_mfun(QUERY, pluginhost_controlChange, "void", "controlChange");
    QUERY->add_arg(QUERY, "int", "control");
    QUERY->add_arg(QUERY, "int", "value");
    QUERY->add_arg(QUERY, "int", "channel");
    QUERY->doc_func(QUERY, "Send a MIDI Control Change message. Channel is 1-16.");

    QUERY->add_mfun(QUERY, pluginhost_controlChange_default, "void", "controlChange");
    QUERY->add_arg(QUERY, "int", "control");
    QUERY->add_arg(QUERY, "int", "value");
    QUERY->doc_func(QUERY, "Send a MIDI Control Change message on default channel 0.");

    QUERY->add_mfun(QUERY, pluginhost_midiMsg, "void", "midiMsg");
    QUERY->add_arg(QUERY, "int", "byte1");
    QUERY->add_arg(QUERY, "int", "byte2");
    QUERY->add_arg(QUERY, "int", "byte3");
    QUERY->doc_func(QUERY, "Send a raw 3-byte MIDI message.");

    QUERY->add_mfun(QUERY, pluginhost_addQWERTYMidiInput, "void", "addQWERTYMidiInput");
    QUERY->doc_func(QUERY, "Add a QWERTY MIDI input window to route computer keyboard input to the plugin.");

    QUERY->add_mfun(QUERY, pluginhost_removeQWERTYMidiInput, "void", "removeQWERTYMidiInput");
    QUERY->doc_func(QUERY, "Remove the QWERTY MIDI input window.");

    QUERY->add_mfun(QUERY, pluginhost_toggleQWERTYMidiInput, "void", "toggleQWERTYMidiInput");
    QUERY->doc_func(QUERY, "Toggle the QWERTY MIDI input window.");

    //-------------------------------------------------------------------------
    // data offset
    //-------------------------------------------------------------------------
    // reserve a variable for internal class pointer
    pluginhost_data_offset = QUERY->add_mvar(QUERY, "int", "@ph_data", false);

    QUERY->end_class(QUERY);

    // register main thread hook
    Chuck_DL_MainThreadHook * hook = QUERY->create_main_thread_hook( QUERY, pluginhost_main_hook, pluginhost_main_quit, NULL );
    // activate
    if( hook ) hook->activate( hook );

    return TRUE;
}


//-----------------------------------------------------------------------------
// constructor/destructor
//-----------------------------------------------------------------------------
CK_DLL_CTOR(pluginhost_ctor)
{
    OBJ_MEMBER_INT(SELF, pluginhost_data_offset) = 0;
    PluginHost * ph_obj = new PluginHost(API->vm->srate(VM));
    OBJ_MEMBER_INT(SELF, pluginhost_data_offset) = (t_CKINT) ph_obj;
}

CK_DLL_DTOR(pluginhost_dtor)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    if( ph_obj )
    {
        delete ph_obj;
        OBJ_MEMBER_INT(SELF, pluginhost_data_offset) = 0;
    }
}

//-----------------------------------------------------------------------------
// tick function
//-----------------------------------------------------------------------------
CK_DLL_TICKF(pluginhost_tick)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    if( ph_obj ) ph_obj->tick(in, out, nframes);
    return TRUE;
}

//-----------------------------------------------------------------------------
// parameter functions
//-----------------------------------------------------------------------------
CK_DLL_MFUN(pluginhost_setParam)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT index = GET_NEXT_INT(ARGS);
    t_CKFLOAT val = GET_NEXT_FLOAT(ARGS);
    RETURN->v_float = ph_obj->setParam(index, (float)val);
}

CK_DLL_MFUN(pluginhost_getParam)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT index = GET_NEXT_INT(ARGS);
    RETURN->v_float = ph_obj->getParam(index);
}

CK_DLL_MFUN(pluginhost_getParamName)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT index = GET_NEXT_INT(ARGS);
    RETURN->v_string = (Chuck_String *)API->object->create_string(VM, ph_obj->getParamName(index).c_str(), false);
}

CK_DLL_MFUN(pluginhost_getParamLabel)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT index = GET_NEXT_INT(ARGS);
    RETURN->v_string = (Chuck_String *)API->object->create_string(VM, ph_obj->getParamLabel(index).c_str(), false);
}

CK_DLL_MFUN(pluginhost_getParamDisplay)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT index = GET_NEXT_INT(ARGS);
    RETURN->v_string = (Chuck_String *)API->object->create_string(VM, ph_obj->getParamDisplay(index).c_str(), false);
}

CK_DLL_MFUN(pluginhost_numParams)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj->getNumParams();
}

CK_DLL_MFUN(pluginhost_numNonMidiParams)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj->getNumNonMidiParams();
}

CK_DLL_MFUN(pluginhost_findParam)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    std::string name = GET_NEXT_STRING_SAFE(ARGS);
    RETURN->v_int = ph_obj->findParam(name);
}

//-----------------------------------------------------------------------------
// program functions
//-----------------------------------------------------------------------------
CK_DLL_MFUN(pluginhost_numPrograms)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj->getNumPrograms();
}

CK_DLL_MFUN(pluginhost_program)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT index = GET_NEXT_INT(ARGS);
    if( ph_obj ) ph_obj->setCurrentProgram(index);
    RETURN->v_int = index;
}

CK_DLL_MFUN(pluginhost_getProgram)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj->getCurrentProgram();
}

CK_DLL_MFUN(pluginhost_programName)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT index = GET_NEXT_INT(ARGS);
    RETURN->v_string = (Chuck_String *)API->object->create_string(VM, ph_obj->getProgramName(index).c_str(), false);
}

//-----------------------------------------------------------------------------
// other functions
//-----------------------------------------------------------------------------
CK_DLL_MFUN(pluginhost_load)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    std::string path = GET_NEXT_STRING_SAFE(ARGS);
    ph_obj->loadPlugin(path);
}

CK_DLL_MFUN(pluginhost_name)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_string = (Chuck_String *)API->object->create_string(VM, ph_obj->getName().c_str(), false);
}

CK_DLL_MFUN(pluginhost_vendor)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_string = (Chuck_String *)API->object->create_string(VM, ph_obj->getVendor().c_str(), false);
}

CK_DLL_MFUN(pluginhost_saveState)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    std::string path = GET_NEXT_STRING_SAFE(ARGS);
    ph_obj->saveState(path);
}

CK_DLL_MFUN(pluginhost_loadState)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    std::string path = GET_NEXT_STRING_SAFE(ARGS);
    ph_obj->loadState(path);
}

CK_DLL_MFUN(pluginhost_showEditor)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    ph_obj->showEditor();
}

CK_DLL_MFUN(pluginhost_hideEditor)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    ph_obj->hideEditor();
}

CK_DLL_MFUN(pluginhost_asyncEventRunning)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj->asyncEventRunning();
}

CK_DLL_MFUN(pluginhost_waitForAsyncEvents)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    ph_obj->waitForAsyncEvents();
}

CK_DLL_MFUN(pluginhost_setForceSynchronous)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT b = GET_NEXT_INT(ARGS);
    ph_obj->setForceSynchronous(b != 0);
    RETURN->v_int = b;
}

CK_DLL_MFUN(pluginhost_getForceSynchronous)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj->getForceSynchronous();
}

CK_DLL_MFUN(pluginhost_setBlockSize)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT size = GET_NEXT_INT(ARGS);
    ph_obj->setBlockSize(size);
    RETURN->v_int = size;
}

CK_DLL_MFUN(pluginhost_getBlockSize)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj->getBlockSize();
}

CK_DLL_MFUN(pluginhost_latency)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj ? ph_obj->getLatency() : 0;
}

CK_DLL_MFUN(pluginhost_setBypass)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT b = GET_NEXT_INT(ARGS);
    if( ph_obj ) ph_obj->setBypass(b != 0);
    RETURN->v_int = b;
}

CK_DLL_MFUN(pluginhost_getBypass)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj ? ph_obj->getBypass() : 0;
}

CK_DLL_MFUN(pluginhost_reset)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    if( ph_obj ) ph_obj->reset();
}

CK_DLL_MFUN(pluginhost_numInputs)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj ? ph_obj->getNumInputs() : 0;
}

CK_DLL_MFUN(pluginhost_numOutputs)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj ? ph_obj->getNumOutputs() : 0;
}

CK_DLL_MFUN(pluginhost_setRealtime)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT b = GET_NEXT_INT(ARGS);
    if( ph_obj ) ph_obj->setRealtime(b != 0);
    RETURN->v_int = b;
}

CK_DLL_MFUN(pluginhost_getRealtime)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj ? ph_obj->isRealtime() : 1;
}

//-----------------------------------------------------------------------------
// playhead functions
//-----------------------------------------------------------------------------
CK_DLL_MFUN(pluginhost_bpm)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_float = ph_obj->setBpm((float)GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(pluginhost_getBpm)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_float = ph_obj->getBpm();
}

CK_DLL_MFUN(pluginhost_timeSig)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT num = GET_NEXT_INT(ARGS);
    t_CKINT den = GET_NEXT_INT(ARGS);
    ph_obj->setTimeSig(num, den);
}

CK_DLL_MFUN(pluginhost_pos)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_float = ph_obj->setPos((float)GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(pluginhost_getPos)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_float = ph_obj->getPos();
}

CK_DLL_MFUN(pluginhost_playing)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj->setPlaying(GET_NEXT_INT(ARGS));
}

CK_DLL_MFUN(pluginhost_getPlaying)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj->getPlaying();
}

CK_DLL_MFUN(pluginhost_recording)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj->setRecording(GET_NEXT_INT(ARGS));
}

CK_DLL_MFUN(pluginhost_getRecording)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj->getRecording();
}

CK_DLL_MFUN(pluginhost_lastBarPos)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_float = ph_obj->setLastBarPos((float)GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(pluginhost_getLastBarPos)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_float = ph_obj->getLastBarPos();
}

CK_DLL_MFUN(pluginhost_looping)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj->setLooping(GET_NEXT_INT(ARGS));
}

CK_DLL_MFUN(pluginhost_getLooping)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_int = ph_obj->getLooping();
}

CK_DLL_MFUN(pluginhost_loopPoints)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKFLOAT start = GET_NEXT_FLOAT(ARGS);
    t_CKFLOAT end = GET_NEXT_FLOAT(ARGS);
    ph_obj->setLoopPoints((float)start, (float)end);
}

CK_DLL_MFUN(pluginhost_loopStart)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_float = ph_obj->setLoopStart((float)GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(pluginhost_getLoopStart)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_float = ph_obj->getLoopStart();
}

CK_DLL_MFUN(pluginhost_loopEnd)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_float = ph_obj->setLoopEnd((float)GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(pluginhost_getLoopEnd)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_float = ph_obj->getLoopEnd();
}

//-----------------------------------------------------------------------------
// MIDI functions
//-----------------------------------------------------------------------------
CK_DLL_MFUN(pluginhost_noteOn)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT note = GET_NEXT_INT(ARGS);
    t_CKFLOAT vel = GET_NEXT_FLOAT(ARGS);
    t_CKINT chan = GET_NEXT_INT(ARGS);
    if( ph_obj ) ph_obj->noteOn(note, (float)vel, chan);
}

CK_DLL_MFUN(pluginhost_noteOn_default)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT note = GET_NEXT_INT(ARGS);
    t_CKFLOAT vel = GET_NEXT_FLOAT(ARGS);
    if( ph_obj ) ph_obj->noteOn(note, (float)vel, 1);
}

CK_DLL_MFUN(pluginhost_noteOff)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT note = GET_NEXT_INT(ARGS);
    t_CKINT chan = GET_NEXT_INT(ARGS);
    if( ph_obj ) ph_obj->noteOff(note, chan);
}

CK_DLL_MFUN(pluginhost_noteOff_default)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT note = GET_NEXT_INT(ARGS);
    if( ph_obj ) ph_obj->noteOff(note, 1);
}

CK_DLL_MFUN(pluginhost_allNotesOff)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT chan = GET_NEXT_INT(ARGS);
    if( ph_obj ) ph_obj->allNotesOff(chan);
}

CK_DLL_MFUN(pluginhost_allNotesOff_default)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    if( ph_obj ) ph_obj->allNotesOff(1);
}

CK_DLL_MFUN(pluginhost_controlChange)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT ctrl = GET_NEXT_INT(ARGS);
    t_CKINT val = GET_NEXT_INT(ARGS);
    t_CKINT chan = GET_NEXT_INT(ARGS);
    if( ph_obj ) ph_obj->controlChange(ctrl, val, chan);
}

CK_DLL_MFUN(pluginhost_controlChange_default)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT ctrl = GET_NEXT_INT(ARGS);
    t_CKINT val = GET_NEXT_INT(ARGS);
    if( ph_obj ) ph_obj->controlChange(ctrl, val, 1);
}

CK_DLL_MFUN(pluginhost_midiMsg)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT b1 = GET_NEXT_INT(ARGS);
    t_CKINT b2 = GET_NEXT_INT(ARGS);
    t_CKINT b3 = GET_NEXT_INT(ARGS);
    if( ph_obj ) ph_obj->midiMsg(b1, b2, b3);
}

CK_DLL_MFUN(pluginhost_pitchBend)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKFLOAT val = GET_NEXT_FLOAT(ARGS);
    t_CKINT chan = GET_NEXT_INT(ARGS);
    if( ph_obj ) ph_obj->pitchBend((float)val, chan);
}

CK_DLL_MFUN(pluginhost_pitchBend_default)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKFLOAT val = GET_NEXT_FLOAT(ARGS);
    if( ph_obj ) ph_obj->pitchBend((float)val, 1);
}

CK_DLL_MFUN(pluginhost_aftertouch)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT note = GET_NEXT_INT(ARGS);
    t_CKFLOAT pressure = GET_NEXT_FLOAT(ARGS);
    t_CKINT chan = GET_NEXT_INT(ARGS);
    if( ph_obj ) ph_obj->aftertouch(note, (float)pressure, chan);
}

CK_DLL_MFUN(pluginhost_aftertouch_default)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKINT note = GET_NEXT_INT(ARGS);
    t_CKFLOAT pressure = GET_NEXT_FLOAT(ARGS);
    if( ph_obj ) ph_obj->aftertouch(note, (float)pressure, 1);
}

CK_DLL_MFUN(pluginhost_aftertouchChannel)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKFLOAT pressure = GET_NEXT_FLOAT(ARGS);
    t_CKINT chan = GET_NEXT_INT(ARGS);
    if( ph_obj ) ph_obj->aftertouchChannel((float)pressure, chan);
}

CK_DLL_MFUN(pluginhost_aftertouchChannel_default)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    t_CKFLOAT pressure = GET_NEXT_FLOAT(ARGS);
    if( ph_obj ) ph_obj->aftertouchChannel((float)pressure, 1);
}

CK_DLL_MFUN(pluginhost_addQWERTYMidiInput)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    if( ph_obj ) ph_obj->addQWERTYMidiInput();
}

CK_DLL_MFUN(pluginhost_removeQWERTYMidiInput)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    if( ph_obj ) ph_obj->removeQWERTYMidiInput();
}

CK_DLL_MFUN(pluginhost_toggleQWERTYMidiInput)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    if( ph_obj ) ph_obj->toggleQWERTYMidiInput();
}
