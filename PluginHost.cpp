/*-----------------------------------------------------------------------------
 PluginHost.cpp
 -----------------------------------------------------------------------------*/

#include "chugin.h"
#include "JuceHeader.h"

#include <stdio.h>
#include <limits.h>
#include <math.h>

#include "PluginEditorWindow.h"
#include "CircularBuffer.h"

// constructor/destructor
CK_DLL_CTOR(pluginhost_ctor);
CK_DLL_DTOR(pluginhost_dtor);

// example functions
CK_DLL_MFUN(pluginhost_setParam);
CK_DLL_MFUN(pluginhost_getParam);
CK_DLL_MFUN(pluginhost_load);
CK_DLL_MFUN(pluginhost_saveState);
CK_DLL_MFUN(pluginhost_loadState);
CK_DLL_MFUN(pluginhost_asyncEventRunning);
CK_DLL_MFUN(pluginhost_waitForAsyncEvents);
CK_DLL_MFUN(pluginhost_setForceSynchronous);
CK_DLL_MFUN(pluginhost_getForceSynchronous);
CK_DLL_MFUN(pluginhost_setBlockSize);
CK_DLL_MFUN(pluginhost_getBlockSize);

// tick function
CK_DLL_TICKF(pluginhost_tick);

// data offset for internal class
t_CKINT pluginhost_data_offset = 0;


// probably shouldn't be used, but is convenient and maybe ok
void callOnMessageThreadSync(std::function<void()> func)
{
    jassert(func);
    
    juce::WaitableEvent event;
    juce::MessageManager::callAsync([func, &event]()
    {
        func();
        event.signal();
    });
    event.wait();
}

void callOnMessageThread(std::function<void()> func)
{
    jassert(func);

    if (juce::MessageManager::existsAndIsCurrentThread())
        func();
    else
        juce::MessageManager::callAsync(func);
}


//-----------------------------------------------------------------------------
// PluginHost class definition
//-----------------------------------------------------------------------------
class PluginHost
{
public:

    PluginHost( t_CKFLOAT fs ) 
    : m_renderBuffer(2, 16),
      m_inputBuffer(2, maxBufferSize * 2),
      m_outputBuffer(2, maxBufferSize * 2)
    {
        m_srate = fs;
        // default block size
        m_blockSize = 16;
        // resize render buffer to match default block size
        m_renderBuffer.setSize(2, m_blockSize);
        m_renderBuffer.clear();
        
        // Register plugin formats
        m_formatManager.addDefaultFormats();

        // Verify the Message Loop is spinning
        juce::MessageManager::callAsync([context = createAsyncEventContext()]()
        {
            std::cout << "Message loop is spinning (callAsync)!\n";
        });

        m_param = 0.0;
    }
    
    ~PluginHost()
    {
        // Delete the plugin instance on the message thread
        if (m_plugin)
        {

            // Destroy the plugin on the main thread
            std::shared_ptr<juce::AudioPluginInstance> plugin = std::move(m_plugin);
            juce::MessageManager::callAsync([plugin]()
            {
                // sharedPlugin will go out of scope here and delete the object
                std::cout << "PluginHost: Plugin destroyed on message thread." << std::endl;
            });
        }
    }

    void tick( SAMPLE * in, SAMPLE * out, int nframes )
    {
        constexpr int numChannels = maxChannels;

        // fine when there is no contention
        juce::SpinLock::ScopedLockType lock(m_audioLock);

        if (nframes == m_blockSize)
        {
            // just wrap the channels in a juce buffer and pass it to the plugin

        }

        for(int f = 0; f < nframes; f++)
        {
            float inputs[2];
            for(int c = 0; c < numChannels; c++)
                inputs[c] = in[f * numChannels + c];
            m_inputBuffer.push(inputs, numChannels);

            // check if we have enough samples to process a block
            if (m_inputBuffer.getAvailableSamples() >= m_blockSize)
            {
                if (m_inputBuffer.pop(m_renderBuffer))
                {
                    if (m_plugin)
                    {
                        m_midiBuffer.clear();

                        // check the number of channels that a plugin actually wants (some might require sidechain inputs)
                        const int totalNumChannels = std::max(m_plugin->getTotalNumInputChannels(), m_plugin->getTotalNumOutputChannels());
                        // currently we don't do anything to accomodate this, but we eventually will make sure plugins get the channels they want
                        if (totalNumChannels > maxChannels)
                            std::cout << "PluginHost: Channel mismatch, this might cause issues..." << std::endl;

                        m_plugin->processBlock(m_renderBuffer, m_midiBuffer);
                    }
                    
                    m_outputBuffer.push(m_renderBuffer);
                }
            }

            float outputs[2] = { 0.0f, 0.0f };
            m_outputBuffer.pop(outputs, numChannels);
            
            for(int c = 0; c < numChannels; c++)
                out[f * numChannels + c] = outputs[c];
        }
    }

    float setParam( t_CKFLOAT p )
    {
        m_param = p;
        return p;
    }

    float getParam()
    {
        return m_param;
    }
    
    void loadPlugin(const std::string& path)
    {
        juce::File file(path);
        if (!file.exists())
        {
            std::cout << "PluginHost: File does not exist: " << path << std::endl;
            return;
        }

        // Dispatch loading to the message thread
        callOnMainThread([this, file, context = createAsyncEventContext()]()
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
            // Use KnownPluginList to scan and add the file
            m_knownPluginList.scanAndAddFile(file.getFullPathName(), false, descriptions, *format);
            
            // print descriptions
            for (int i = 0; i < descriptions.size(); ++i)
            {
                std::cout << "PluginHost: " << i << ": " << descriptions[i]->descriptiveName << std::endl;
            }

            if (descriptions.size() == 0)
            {
                std::cout << "PluginHost: No plugin descriptions found in file." << std::endl;
                return;
            }

            std::cout << "PluginHost: Found " << descriptions.size() << " plugin descriptions. Loading the first one..." << std::endl;
            
            

            const auto callback = [this, context](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error)
                {
                    if (!instance)
                    {
                        std::cout << "PluginHost: Failed to load plugin: " << error << std::endl;
                        return;
                    }

                    m_plugin = std::move(instance);
                    std::cout << "PluginHost: Successfully loaded: " << m_plugin->getName() << std::endl;
                    
                    m_plugin->prepareToPlay(m_srate, m_blockSize);

                    // request normal stereo layout
                    juce::AudioProcessor::BusesLayout normalLayout;
                    normalLayout.inputBuses.add(juce::AudioChannelSet::stereo());
                    normalLayout.outputBuses.add(juce::AudioChannelSet::stereo());
                    
                    if (m_plugin->checkBusesLayoutSupported(normalLayout))
                        m_plugin->setBusesLayout(normalLayout);
                    else
                    {
                        // the plugin doesn't like the normal layout it is going to force some other layout
                    }

                    // probably don't want to do this immediately long term
                    showEditor();
                };

            // Create the plugin instance asynchronously
            format->createPluginInstanceAsync(*descriptions[0], m_srate, m_blockSize, callback);
        });

        // if we are forcing synchronicity, wait for the plugin to load
        if (m_forceSynchronous)
            waitForAsyncEvents();
    }

    void showEditor()
    {
        // if the editor already exists, just bring it to the front
        if (m_editor)
        {
            m_editor->toFront(true);
            return;
        }
        
        // make sure that the plugin has an editor
        if (!m_plugin->hasEditor())
            return;
        
        // create the editor
        if (auto* editor = m_plugin->createEditorIfNeeded())
        {
            // wrap the editor in a window
            auto* window = new PluginEditorWindow(editor);
            window->addToDesktop();
            window->toFront(true);
            window->onClose = [this]() { m_editor.reset(); };
            m_editor.reset(window);
        }
    }

    void saveState(const std::string& path)
    {
        callOnMainThread([this, path, context = createAsyncEventContext()]()
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

    void loadState(const std::string& path)
    {
        callOnMainThread([this, path, context = createAsyncEventContext()]()
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

    bool asyncEventRunning() const
    {
        return m_asyncEventCount > 0;
    }

    void waitForAsyncEvents() const
    {
        while (m_asyncEventCount > 0)
            std::this_thread::sleep_for(std::chrono::microseconds(1));
    }

    void setForceSynchronous(bool b)
    {
        m_forceSynchronous = b;
    }

    bool getForceSynchronous() const
    {
        return m_forceSynchronous;
    }

    void setBlockSize(int size)
    {
        if (size <= 0) return;

        callOnMainThread([this, size, context = createAsyncEventContext()]()
        {
            juce::SpinLock::ScopedLockType lock(m_audioLock);
            m_blockSize = size;
            m_renderBuffer.setSize(2, m_blockSize);

            if (m_plugin)
                m_plugin->prepareToPlay(m_srate, m_blockSize);
        });
    }

    int getBlockSize() const
    {
        return m_blockSize;
    }

    // for now used fixed number of channels and lock it to stereo
    static constexpr int maxChannels = 2;

private:

    float m_param;
    double m_srate;
    
    juce::AudioPluginFormatManager m_formatManager;
    juce::KnownPluginList m_knownPluginList;
    std::unique_ptr<juce::AudioPluginInstance> m_plugin;
    std::unique_ptr<PluginEditorWindow> m_editor;
    juce::AudioBuffer<float> m_renderBuffer;
    juce::MidiBuffer m_midiBuffer;

    // brute force synchronization - use sparingly
    juce::SpinLock m_audioLock;

    int m_blockSize = 16;
    static constexpr int maxBufferSize = 256;
    CircularBuffer m_inputBuffer;
    CircularBuffer m_outputBuffer;

    struct AsyncEventContext
    {
        AsyncEventContext(PluginHost& host) : m_host(&host) { m_host->m_asyncEventCount.fetch_add(1); }
        ~AsyncEventContext() { if (m_host) m_host->m_asyncEventCount.fetch_sub(1); }

        AsyncEventContext(const AsyncEventContext&) = delete;
        AsyncEventContext& operator=(const AsyncEventContext&) = delete;
        AsyncEventContext(AsyncEventContext&& other) = delete;
        AsyncEventContext& operator=(AsyncEventContext&& other) = delete;

        PluginHost* m_host = nullptr;
    };
    std::shared_ptr<AsyncEventContext> createAsyncEventContext() { return std::make_shared<AsyncEventContext>(*this); }

    void callOnMainThread(std::function<void()> func)
    {
        if (m_forceSynchronous)
            callOnMessageThreadSync(func);
        else
            callOnMessageThread(func);
    }

    std::atomic<int> m_asyncEventCount { 0 };

    // if true all main thread events will be force to be "synchronous" (i.e. blocking audio process until they finish)
    // this is simpler for user (since they don't have to manage waiting for asynchronous events) and nice for debugging
    // but it is fundamentally bad audio programming practice - it may result in unnecessary audio dropouts,
    // and depending on the way ChucK handles the main thread events, it may result in deadlocks
    // seems to work in practice for now though
    bool m_forceSynchronous = true;
};


//-----------------------------------------------------------------------------
// Main Thread Hook
//-----------------------------------------------------------------------------
t_CKBOOL CK_DLL_CALL pluginhost_main_hook( void * bindle )
{
    static bool juceInitialized = false;
    if(!juceInitialized)
    {
        // Initialize JUCE Message Manager
        juce::MessageManager::getInstance();
        juceInitialized = true;
        printf("JUCE MessageManager initialized on main thread.\n");
    }

    // Pump the message loop briefly to process events
    constexpr int ms = 1; // should really be 0, but it doesn't seem to work if it's 0...
    juce::MessageManager::getInstance()->runDispatchLoopUntil(1); 

    return TRUE;
}

t_CKBOOL CK_DLL_CALL pluginhost_main_quit( void * bindle )
{
    // Clean up JUCE Message Manager
    juce::MessageManager::deleteInstance();
    printf("JUCE MessageManager deleted.\n");
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

    QUERY->add_ctor(QUERY, pluginhost_ctor);
    QUERY->add_dtor(QUERY, pluginhost_dtor);

    QUERY->add_ugen_funcf(QUERY, pluginhost_tick, NULL, PluginHost::maxChannels, PluginHost::maxChannels);

    QUERY->add_mfun(QUERY, pluginhost_setParam, "float", "param");
    QUERY->add_arg(QUERY, "float", "arg");
    QUERY->doc_func(QUERY, "Example parameter setter.");

    QUERY->add_mfun(QUERY, pluginhost_getParam, "float", "param");
    QUERY->doc_func(QUERY, "Example parameter getter.");
    
    QUERY->add_mfun(QUERY, pluginhost_load, "void", "load");
    QUERY->add_arg(QUERY, "string", "path");
    QUERY->doc_func(QUERY, "Load a plugin from a file path.");

    QUERY->add_mfun(QUERY, pluginhost_saveState, "void", "saveState");
    QUERY->add_arg(QUERY, "string", "path");
    QUERY->doc_func(QUERY, "Save plugin state to a file.");

    QUERY->add_mfun(QUERY, pluginhost_loadState, "void", "loadState");
    QUERY->add_arg(QUERY, "string", "path");
    QUERY->doc_func(QUERY, "Load plugin state from a file.");

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

    // reserve a variable for internal class pointer
    pluginhost_data_offset = QUERY->add_mvar(QUERY, "int", "@ph_data", false);

    QUERY->end_class(QUERY);

    // register main thread hook
    Chuck_DL_MainThreadHook * hook = QUERY->create_main_thread_hook( QUERY, pluginhost_main_hook, pluginhost_main_quit, NULL );
    // activate
    if( hook ) hook->activate( hook );

    return TRUE;
}


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


CK_DLL_TICKF(pluginhost_tick)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    if( ph_obj ) ph_obj->tick(in, out, nframes);
    return TRUE;
}


CK_DLL_MFUN(pluginhost_setParam)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_float = ph_obj->setParam(GET_NEXT_FLOAT(ARGS));
}


CK_DLL_MFUN(pluginhost_getParam)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    RETURN->v_float = ph_obj->getParam();
}

CK_DLL_MFUN(pluginhost_load)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    std::string path = GET_NEXT_STRING_SAFE(ARGS);
    ph_obj->loadPlugin(path);
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
    ph_obj->setForceSynchronous(b);
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
