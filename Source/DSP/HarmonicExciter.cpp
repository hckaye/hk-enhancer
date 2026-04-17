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

    // We need temporary storage for the harmonics we generate
    juce::AudioBuffer<float> harmonicsBuffer(numChannels, numSamples);
    harmonicsBuffer.clear();

    // Extract harmonics: HPF -> envelope tracking -> waveshaping
    for (int ch = 0; ch < numChannels; ++ch)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float input = buffer.getSample(ch, sample);

            // Highpass filter to isolate high-frequency content
            float filtered = hpFilters[ch].processSample(input);

            // Track envelope for dynamic scaling
            float env = envelopeFollowers[ch].process(filtered);

            // Generate harmonics via waveshaping
            float harmonic = exciterWaveshape(filtered * 2.0f); // boost before clipping

            // Scale harmonics by envelope (program-dependent)
            float scaledHarmonic = harmonic * (env > 0.001f ? 1.0f : 0.0f);

            harmonicsBuffer.setSample(ch, sample, scaledHarmonic);
        }
    }

    // Oversample the harmonics to reduce aliasing
    auto harmonicsBlock = juce::dsp::AudioBlock<float>(harmonicsBuffer).getSubBlock(0, static_cast<size_t>(numSamples));
    oversampling.processSamplesUp(harmonicsBlock);

    // No additional processing needed at oversampled rate for the exciter
    // (the waveshaping was already done, oversampling just filters the result)

    oversampling.processSamplesDown(harmonicsBlock);

    // Blend harmonics into original signal based on amount
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
