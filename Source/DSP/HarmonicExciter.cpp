#include "HarmonicExciter.h"

#include <cmath>

void HarmonicExciter::prepare(double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;

    for (auto& hpFilter : hpFilters)
    {
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 3000.0f, 0.707f);
        hpFilter.coefficients = coeffs;
        hpFilter.reset();
    }

    for (auto& envFollower : envelopeFollowers)
    {
        envFollower.prepare(sampleRate);
        envFollower.setAttackMs(2.0f);
        envFollower.setReleaseMs(20.0f);
    }

    harmonicsBuffer.setSize(2, samplesPerBlock);
    oversampling.initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampling.reset();
}

float HarmonicExciter::exciterWaveshape(float x)
{
    // Asymmetric cubic soft clip
    if (x >= 0.0f)
        return x - (x * x * x) / 3.0f;
    else
        return (x - (x * x * x) / 3.0f) * 0.9f;
}

void HarmonicExciter::process(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& amount)
{
    int numSamples = buffer.getNumSamples();
    int numChannels = juce::jmin(buffer.getNumChannels(), 2);

    // Ensure pre-allocated buffer is large enough
    if (harmonicsBuffer.getNumSamples() < numSamples)
        harmonicsBuffer.setSize(2, numSamples, false, false, true);
    harmonicsBuffer.clear(0, numSamples);

    // Step 1: HPF + envelope tracking to isolate high-frequency content
    for (int ch = 0; ch < numChannels; ++ch)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float input = buffer.getSample(ch, sample);
            float filtered = hpFilters[ch].processSample(input);
            float env = envelopeFollowers[ch].process(filtered);

            // Scale by envelope (gate very quiet signals)
            float scaled = filtered * 2.0f * (env > 0.001f ? 1.0f : 0.0f);
            harmonicsBuffer.setSample(ch, sample, scaled);
        }
    }

    // Step 2: Upsample the filtered signal
    auto harmonicsBlock = juce::dsp::AudioBlock<float>(harmonicsBuffer).getSubBlock(0, static_cast<size_t>(numSamples));
    auto oversampledBlock = oversampling.processSamplesUp(harmonicsBlock);

    // Step 3: Apply waveshaping at the oversampled rate (correct order for anti-aliasing)
    int oversampledLength = static_cast<int>(oversampledBlock.getNumSamples());
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = oversampledBlock.getChannelPointer(static_cast<size_t>(ch));
        for (int i = 0; i < oversampledLength; ++i)
        {
            data[i] = exciterWaveshape(data[i]);
        }
    }

    // Step 4: Downsample back
    oversampling.processSamplesDown(harmonicsBlock);

    // Step 5: Blend harmonics into original signal based on amount
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float amt = amount.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float original = buffer.getSample(ch, sample);
            float harmonics = harmonicsBuffer.getSample(ch, sample);
            buffer.setSample(ch, sample, original + harmonics * amt);
        }
    }
}
