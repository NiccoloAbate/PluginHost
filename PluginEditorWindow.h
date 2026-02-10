#pragma once

#include "chugin.h"
#include <JuceHeader.h>

class PluginHost;

//-----------------------------------------------------------------------------
// PluginEditorWindow
//-----------------------------------------------------------------------------
class PluginEditorWindow : public juce::DocumentWindow, public juce::MenuBarModel
{
public:

    PluginEditorWindow(PluginHost& host, juce::AudioProcessorEditor* editorToShow);

    ~PluginEditorWindow() override;

    void activeWindowStatusChanged() override;

    void closeButtonPressed() override;

    // --- MenuBarModel Overrides ---

    juce::StringArray getMenuBarNames() override;

    juce::PopupMenu getMenuForIndex (int menuIndex, const juce::String& menuName) override;

    void menuItemSelected (int menuID, int topLevelMenuIndex) override;

    std::function<void()> onClose;

private:

    PluginHost& host;
    juce::AudioProcessorEditor* editor;
    std::unique_ptr<juce::FileChooser> fileChooser;
};