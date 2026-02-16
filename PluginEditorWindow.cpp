#include "PluginEditorWindow.h"
#include "PluginHost.h"

//-----------------------------------------------------------------------------
// PluginEditorWindow implementation
//-----------------------------------------------------------------------------

PluginEditorWindow::PluginEditorWindow(PluginHost& host, juce::AudioProcessorEditor* editorToShow)
    : juce::DocumentWindow("Plugin Editor", juce::Colours::darkgrey, juce::DocumentWindow::closeButton),
      host(host), editor(editorToShow)
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

PluginEditorWindow::~PluginEditorWindow()
{
#if JUCE_MAC
    if (juce::MenuBarModel::getMacMainMenu() == this)
        juce::MenuBarModel::setMacMainMenu(nullptr);
#else
    setMenuBar(nullptr);
#endif
}

void PluginEditorWindow::activeWindowStatusChanged()
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

void PluginEditorWindow::closeButtonPressed()
{
    if (onClose)
        onClose();
}

// --- MenuBarModel Overrides ---

juce::StringArray PluginEditorWindow::getMenuBarNames()
{
    return { getName() };
}

juce::PopupMenu PluginEditorWindow::getMenuForIndex(int menuIndex, const juce::String& /*menuName*/)
{
    juce::PopupMenu menu;

    if (menuIndex == 0) // Plugin Name
    {
        menu.addItem(1, "Save State...");
        menu.addItem(2, "Load State...");
        menu.addItem(3, "QWERTY Keyboard");
    }

    return menu;
}

void PluginEditorWindow::menuItemSelected(int menuID, int /*topLevelMenuIndex*/)
{
    switch (menuID)
    {
        case 1: // Save State
        {
            fileChooser.reset(new juce::FileChooser("Save Plugin State", juce::File::getCurrentWorkingDirectory(), "*.bin"));
            auto folderChooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;
            fileChooser->launchAsync (folderChooserFlags, [this] (const juce::FileChooser& chooser)
            {
                auto file = chooser.getResult();
                if (file != juce::File{})
                    host.saveState(file.getFullPathName().toStdString());
            });
            break;
        }
        case 2: // Load State
        {
            fileChooser.reset(new juce::FileChooser("Load Plugin State", juce::File::getCurrentWorkingDirectory(), "*.bin"));
            auto folderChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
            fileChooser->launchAsync (folderChooserFlags, [this] (const juce::FileChooser& chooser)
            {
                auto file = chooser.getResult();
                if (file != juce::File{})
                    host.loadState(file.getFullPathName().toStdString());
            });
            break;
        }
        case 3: host.addQWERTYMidiInput(); break;
        default: break;
    }
}