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
        setUsingNativeTitleBar(true);
        setContentOwned(editorToShow, true);
        centreWithSize(editorToShow->getWidth(), editorToShow->getHeight());

        // OS Menu Bar setup
#if JUCE_MAC
        setMacMainMenu(this);
#else
        setMenuBar(this);
#endif

        setVisible(true);
    }

    ~PluginEditorWindow() override
    {
#if JUCE_MAC
        setMacMainMenu(nullptr);
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
        return { "File", "Options" };
    }

    juce::PopupMenu getMenuForIndex (int menuIndex, const juce::String& /*menuName*/) override
    {
        juce::PopupMenu menu;

        if (menuIndex == 0) // File
        {
            menu.addItem (1, "Save State...");
            menu.addItem (2, "Load State...");
        }
        else if (menuIndex == 1) // Options
        {
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
