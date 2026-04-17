#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class MultibandSplitter
{
public:
    void prepare(double sampleRate, int samplesPerBlock);
    void setCrossoverFrequencies(float lowMidHz, float midHighHz);
    void process(const juce::AudioBuffer<float>& input,
                 juce::AudioBuffer<float>& low,
                 juce::AudioBuffer<float>& mid,
                 juce::AudioBuffer<float>& high);

private:
    // Crossover 1: Low/Mid boundary
    juce::dsp::LinkwitzRileyFilter<float> lp1, hp1;
    // Crossover 2: Mid/High boundary (applied to hp1 output)
    juce::dsp::LinkwitzRileyFilter<float> lp2, hp2;
    // Allpass compensation for low band (matches phase of crossover 2)
    juce::dsp::LinkwitzRileyFilter<float> ap2Lp, ap2Hp;

    juce::AudioBuffer<float> tempBuffer;
    juce::AudioBuffer<float> allpassBuffer;

    double sampleRate = 44100.0;
    float currentLowMidFreq = 200.0f;
    float currentMidHighFreq = 4000.0f;
};
