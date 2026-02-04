#pragma once

#include <JuceHeader.h>

class PluginEditorWindow : public juce::DocumentWindow
{
public:
    
    PluginEditorWindow(juce::AudioProcessorEditor* editorToShow)
        : juce::DocumentWindow("Plugin Editor",
                               juce::Colours::darkgrey,
                               juce::DocumentWindow::closeButton), editor(editorToShow)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(editorToShow, true);
        centreWithSize(editorToShow->getWidth(), editorToShow->getHeight());
        //setResizable(true, false); // causing weird GUI / parameter crashes for a bunch of Apple AUs...
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        if (onClose)
            onClose();
    }
    
    juce::AudioProcessorEditor* editor;
    std::function<void()> onClose;
};