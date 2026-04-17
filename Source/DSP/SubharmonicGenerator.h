#pragma once
#include "EnvelopeFollower.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class SubharmonicGenerator
{
public:
    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& amount);

private:
    float processChannel(float input, int channel);

    // Per-channel state for zero-crossing detection
    struct ChannelState
    {
        float prevSample = 0.0f;
        bool subState = false; // toggles every other zero crossing
        int zeroCrossCount = 0;
        float subHarmonicValue = 0.0f;
    };

    ChannelState channelStates[2];
    EnvelopeFollower envelopeFollowers[2];

    // Smoothing filter for the generated subharmonic (remove harsh square wave edges)
    juce::dsp::IIR::Filter<float> smoothingFilters[2];

    double sr = 44100.0;
    static constexpr float noiseGateThreshold = 0.001f; // ~-60dB
};
