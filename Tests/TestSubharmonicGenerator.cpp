#include "DSP/SubharmonicGenerator.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

using Catch::Matchers::WithinAbs;

namespace
{
constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 512;
} // namespace

TEST_CASE("SubharmonicGenerator passes silence", "[subharmonic]")
{
    SubharmonicGenerator gen;
    gen.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    gen.process(buffer, amount);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE_THAT((double)buffer.getSample(ch, i), WithinAbs(0.0, 1e-5));
}

TEST_CASE("SubharmonicGenerator with zero amount is bypass", "[subharmonic]")
{
    SubharmonicGenerator gen;
    gen.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::AudioBuffer<float> original(2, kBlockSize);

    // 100Hz sine
    for (int i = 0; i < kBlockSize; ++i)
    {
        float val = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 100.0f * (float)i / (float)kSampleRate);
        buffer.setSample(0, i, val);
        buffer.setSample(1, i, val);
    }
    original.makeCopyOf(buffer);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(0.0f);

    gen.process(buffer, amount);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE_THAT((double)buffer.getSample(ch, i), WithinAbs((double)original.getSample(ch, i), 1e-4));
}

TEST_CASE("SubharmonicGenerator adds subharmonic content", "[subharmonic]")
{
    SubharmonicGenerator gen;
    gen.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::AudioBuffer<float> original(2, kBlockSize);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    // Process multiple blocks of 100Hz sine for the generator to stabilize
    for (int block = 0; block < 10; ++block)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            float val = 0.8f * std::sin(2.0f * juce::MathConstants<float>::pi * 100.0f *
                                        (float)(block * kBlockSize + i) / (float)kSampleRate);
            buffer.setSample(0, i, val);
            buffer.setSample(1, i, val);
        }
        original.makeCopyOf(buffer);
        gen.process(buffer, amount);
    }

    // Signal should differ from original (subharmonics added)
    float totalDiff = 0.0f;
    for (int i = 0; i < kBlockSize; ++i)
    {
        float diff = buffer.getSample(0, i) - original.getSample(0, i);
        totalDiff += diff * diff;
    }

    REQUIRE(totalDiff > 0.001f);
}

TEST_CASE("SubharmonicGenerator noise gate suppresses quiet signals", "[subharmonic]")
{
    SubharmonicGenerator gen;
    gen.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::AudioBuffer<float> original(2, kBlockSize);

    // Very quiet signal (below noise gate threshold ~0.001)
    for (int i = 0; i < kBlockSize; ++i)
    {
        float val = 0.0001f * std::sin(2.0f * juce::MathConstants<float>::pi * 100.0f * (float)i / (float)kSampleRate);
        buffer.setSample(0, i, val);
        buffer.setSample(1, i, val);
    }
    original.makeCopyOf(buffer);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    gen.process(buffer, amount);

    // Should be nearly identical to original (noise gate blocks subharmonic generation)
    float totalDiff = 0.0f;
    for (int i = 0; i < kBlockSize; ++i)
    {
        float diff = buffer.getSample(0, i) - original.getSample(0, i);
        totalDiff += diff * diff;
    }

    REQUIRE(totalDiff < 1e-6f);
}

TEST_CASE("SubharmonicGenerator output stays bounded", "[subharmonic]")
{
    SubharmonicGenerator gen;
    gen.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    for (int block = 0; block < 5; ++block)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            float val = 0.9f * std::sin(2.0f * juce::MathConstants<float>::pi * 80.0f *
                                        (float)(block * kBlockSize + i) / (float)kSampleRate);
            buffer.setSample(0, i, val);
            buffer.setSample(1, i, val);
        }
        gen.process(buffer, amount);
    }

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE(std::fabs(buffer.getSample(ch, i)) < 5.0f);
}
