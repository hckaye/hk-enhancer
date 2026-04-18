#include "TextureEnhancer.h"

#include <cmath>

// --- Pink noise (Voss-McCartney) ---

int TextureEnhancer::PinkNoiseGen::countTrailingZeros(int n)
{
    if (n == 0)
        return 0;
    int count = 0;
    while ((n & 1) == 0)
    {
        count++;
        n >>= 1;
    }
    return count;
}

float TextureEnhancer::PinkNoiseGen::whiteNoise()
{
    // Simple xorshift32 PRNG
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return static_cast<float>(seed) / static_cast<float>(0xFFFFFFFF) * 2.0f - 1.0f;
}

float TextureEnhancer::PinkNoiseGen::next()
{
    // Voss-McCartney: on each step, update one row based on trailing zeros of index
    int rowToUpdate = countTrailingZeros(rowIndex);
    if (rowToUpdate >= kNumRows)
        rowToUpdate = kNumRows - 1;

    runningSum -= rows[rowToUpdate];
    float newVal = whiteNoise();
    rows[rowToUpdate] = newVal;
    runningSum += newVal;

    rowIndex++;

    // Add one more white noise sample for the high-frequency component
    float output = (runningSum + whiteNoise()) / static_cast<float>(kNumRows + 1);
    return output;
}

// --- TextureEnhancer ---

void TextureEnhancer::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    sr = sampleRate;

    for (int ch = 0; ch < 2; ++ch)
    {
        noiseGens[ch] = {};
        // Use different seeds per channel for stereo decorrelation
        noiseGens[ch].seed = (ch == 0) ? 12345u : 67890u;

        envelopeFollowers[ch].prepare(sampleRate);
        envelopeFollowers[ch].setAttackMs(5.0f);
        envelopeFollowers[ch].setReleaseMs(40.0f);

        // Highpass at 200Hz to keep noise out of the low end (avoids muddiness)
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 200.0f, 0.707f);
        noiseTiltFilters[ch].coefficients = coeffs;
        noiseTiltFilters[ch].reset();
    }
}

float TextureEnhancer::densitySaturate(float x, float drive)
{
    // Gentle polynomial saturation that enriches existing content
    // Produces both even and odd harmonics of whatever goes in
    // Much milder than tanh — preserves transients and character
    float driven = x * drive;
    float clipped = driven / (1.0f + std::fabs(driven)); // soft clip
    // Blend in a touch of asymmetry for even harmonics
    float asymmetry = driven > 0.0f ? 0.95f : 1.0f;
    return clipped * asymmetry;
}

void TextureEnhancer::process(juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& amount)
{
    int numSamples = buffer.getNumSamples();
    int numChannels = juce::jmin(buffer.getNumChannels(), 2);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float amt = amount.getNextValue();
        if (amt < 1e-6f)
            continue;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float input = buffer.getSample(ch, sample);
            float env = envelopeFollowers[ch].process(input);

            // --- Component 1: Harmonic density ---
            // Gentle saturation of the full signal — creates new harmonics
            // from ALL existing partials (fundamentals, overtones, noise, transients)
            float drive = 1.0f + amt * 1.2f; // 1.0 to 2.2
            float saturated = densitySaturate(input, drive);
            float densityContribution = (saturated - input) * amt;

            // --- Component 2: Dynamic noise texture ---
            // Pink noise shaped by the signal envelope — adds analog-like grit
            // Only audible when the signal is playing (masked by audio)
            float pinkNoise = noiseGens[ch].next();
            float shapedNoise = noiseTiltFilters[ch].processSample(pinkNoise);
            float noiseContribution = shapedNoise * env * amt * 0.15f;

            buffer.setSample(ch, sample, input + densityContribution + noiseContribution);
        }
    }
}
