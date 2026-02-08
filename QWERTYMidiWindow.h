#pragma once

#include <JuceHeader.h>

//-----------------------------------------------------------------------------
// QWERTY MIDI Window
//-----------------------------------------------------------------------------
class QWERTYMidiWindow : public juce::DocumentWindow
{
public:

    QWERTYMidiWindow(juce::MidiKeyboardState& state, std::function<void()> onClosed)
        : juce::DocumentWindow("QWERTY MIDI Input", juce::Colours::darkgrey, juce::DocumentWindow::closeButton),
          m_onClosed(onClosed)
    {
        auto* keyboard = new juce::MidiKeyboardComponent(state, juce::MidiKeyboardComponent::horizontalKeyboard);
        setContentOwned(keyboard, true);
        setUsingNativeTitleBar(true);
        centreWithSize(400, 100);
        setVisible(true);
        setAlwaysOnTop(true);
        keyboard->grabKeyboardFocus();
    }

    void closeButtonPressed() override
    {
        if (m_onClosed) m_onClosed();
    }

private:

    std::function<void()> m_onClosed;
};
