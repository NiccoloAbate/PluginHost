#pragma once

#include <JuceHeader.h>


// bad practice, but is convenient and can be ok
inline void callOnMessageThreadSync(std::function<void()> func)
{
    jassert(func);
    
    if (juce::MessageManager::existsAndIsCurrentThread())
    {
        func();
        return;
    }

    juce::WaitableEvent event;
    juce::MessageManager::callAsync([func, &event]()
    {
        func();
        event.signal();
    });
    event.wait();
}

inline void callOnMessageThread(std::function<void()> func)
{
    jassert(func);

    if (juce::MessageManager::existsAndIsCurrentThread())
        func();
    else
        juce::MessageManager::callAsync(func);
}
