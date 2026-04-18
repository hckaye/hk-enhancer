#pragma once
#include "EnvelopeFollower.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class TextureEnhancer
{
public:
    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& amount);

private:
    // --- Harmonic density: gentle full-band saturation ---
    // Enhances ALL existing harmonics/partials, not just fundamentals
    static float densitySaturate(float x, float drive);

    // --- Dynamic noise texture ---
    // Pink noise generator (Voss-McCartney algorithm)
    struct PinkNoiseGen
    {
        static constexpr int kNumRows = 12;
        int rowIndex = 0;
        float rows[kNumRows] = {};
        float runningSum = 0.0f;
        unsigned int seed = 12345;

        float next();

    private:
        float whiteNoise();
        static int countTrailingZeros(int n);
    };

    PinkNoiseGen noiseGens[2];
    EnvelopeFollower envelopeFollowers[2];

    // Tilt EQ: gently shape noise texture (roll off lows, keep highs airy)
    juce::dsp::IIR::Filter<float> noiseTiltFilters[2];

    double sr = 44100.0;
};
