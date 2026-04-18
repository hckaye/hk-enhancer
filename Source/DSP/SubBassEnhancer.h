#pragma once
#include "EnvelopeFollower.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class SubBassEnhancer
{
public:
    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& amount);

    // Determine how many octaves to shift down to land in 20-50Hz
    static int calcOctavesDown(float fundamentalFreq);

private:
    float processChannel(float input, int channel);

    struct ChannelState
    {
        float prevFiltered = 0.0f;

        // Frequency estimation via zero-crossing interval
        int samplesSinceLastCross = 0;
        float estimatedFreq = 0.0f;

        // Adaptive multi-octave sub generation
        // Each divider stage halves the frequency
        bool dividerState[4] = {}; // up to ÷16 (4 octave-down stages)
        int dividerCount[4] = {};

        float subValue = 0.0f;
    };

    ChannelState channelStates[2];
    EnvelopeFollower envelopeFollowers[2];

    // Bandpass to isolate fundamentals for pitch tracking
    juce::dsp::IIR::Filter<float> bpFilters[2];

    // Smoothing LPF for the generated sub square wave — very low cutoff
    juce::dsp::IIR::Filter<float> subSmoothingFilters[2];

    // Soft saturation for even-harmonic warmth
    static float warmSaturate(float x);

    // Soft-knee compressor for consistent sub-bass pressure
    float compressEnvelope(float envelope, int channel);

    float compGain[2] = {1.0f, 1.0f};

    double sr = 44100.0;
    static constexpr float noiseGateThreshold = 0.002f;
    static constexpr float targetSubFreqHigh = 50.0f;
};
