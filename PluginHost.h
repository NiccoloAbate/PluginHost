//-----------------------------------------------------------------------------
// PluginHost.h
//-----------------------------------------------------------------------------
#pragma once

#include "chugin.h"
#include <JuceHeader.h>

#include "PluginEditorWindow.h"
#include "CircularBuffer.h"
#include "PlayHead.h"
#include "QWERTYMidiWindow.h"

#include <string>
#include <memory>
#include <atomic>
#include <functional>

//-----------------------------------------------------------------------------
// PluginHost
//-----------------------------------------------------------------------------
class PluginHost
{
public:

    //-------------------------------------------------------------------------
    // constructor/destructor
    //-------------------------------------------------------------------------
    PluginHost( t_CKFLOAT fs, Chuck_VM * vm, CK_DL_API api );
    ~PluginHost();

    //-------------------------------------------------------------------------
    // tick
    //-------------------------------------------------------------------------
    void tick( SAMPLE * in, SAMPLE * out, int nframes );

    //-------------------------------------------------------------------------
    // parameter accessors
    //-------------------------------------------------------------------------
    int getNumParams();
    int getNumNonMidiParams();
    std::string getParamName(int index);
    float getParam(int index);
    float setParam(int index, float val);
    int findParam(const std::string& name);
    std::string getParamLabel(int index);
    std::string getParamDisplay(int index);

    //-------------------------------------------------------------------------
    // metadata
    //-------------------------------------------------------------------------
    std::string getName() const;
    std::string getVendor() const;

    //-------------------------------------------------------------------------
    // load / state
    //-------------------------------------------------------------------------
    void loadPlugin(const std::string& path);
    void showEditor();
    void hideEditor();
    void saveState(const std::string& path);
    void loadState(const std::string& path);

    //-------------------------------------------------------------------------
    // async / sync
    //-------------------------------------------------------------------------
    bool asyncEventRunning() const;
    void waitForAsyncEvents(int timeoutMs = -1) const;
    void setForceSynchronous(bool b);
    bool getForceSynchronous() const;

    //-------------------------------------------------------------------------
    // processing config
    //-------------------------------------------------------------------------
    void setBlockSize(int size);
    int getBlockSize() const;
    int getLatency() const;
    void setBypass(bool b);
    bool getBypass() const;
    void reset();
    int getNumInputs() const;
    int getNumOutputs() const;
    void setRealtime(bool b);
    bool isRealtime() const;

    Chuck_Event* getAsyncEvent();

    //-------------------------------------------------------------------------
    // program functions
    //-------------------------------------------------------------------------
    int getNumPrograms();
    int getCurrentProgram();
    void setCurrentProgram(int index);
    std::string getProgramName(int index);

    //-------------------------------------------------------------------------
    // playHead accessors
    //-------------------------------------------------------------------------
    float setBpm(float b);
    float getBpm();
    void setTimeSig(int n, int d);
    float setPos(float p);
    float getPos();
    int setPlaying(int p);
    int getPlaying();
    int setRecording(int r);
    int getRecording();
    float setLastBarPos(float p);
    float getLastBarPos();
    int setLooping(int l);
    int getLooping();
    void setLoopPoints(float start, float end);
    float setLoopStart(float s);
    float getLoopStart();
    float setLoopEnd(float e);
    float getLoopEnd();

    //-------------------------------------------------------------------------
    // MIDI functions
    //-------------------------------------------------------------------------
    void noteOn(int noteNumber, float velocity, int channel);
    void noteOff(int noteNumber, int channel);
    void allNotesOff(int channel);
    void pitchBend(float value, int channel);
    void aftertouch(int noteNumber, float pressure, int channel);
    void aftertouchChannel(float pressure, int channel);
    void controlChange(int controlNumber, int value, int channel);
    void midiMsg(int byte1, int byte2, int byte3);
    void addMidiEvent(const juce::MidiMessage& msg);
    void addQWERTYMidiInput();
    void removeQWERTYMidiInput();
    void toggleQWERTYMidiInput();

    // for now used fixed number of channels
    static constexpr int maxChannels = 8;

private:

    // plugin format manager
    juce::AudioPluginFormatManager m_formatManager;
    // plugin list
    juce::KnownPluginList m_knownPluginList;
    // playhead
    PlayHead m_playHead;
    // plugin instance
    std::unique_ptr<juce::AudioPluginInstance> m_plugin;
    // plugin editor window
    std::unique_ptr<PluginEditorWindow> m_editor;
    // audio render buffer
    juce::AudioBuffer<float> m_renderBuffer;
    // keyboard state
    juce::MidiKeyboardState m_keyboardState;
    // qwerty window
    std::unique_ptr<QWERTYMidiWindow> m_qwertyWindow;
    // accumulated MIDI buffer which will be used as input
    juce::MidiBuffer m_inputMidi;
    // processed MIDI buffer which will store the midi output
    juce::MidiBuffer m_outputMidi;

    // brute force synchronization - use sparingly
    // currently used for protecting critical audio processing code, such as resizing buffers
    juce::SpinLock m_audioLock;

    double m_srate;
    // Plugin block size - since chugins are generally sample by sample, samples will have to accumulate,
    // meaning a delay will be introduced. This is a tradeoff between delay and processing speed.
    // Plugins are optimized for larger block sizes, generally.
    // If the block size is equivilent to the number of frames in tick(), then the the audio
    // will be passed directly to the plugin (bypassing the delay and accumulation).
    int m_blockSize = 16;
    // maximum plugin block size
    static constexpr int maxBufferSize = 256;
    // input accumulation buffer
    CircularBuffer m_inputBuffer;
    // output buffer
    CircularBuffer m_outputBuffer;

    Chuck_VM* m_vm;
    CK_DL_API m_api;
    Chuck_Event* m_asyncEvent;

    void broadcastAsyncEvent();

    // context for tracking async events
    struct AsyncEventContext
    {
        AsyncEventContext(PluginHost& host) : m_host(&host) { m_host->m_asyncEventCount.fetch_add(1); }
        ~AsyncEventContext() 
        { 
            if (m_host) 
            {
                if (m_host->m_asyncEventCount.fetch_sub(1) == 1)
                    m_host->broadcastAsyncEvent();
            }
        }

        AsyncEventContext(const AsyncEventContext&) = delete;
        AsyncEventContext& operator=(const AsyncEventContext&) = delete;
        AsyncEventContext(AsyncEventContext&& other) = delete;
        AsyncEventContext& operator=(AsyncEventContext&& other) = delete;

        PluginHost* m_host = nullptr;
    };
    // create an async event context
    std::shared_ptr<AsyncEventContext> createAsyncEventContext();

    // ensure that the process is a foreground process (Mac only)
    void ensureForegroundProcess();

    // call a function on the main thread, either synchonously or asynchronously
    void callOnMainThread(std::function<void()> func);

    // number of currently running asynchronous events
    std::atomic<int> m_asyncEventCount { 0 };

    // If true all main thread events will be force to be "synchronous" (i.e. blocking audio process until they finish).
    // This is simpler for user (since they don't have to manage waiting for asynchronous events) and nice for debugging
    // but it is fundamentally bad audio programming practice - it may result in unnecessary audio dropouts,
    // and, depending on the way ChucK handles the main thread events, it may result in deadlocks.
    // Deadlocks don't seem to occur in practice with ChucK though, and this greatly simplified usage.
    bool m_forceSynchronous = true;
};
