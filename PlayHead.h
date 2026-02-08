#pragma once

#include <JuceHeader.h>

//-----------------------------------------------------------------------------
// PlayHead
//-----------------------------------------------------------------------------
class PlayHead : public juce::AudioPlayHead
{
public:

    juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
    {
        juce::AudioPlayHead::PositionInfo info;
        info.setBpm(bpm.load(std::memory_order_relaxed));
        
        juce::AudioPlayHead::TimeSignature ts;
        ts.numerator = numerator.load(std::memory_order_relaxed);
        ts.denominator = denominator.load(std::memory_order_relaxed);
        info.setTimeSignature(ts);
        
        info.setIsPlaying(playing.load(std::memory_order_relaxed));
        info.setIsRecording(recording.load(std::memory_order_relaxed));
        info.setPpqPosition(ppqPosition.load(std::memory_order_relaxed));
        info.setPpqPositionOfLastBarStart(ppqPositionOfLastBarStart.load(std::memory_order_relaxed));
        info.setTimeInSeconds(timeInSeconds.load(std::memory_order_relaxed));
        info.setTimeInSamples(timeInSamples.load(std::memory_order_relaxed));
        info.setIsLooping(looping.load(std::memory_order_relaxed));
        
        juce::AudioPlayHead::LoopPoints lp;
        lp.ppqStart = loopStart.load(std::memory_order_relaxed);
        lp.ppqEnd = loopEnd.load(std::memory_order_relaxed);
        info.setLoopPoints(lp);
        
        return info;
    }

    void setBpm(double b) { bpm.store(b, std::memory_order_relaxed); }
    void setTimeSignature(int n, int d)
    {
        numerator.store(n, std::memory_order_relaxed); 
        denominator.store(d, std::memory_order_relaxed); 
    }
    void setPpqPosition(double p) { ppqPosition.store(p, std::memory_order_relaxed); }
    void setPlaying(bool p) { playing.store(p, std::memory_order_relaxed); }
    void setRecording(bool r) { recording.store(r, std::memory_order_relaxed); }
    void setTimeInSeconds(double t) { timeInSeconds.store(t, std::memory_order_relaxed); }
    void setTimeInSamples(juce::int64 s) { timeInSamples.store(s, std::memory_order_relaxed); }
    void setPpqPositionOfLastBarStart(double p) { ppqPositionOfLastBarStart.store(p, std::memory_order_relaxed); }
    void setIsLooping(bool l) { looping.store(l, std::memory_order_relaxed); }
    void setLoopPoints(double start, double end)
    {
        loopStart.store(start, std::memory_order_relaxed);
        loopEnd.store(end, std::memory_order_relaxed);
    }
    void setLoopStart(double s) { loopStart.store(s, std::memory_order_relaxed); }
    void setLoopEnd(double e) { loopEnd.store(e, std::memory_order_relaxed); }

    double getBpm() const { return bpm.load(std::memory_order_relaxed); }
    double getPpqPosition() const { return ppqPosition.load(std::memory_order_relaxed); }
    bool getPlaying() const { return playing.load(std::memory_order_relaxed); }
    bool getRecording() const { return recording.load(std::memory_order_relaxed); }
    double getPpqPositionOfLastBarStart() const { return ppqPositionOfLastBarStart.load(std::memory_order_relaxed); }
    juce::int64 getTimeInSamples() const { return timeInSamples.load(std::memory_order_relaxed); }
    double getTimeInSeconds() const { return timeInSeconds.load(std::memory_order_relaxed); }
    bool getIsLooping() const { return looping.load(std::memory_order_relaxed); }
    double getLoopStart() const { return loopStart.load(std::memory_order_relaxed); }
    double getLoopEnd() const { return loopEnd.load(std::memory_order_relaxed); }

private:

    std::atomic<double> bpm { 120.0 };
    std::atomic<int> numerator { 4 };
    std::atomic<int> denominator { 4 };
    std::atomic<bool> playing { false };
    std::atomic<bool> recording { false };
    std::atomic<double> ppqPosition { 0.0 };
    std::atomic<double> ppqPositionOfLastBarStart { 0.0 };
    std::atomic<double> timeInSeconds { 0.0 };
    std::atomic<juce::int64> timeInSamples { 0 };
    std::atomic<bool> looping { false };
    std::atomic<double> loopStart { 0.0 };
    std::atomic<double> loopEnd { 0.0 };
};
