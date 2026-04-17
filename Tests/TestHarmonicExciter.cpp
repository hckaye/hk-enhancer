#include "DSP/HarmonicExciter.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

using Catch::Matchers::WithinAbs;

namespace
{
constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 512;
} // namespace

TEST_CASE("HarmonicExciter passes silence", "[exciter]")
{
    HarmonicExciter exciter;
    exciter.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    exciter.process(buffer, amount);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE_THAT((double)buffer.getSample(ch, i), WithinAbs(0.0, 1e-5));
}

TEST_CASE("HarmonicExciter with zero amount is bypass", "[exciter]")
{
    HarmonicExciter exciter;
    exciter.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::AudioBuffer<float> original(2, kBlockSize);

    // High frequency sine (5kHz - above the exciter's HPF)
    for (int i = 0; i < kBlockSize; ++i)
    {
        float val = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 5000.0f * (float)i / (float)kSampleRate);
        buffer.setSample(0, i, val);
        buffer.setSample(1, i, val);
    }
    original.makeCopyOf(buffer);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(0.0f);

    exciter.process(buffer, amount);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE_THAT((double)buffer.getSample(ch, i), WithinAbs((double)original.getSample(ch, i), 1e-4));
}

TEST_CASE("HarmonicExciter adds harmonics to high content", "[exciter]")
{
    HarmonicExciter exciter;
    exciter.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::AudioBuffer<float> original(2, kBlockSize);

    // 5kHz sine — above HPF cutoff, should generate harmonics
    for (int i = 0; i < kBlockSize; ++i)
    {
        float val = 0.8f * std::sin(2.0f * juce::MathConstants<float>::pi * 5000.0f * (float)i / (float)kSampleRate);
        buffer.setSample(0, i, val);
        buffer.setSample(1, i, val);
    }
    original.makeCopyOf(buffer);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    // Process multiple blocks for filter settling
    for (int block = 0; block < 5; ++block)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            float val = 0.8f * std::sin(2.0f * juce::MathConstants<float>::pi * 5000.0f *
                                        (float)(block * kBlockSize + i) / (float)kSampleRate);
            buffer.setSample(0, i, val);
            buffer.setSample(1, i, val);
        }
        original.makeCopyOf(buffer);
        exciter.process(buffer, amount);
    }

    // Signal should be modified
    float totalDiff = 0.0f;
    for (int i = 0; i < kBlockSize; ++i)
    {
        float diff = buffer.getSample(0, i) - original.getSample(0, i);
        totalDiff += diff * diff;
    }

    REQUIRE(totalDiff > 0.001f);
}

TEST_CASE("HarmonicExciter output stays bounded", "[exciter]")
{
    HarmonicExciter exciter;
    exciter.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);

    for (int i = 0; i < kBlockSize; ++i)
    {
        float val = 0.9f * std::sin(2.0f * juce::MathConstants<float>::pi * 6000.0f * (float)i / (float)kSampleRate);
        buffer.setSample(0, i, val);
        buffer.setSample(1, i, val);
    }

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    exciter.process(buffer, amount);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE(std::fabs(buffer.getSample(ch, i)) < 5.0f);
}
