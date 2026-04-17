#include "MultibandSplitter.h"

#include <cmath>

void MultibandSplitter::prepare(double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec{};
    spec.sampleRate = newSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    lp1.prepare(spec);
    hp1.prepare(spec);
    lp2.prepare(spec);
    hp2.prepare(spec);
    ap2Lp.prepare(spec);
    ap2Hp.prepare(spec);

    lp1.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    hp1.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    lp2.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    hp2.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    ap2Lp.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    ap2Hp.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

    setCrossoverFrequencies(200.0f, 4000.0f);

    tempBuffer.setSize(2, samplesPerBlock);
    allpassBuffer.setSize(2, samplesPerBlock);
}

void MultibandSplitter::setCrossoverFrequencies(float lowMidHz, float midHighHz)
{
    if (std::fabs(lowMidHz - currentLowMidFreq) > 0.01f)
    {
        currentLowMidFreq = lowMidHz;
        lp1.setCutoffFrequency(lowMidHz);
        hp1.setCutoffFrequency(lowMidHz);
    }

    if (std::fabs(midHighHz - currentMidHighFreq) > 0.01f)
    {
        currentMidHighFreq = midHighHz;
        lp2.setCutoffFrequency(midHighHz);
        hp2.setCutoffFrequency(midHighHz);
        ap2Lp.setCutoffFrequency(midHighHz);
        ap2Hp.setCutoffFrequency(midHighHz);
    }
}

void MultibandSplitter::process(const juce::AudioBuffer<float>& input,
                                juce::AudioBuffer<float>& low,
                                juce::AudioBuffer<float>& mid,
                                juce::AudioBuffer<float>& high)
{
    int numSamples = input.getNumSamples();

    // Ensure temp buffers are large enough
    if (tempBuffer.getNumSamples() < numSamples)
    {
        tempBuffer.setSize(2, numSamples, false, false, true);
        allpassBuffer.setSize(2, numSamples, false, false, true);
    }

    // Copy input to low and temp (for hp1)
    for (int ch = 0; ch < 2; ++ch)
    {
        low.copyFrom(ch, 0, input, ch, 0, numSamples);
        tempBuffer.copyFrom(ch, 0, input, ch, 0, numSamples);
    }

    // Crossover 1: split into low and (mid+high)
    {
        auto lowBlock = juce::dsp::AudioBlock<float>(low);
        auto tempBlock = juce::dsp::AudioBlock<float>(tempBuffer);
        lp1.process(juce::dsp::ProcessContextReplacing<float>(lowBlock));
        hp1.process(juce::dsp::ProcessContextReplacing<float>(tempBlock));
    }

    // Allpass compensation for low band: pass through LR4 pair at midHighFreq
    // This matches the phase shift that crossover 2 introduces on the mid+high path
    for (int ch = 0; ch < 2; ++ch)
    {
        allpassBuffer.copyFrom(ch, 0, low, ch, 0, numSamples);
    }
    {
        auto apBlock = juce::dsp::AudioBlock<float>(allpassBuffer);
        auto lowBlock = juce::dsp::AudioBlock<float>(low);

        // Process low through both LP and HP of crossover 2 frequency, then sum = allpass
        ap2Lp.process(juce::dsp::ProcessContextReplacing<float>(lowBlock));
        ap2Hp.process(juce::dsp::ProcessContextReplacing<float>(apBlock));

        // Sum: low = lp_output + hp_output (= allpass)
        for (int ch = 0; ch < 2; ++ch)
            low.addFrom(ch, 0, allpassBuffer, ch, 0, numSamples);
    }

    // Crossover 2: split temp (mid+high) into mid and high
    for (int ch = 0; ch < 2; ++ch)
    {
        mid.copyFrom(ch, 0, tempBuffer, ch, 0, numSamples);
        high.copyFrom(ch, 0, tempBuffer, ch, 0, numSamples);
    }
    {
        auto midBlock = juce::dsp::AudioBlock<float>(mid);
        auto highBlock = juce::dsp::AudioBlock<float>(high);
        lp2.process(juce::dsp::ProcessContextReplacing<float>(midBlock));
        hp2.process(juce::dsp::ProcessContextReplacing<float>(highBlock));
    }
}
