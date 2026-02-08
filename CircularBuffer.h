#pragma once

#include <JuceHeader.h>

class CircularBuffer
{
public:

    CircularBuffer(int numChannels, int size)
        : buffer(numChannels, size), writeIndex(0), readIndex(0)
    {
        buffer.clear();
    }

    void setSize(int numChannels, int size)
    {
        buffer.setSize(numChannels, size);
        clear();
    }

    void clear()
    {
        buffer.clear();
        writeIndex = 0;
        readIndex = 0;
    }

    void push(float* input, int numChannels)
    {
        for (int ch = 0; ch < numChannels && ch < buffer.getNumChannels(); ++ch)
        {
            buffer.setSample(ch, writeIndex, input[ch]);
        }
        
        writeIndex = (writeIndex + 1) % buffer.getNumSamples();
    }

    // Fills the destination buffer with accumulated samples
    // Returns true if enough samples were available to fill the buffer
    bool pop(juce::AudioBuffer<float>& destination)
    {
        int samplesNeeded = destination.getNumSamples();
        int availableSamples = getAvailableSamples();

        if (availableSamples < samplesNeeded)
            return false;

        for (int ch = 0; ch < destination.getNumChannels() && ch < buffer.getNumChannels(); ++ch)
        {
            auto* dest = destination.getWritePointer(ch);
            auto* src = buffer.getReadPointer(ch);
            
            for (int i = 0; i < samplesNeeded; ++i)
            {
                int idx = (readIndex + i) % buffer.getNumSamples();
                dest[i] = src[idx];
            }
        }

        readIndex = (readIndex + samplesNeeded) % buffer.getNumSamples();
        return true;
    }
    
    // Writes samples from source buffer back into the circular buffer
    // This is used to push processed samples back
    void writeBack(const juce::AudioBuffer<float>& source)
    {
        int numSamples = source.getNumSamples();
        // We write back to where we just read from (effectively overwriting or appending? 
        // Wait, for a simple accumulation strategy we usually:
        // 1. Push single samples into input circular buffer
        // 2. When enough samples, process block
        // 3. Push processed block into output circular buffer
        // 4. Pop single sample from output circular buffer
        
        // The user asked for "a simple circular buffer which wraps a juce::AudioBuffer".
        // I'll stick to basic push/pop functionality. 
        // The logic for handling input vs output buffering will be in PluginHost.cpp
    }
    
    int getAvailableSamples() const
    {
        int size = buffer.getNumSamples();
        if (writeIndex >= readIndex)
            return writeIndex - readIndex;
        else
            return size - (readIndex - writeIndex);
    }

    int getFreeSpace() const
    {
        return buffer.getNumSamples() - 1 - getAvailableSamples();
    }
    
    // For popping single sample (output)
    void pop(float* output, int numChannels)
    {
        if (getAvailableSamples() == 0)
        {
            for(int i=0; i < numChannels; i++)
                output[i] = 0.0f;
            return;
        }

        for (int ch = 0; ch < std::min(numChannels, buffer.getNumChannels()); ++ch)
            output[ch] = buffer.getSample(ch, readIndex);
        
        readIndex = (readIndex + 1) % buffer.getNumSamples();
    }
    
    // For pushing block (output from plugin)
    void push(const juce::AudioBuffer<float>& source)
    {
        int numSamples = source.getNumSamples();
        if (getFreeSpace() < numSamples) return; // Overflow protection
        
        for (int ch = 0; ch < source.getNumChannels() && ch < buffer.getNumChannels(); ++ch)
        {
            auto* src = source.getReadPointer(ch);
            
            for (int i = 0; i < numSamples; ++i)
            {
                int idx = (writeIndex + i) % buffer.getNumSamples();
                buffer.setSample(ch, idx, src[i]);
            }
        }
        writeIndex = (writeIndex + numSamples) % buffer.getNumSamples();
    }

private:

    juce::AudioBuffer<float> buffer;
    int writeIndex;
    int readIndex;
};
