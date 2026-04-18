#include "SubBassEnhancer.h"

#include <cmath>

void SubBassEnhancer::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    sr = sampleRate;

    for (int ch = 0; ch < 2; ++ch)
    {
        channelStates[ch] = {};
        compGain[ch] = 1.0f;

        envelopeFollowers[ch].prepare(sampleRate);
        envelopeFollowers[ch].setAttackMs(8.0f);
        envelopeFollowers[ch].setReleaseMs(30.0f);

        // Wider bandpass to capture fundamentals from ~30Hz to ~300Hz
        auto bpCoeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, 120.0f, 0.7f);
        bpFilters[ch].coefficients = bpCoeffs;
        bpFilters[ch].reset();

        // Smoothing LPF at 25Hz — allows 20Hz+ content through while rounding square edges
        auto smoothCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 25.0f, 0.5f);
        subSmoothingFilters[ch].coefficients = smoothCoeffs;
        subSmoothingFilters[ch].reset();
    }
}

int SubBassEnhancer::calcOctavesDown(float fundamentalFreq)
{
    // Find how many octaves to shift down so the result lands in ~20-50Hz
    // freq / 2^n should be in [targetSubFreqLow, targetSubFreqHigh]
    if (fundamentalFreq <= 0.0f)
        return 1;

    int octaves = 0;
    float freq = fundamentalFreq;
    while (freq > targetSubFreqHigh && octaves < 4)
    {
        freq *= 0.5f;
        octaves++;
    }

    // At least 1 octave down
    return juce::jmax(1, octaves);
}

float SubBassEnhancer::warmSaturate(float x)
{
    // Soft saturation with even-harmonic bias for warmth
    if (x >= 0.0f)
        return std::tanh(x * 1.5f) * 0.9f;
    else
        return std::tanh(x * 1.2f);
}

float SubBassEnhancer::compressEnvelope(float envelope, int channel)
{
    constexpr float threshold = 0.1f;
    constexpr float ratio = 3.0f;
    constexpr float smoothCoeff = 0.995f;

    float targetGain = 1.0f;
    if (envelope > threshold)
    {
        float overDb = 20.0f * std::log10(envelope / threshold);
        float compressedDb = overDb / ratio;
        float targetLevel = threshold * std::pow(10.0f, compressedDb / 20.0f);
        targetGain = targetLevel / envelope;
    }

    compGain[channel] = smoothCoeff * compGain[channel] + (1.0f - smoothCoeff) * targetGain;
    return compGain[channel];
}

float SubBassEnhancer::processChannel(float input, int channel)
{
    auto& state = channelStates[channel];

    // 1. Bandpass filter to isolate fundamental
    float filtered = bpFilters[channel].processSample(input);

    // 2. Envelope follower
    float env = envelopeFollowers[channel].process(filtered);

    state.samplesSinceLastCross++;

    // Noise gate
    if (env < noiseGateThreshold)
    {
        state.prevFiltered = filtered;
        return 0.0f;
    }

    // 3. Zero-crossing detection (positive-going) + frequency estimation
    bool zeroCrossing = (state.prevFiltered <= 0.0f && filtered > 0.0f);
    state.prevFiltered = filtered;

    if (zeroCrossing)
    {
        // Estimate fundamental frequency from zero-crossing interval
        if (state.samplesSinceLastCross > 0)
        {
            float rawFreq = static_cast<float>(sr) / static_cast<float>(state.samplesSinceLastCross);
            // Smooth the frequency estimate to avoid jitter
            constexpr float freqSmooth = 0.85f;
            state.estimatedFreq = freqSmooth * state.estimatedFreq + (1.0f - freqSmooth) * rawFreq;
        }
        state.samplesSinceLastCross = 0;

        // 4. Cascaded frequency divider chain
        // Stage 0: ÷2, Stage 1: ÷4, Stage 2: ÷8, Stage 3: ÷16
        // Each stage toggles on every other trigger from the previous stage
        bool trigger = true; // zero-crossing is the initial trigger
        for (int stage = 0; stage < 4; ++stage)
        {
            if (trigger)
            {
                state.dividerCount[stage]++;
                if (state.dividerCount[stage] >= 2)
                {
                    state.dividerState[stage] = !state.dividerState[stage];
                    state.dividerCount[stage] = 0;
                    trigger = true; // propagate to next stage
                }
                else
                {
                    trigger = false;
                }
            }
            else
            {
                break;
            }
        }
    }

    // 5. Pick the right divider stage based on estimated frequency
    int octavesDown = calcOctavesDown(state.estimatedFreq);
    int stageIndex = juce::jlimit(0, 3, octavesDown - 1);
    float rawSub = state.dividerState[stageIndex] ? 1.0f : -1.0f;

    // 6. Smooth into sine-like wave
    float smoothedSub = subSmoothingFilters[channel].processSample(rawSub);

    // 7. Modulate by original envelope (natural dynamics)
    float dynamicSub = smoothedSub * env * 3.5f;

    // 8. Warm saturation for even harmonics
    float saturatedSub = warmSaturate(dynamicSub);

    // 9. Compression for consistent pressure
    float subEnv = std::fabs(saturatedSub);
    float gain = compressEnvelope(subEnv, channel);
    return saturatedSub * gain;
}

void SubBassEnhancer::process(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& amount)
{
    int numSamples = buffer.getNumSamples();
    int numChannels = juce::jmin(buffer.getNumChannels(), 2);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float amt = amount.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float input = buffer.getSample(ch, sample);
            float subBass = processChannel(input, ch);
            buffer.setSample(ch, sample, input + subBass * amt);
        }
    }
}
