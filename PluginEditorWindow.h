#pragma once

#include <JuceHeader.h>


//-----------------------------------------------------------------------------


// PluginEditorWindow


//-----------------------------------------------------------------------------


class PluginEditorWindow : public juce::DocumentWindow, public juce::MenuBarModel
{
public:

    PluginEditorWindow(juce::AudioProcessorEditor* editorToShow)
        : juce::DocumentWindow("Plugin Editor", juce::Colours::darkgrey, juce::DocumentWindow::closeButton),
          editor(editorToShow)
    {
        jassert(editorToShow);

        setUsingNativeTitleBar(true);
        setContentOwned(editorToShow, true);
        centreWithSize(editorToShow->getWidth(), editorToShow->getHeight());

        const auto pluginName = editorToShow->getAudioProcessor()->getName();
        setName(pluginName);

        // OS Menu Bar setup
#if JUCE_MAC
        juce::MenuBarModel::setMacMainMenu(this);
#else
        setMenuBar(this);
#endif

        setVisible(true);
    }

    ~PluginEditorWindow() override
    {
#if JUCE_MAC
        if (juce::MenuBarModel::getMacMainMenu() == this)
            juce::MenuBarModel::setMacMainMenu(nullptr);
#else
        setMenuBar(nullptr);
#endif
    }

    void activeWindowStatusChanged() override
    {
        juce::DocumentWindow::activeWindowStatusChanged();
#if JUCE_MAC
        if (isActiveWindow())
        {
            // Force a full refresh by unsetting and resetting
            juce::MenuBarModel::setMacMainMenu(nullptr);
            juce::MenuBarModel::setMacMainMenu(this);
            menuItemsChanged();
        }
#endif
    }

    void closeButtonPressed() override
    {
        if (onClose)
            onClose();
    }

    // --- MenuBarModel Overrides ---

    juce::StringArray getMenuBarNames() override
    {
        return { getName() };
    }

    juce::PopupMenu getMenuForIndex (int menuIndex, const juce::String& /*menuName*/) override
    {
        juce::PopupMenu menu;

        if (menuIndex == 0) // Plugin Name
        {
            menu.addItem (1, "Save State...");
            menu.addItem (2, "Load State...");
            menu.addItem (3, "QWERTY Keyboard");
        }

        return menu;
    }

    void menuItemSelected (int menuID, int /*topLevelMenuIndex*/) override
    {
        // Functional logic will be implemented here later
        switch (menuID)
        {
            case 1: /* Save State */ break;
            case 2: /* Load State */ break;
            case 3: /* QWERTY Keyboard */ break;
            default: break;
        }
    }

    juce::AudioProcessorEditor* editor;
    std::function<void()> onClose;
};
