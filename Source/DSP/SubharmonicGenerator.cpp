#include "SubharmonicGenerator.h"

void SubharmonicGenerator::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    sr = sampleRate;

    for (int ch = 0; ch < 2; ++ch)
    {
        channelStates[ch] = {};
        envelopeFollowers[ch].prepare(sampleRate);
        envelopeFollowers[ch].setAttackMs(5.0f);
        envelopeFollowers[ch].setReleaseMs(15.0f);

        // Lowpass at 80Hz to smooth the subharmonic square wave into a sine-like wave
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 80.0f, 0.707f);
        smoothingFilters[ch].coefficients = coeffs;
        smoothingFilters[ch].reset();
    }
}

float SubharmonicGenerator::processChannel(float input, int channel)
{
    auto& state = channelStates[channel];
    auto& envFollower = envelopeFollowers[channel];

    float env = envFollower.process(input);

    // Noise gate: don't generate subharmonics for very quiet signals
    if (env < noiseGateThreshold)
    {
        state.prevSample = input;
        return 0.0f;
    }

    // Zero-crossing detection (positive-going)
    if (state.prevSample <= 0.0f && input > 0.0f)
    {
        state.zeroCrossCount++;
        // Toggle every other zero crossing -> half frequency
        if (state.zeroCrossCount >= 2)
        {
            state.subState = !state.subState;
            state.zeroCrossCount = 0;
        }
    }

    state.prevSample = input;

    // Generate square wave at half frequency
    float rawSub = state.subState ? 1.0f : -1.0f;

    // Smooth the square wave into a sine-like wave
    float smoothedSub = smoothingFilters[channel].processSample(rawSub);

    // Modulate by envelope of original signal
    return smoothedSub * env;
}

void SubharmonicGenerator::process(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& amount)
{
    int numSamples = buffer.getNumSamples();
    int numChannels = juce::jmin(buffer.getNumChannels(), 2);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float amt = amount.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float input = buffer.getSample(ch, sample);
            float subHarmonic = processChannel(input, ch);
            // Blend: original + subharmonic scaled by amount
            buffer.setSample(ch, sample, input + subHarmonic * amt);
        }
    }
}
