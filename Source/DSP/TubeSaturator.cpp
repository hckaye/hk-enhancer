#include "TubeSaturator.h"

#include <cmath>

void TubeSaturator::prepare(double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    oversampling.initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampling.reset();
}

float TubeSaturator::waveshape(float x, float drive)
{
    // Asymmetric soft clipping: even + odd harmonics
    if (x >= 0.0f)
        return std::tanh(drive * x);
    else
        return std::tanh(drive * x * 0.8f);
}

void TubeSaturator::process(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& amount)
{
    int numSamples = buffer.getNumSamples();
    int numChannels = juce::jmin(buffer.getNumChannels(), 2);

    // Make a copy of dry signal for blending
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Upsample
    auto block = juce::dsp::AudioBlock<float>(buffer).getSubBlock(0, static_cast<size_t>(numSamples));
    auto oversampledBlock = oversampling.processSamplesUp(block);

    int oversampledLength = static_cast<int>(oversampledBlock.getNumSamples());

    // Process with saturation at oversampled rate
    // Use a fixed moderate drive; the amount parameter controls wet/dry blend
    float drive = 4.0f;
    float makeup = 1.0f / std::tanh(drive); // compensate for volume loss

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = oversampledBlock.getChannelPointer(static_cast<size_t>(ch));
        for (int i = 0; i < oversampledLength; ++i)
        {
            data[i] = waveshape(data[i], drive) * makeup;
        }
    }

    // Downsample
    oversampling.processSamplesDown(block);

    // Blend dry/wet based on amount
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float amt = amount.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float dry = dryBuffer.getSample(ch, sample);
            float wet = buffer.getSample(ch, sample);
            buffer.setSample(ch, sample, dry * (1.0f - amt) + wet * amt);
        }
    }
}
