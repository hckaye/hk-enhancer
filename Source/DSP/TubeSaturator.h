#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class TubeSaturator
{
public:
    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& amount);

private:
    juce::dsp::Oversampling<float> oversampling{2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR};
    double sr = 44100.0;

    // Asymmetric soft clipping
    static float waveshape(float x, float drive);
};
