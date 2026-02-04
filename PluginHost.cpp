/*-----------------------------------------------------------------------------
 PluginHost.cpp
 -----------------------------------------------------------------------------*/

#include "chugin.h"
#include "JuceHeader.h"

#include <stdio.h>
#include <limits.h>
#include <math.h>

#include "PluginEditorWindow.h"

// constructor/destructor
CK_DLL_CTOR(pluginhost_ctor);
CK_DLL_DTOR(pluginhost_dtor);

// example functions
CK_DLL_MFUN(pluginhost_setParam);
CK_DLL_MFUN(pluginhost_getParam);
CK_DLL_MFUN(pluginhost_load);

// tick function
CK_DLL_TICK(pluginhost_tick);

// data offset for internal class
t_CKINT pluginhost_data_offset = 0;


//-----------------------------------------------------------------------------
// PluginHost class definition
//-----------------------------------------------------------------------------
class PluginHost
{
public:

    PluginHost( t_CKFLOAT fs )
    {
        m_srate = fs;

        m_renderBuffer.setSize(2, 1);
        m_renderBuffer.clear();
        
        // Register plugin formats
        m_formatManager.addDefaultFormats();

        // Verify the Message Loop is spinning
        juce::MessageManager::callAsync([]()
        {
            std::cout << "Message loop is spinning (callAsync)!\n";

            // juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "PluginHost", "JUCE popup window!", "OK" );
        });

        m_param = 0.0;
    }
    
    ~PluginHost()
    {
        // Delete the plugin instance on the message thread
        if (m_plugin)
        {
            // Destroy the plugin asynchronously
            // juce::MessageManager::callAsync([plugin = std::move(m_plugin)]() mutable
            // {
            //     plugin.reset();
            //     std::cout << "PluginHost: Plugin destroyed on message thread." << std::endl;
            // });
        }
    }

    SAMPLE tick( SAMPLE in )
    {
        // Use pre-allocated buffer
        m_renderBuffer.getWritePointer(0)[0] = in;
        m_renderBuffer.getWritePointer(1)[0] = in;

        if (m_plugin)
        {
            m_midiBuffer.clear();
            m_plugin->processBlock(m_renderBuffer, m_midiBuffer);
        }

        const float out = (m_renderBuffer.getReadPointer(0)[0] + m_renderBuffer.getReadPointer(1)[0]) / 2.0f;
        return out;
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
        juce::MessageManager::callAsync([this, file]()
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

            if (descriptions.size() > 0)
            {
                std::cout << "PluginHost: Found " << descriptions.size() << " plugin descriptions. Loading the first one..." << std::endl;
                
                // TODO: Figure out block size
                constexpr int blockSize = 512;

                juce::String error;
                // Create the plugin instance asynchronously
                format->createPluginInstanceAsync(*descriptions[0], m_srate, blockSize,
                    [this](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error)
                    {
                        if (instance)
                        {
                            m_plugin = std::move(instance);
                            std::cout << "PluginHost: Successfully loaded: " << m_plugin->getName() << std::endl;

                            m_plugin->prepareToPlay(m_srate, blockSize);

                            // juce::AudioProcessor::BusesLayout normalLayout;
                            // normalLayout.inputBuses.add(juce::AudioChannelSet::mono());
                            // normalLayout.outputBuses.add(juce::AudioChannelSet::mono());
                            
                            // if (m_plugin->checkBusesLayoutSupported(normalLayout))
                            //     m_plugin->setBusesLayout(normalLayout);
                            // else
                            // {
                            //     // the plugin doesn't like the normal layout - should accomodate the defaultLayout instead then?
                            //     std::cout << "PluginHost: Default layout not supported. Using normal layout." << std::endl;
                            // }

                            showEditor();
                        }
                        else
                        {
                            std::cout << "PluginHost: Failed to load plugin: " << error << std::endl;
                        }
                    });
            }
            else
            {
                std::cout << "PluginHost: No plugin descriptions found in file." << std::endl;
            }
        });
    }

    void showEditor()
    {
        if (m_editor)
        {
            m_editor->toFront(true);
            return;
        }
        
        if (m_plugin->hasEditor())
        {
            if (auto* editor = m_plugin->createEditorIfNeeded())
            {
                auto* window = new PluginEditorWindow(editor);
                window->addToDesktop();
                window->toFront(true);
                window->onClose = [this]() { m_editor.reset(); };
                
                m_editor.reset(window);
            }
        }
    }

    static void printPluginList()
    {

    }

private:

    float m_param;
    double m_srate;
    
    juce::AudioPluginFormatManager m_formatManager;
    juce::KnownPluginList m_knownPluginList;
    std::unique_ptr<juce::AudioPluginInstance> m_plugin;
    std::unique_ptr<PluginEditorWindow> m_editor;
    juce::AudioBuffer<float> m_renderBuffer;
    juce::MidiBuffer m_midiBuffer;
};


//-----------------------------------------------------------------------------
// Main Thread Hook
//-----------------------------------------------------------------------------
t_CKBOOL CK_DLL_CALL pluginhost_main_hook( void * bindle )
{
    static bool juceInitialized = false;
    if( !juceInitialized )
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

    QUERY->add_ugen_func(QUERY, pluginhost_tick, NULL, 1, 1);

    QUERY->add_mfun(QUERY, pluginhost_setParam, "float", "param");
    QUERY->add_arg(QUERY, "float", "arg");
    QUERY->doc_func(QUERY, "Example parameter setter.");

    QUERY->add_mfun(QUERY, pluginhost_getParam, "float", "param");
    QUERY->doc_func(QUERY, "Example parameter getter.");
    
    QUERY->add_mfun(QUERY, pluginhost_load, "void", "load");
    QUERY->add_arg(QUERY, "string", "path");
    QUERY->doc_func(QUERY, "Load a plugin from a file path.");

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


CK_DLL_TICK(pluginhost_tick)
{
    PluginHost * ph_obj = (PluginHost *) OBJ_MEMBER_INT(SELF, pluginhost_data_offset);
    if( ph_obj ) *out = ph_obj->tick(in);
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
