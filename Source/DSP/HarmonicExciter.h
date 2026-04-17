#pragma once
#include "EnvelopeFollower.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class HarmonicExciter
{
public:
    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& amount);

private:
    // Highpass filter to isolate content for excitation
    juce::dsp::IIR::Filter<float> hpFilters[2];
    EnvelopeFollower envelopeFollowers[2];

    juce::dsp::Oversampling<float> oversampling{2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR};

    double sr = 44100.0;

    // Asymmetric cubic soft clip for harmonic generation
    static float exciterWaveshape(float x);
};
