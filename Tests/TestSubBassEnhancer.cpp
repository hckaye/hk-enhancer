#include "DSP/SubBassEnhancer.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

using Catch::Matchers::WithinAbs;

namespace
{
constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 512;
} // namespace

TEST_CASE("SubBassEnhancer passes silence", "[subbass]")
{
    SubBassEnhancer enhancer;
    enhancer.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    enhancer.process(buffer, amount);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE_THAT((double)buffer.getSample(ch, i), WithinAbs(0.0, 1e-5));
}

TEST_CASE("SubBassEnhancer with zero amount is bypass", "[subbass]")
{
    SubBassEnhancer enhancer;
    enhancer.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::AudioBuffer<float> original(2, kBlockSize);

    // 80Hz sine (bass fundamental)
    for (int i = 0; i < kBlockSize; ++i)
    {
        float val = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 80.0f * (float)i / (float)kSampleRate);
        buffer.setSample(0, i, val);
        buffer.setSample(1, i, val);
    }
    original.makeCopyOf(buffer);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(0.0f);

    enhancer.process(buffer, amount);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE_THAT((double)buffer.getSample(ch, i), WithinAbs((double)original.getSample(ch, i), 1e-4));
}

TEST_CASE("SubBassEnhancer adds sub-bass content to bass signal", "[subbass]")
{
    SubBassEnhancer enhancer;
    enhancer.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::AudioBuffer<float> original(2, kBlockSize);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    // Process multiple blocks of 80Hz sine for stabilization
    for (int block = 0; block < 15; ++block)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            float val = 0.7f * std::sin(2.0f * juce::MathConstants<float>::pi * 80.0f *
                                        (float)(block * kBlockSize + i) / (float)kSampleRate);
            buffer.setSample(0, i, val);
            buffer.setSample(1, i, val);
        }
        original.makeCopyOf(buffer);
        enhancer.process(buffer, amount);
    }

    // Signal should differ from original (sub-bass content added)
    float totalDiff = 0.0f;
    for (int i = 0; i < kBlockSize; ++i)
    {
        float diff = buffer.getSample(0, i) - original.getSample(0, i);
        totalDiff += diff * diff;
    }

    REQUIRE(totalDiff > 0.001f);
}

TEST_CASE("SubBassEnhancer noise gate suppresses quiet signals", "[subbass]")
{
    SubBassEnhancer enhancer;
    enhancer.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::AudioBuffer<float> original(2, kBlockSize);

    // Very quiet signal (below noise gate)
    for (int i = 0; i < kBlockSize; ++i)
    {
        float val = 0.0001f * std::sin(2.0f * juce::MathConstants<float>::pi * 80.0f * (float)i / (float)kSampleRate);
        buffer.setSample(0, i, val);
        buffer.setSample(1, i, val);
    }
    original.makeCopyOf(buffer);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    enhancer.process(buffer, amount);

    float totalDiff = 0.0f;
    for (int i = 0; i < kBlockSize; ++i)
    {
        float diff = buffer.getSample(0, i) - original.getSample(0, i);
        totalDiff += diff * diff;
    }

    REQUIRE(totalDiff < 1e-6f);
}

TEST_CASE("SubBassEnhancer output stays bounded", "[subbass]")
{
    SubBassEnhancer enhancer;
    enhancer.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    for (int block = 0; block < 10; ++block)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            float val = 0.9f * std::sin(2.0f * juce::MathConstants<float>::pi * 60.0f *
                                        (float)(block * kBlockSize + i) / (float)kSampleRate);
            buffer.setSample(0, i, val);
            buffer.setSample(1, i, val);
        }
        enhancer.process(buffer, amount);
    }

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE(std::fabs(buffer.getSample(ch, i)) < 5.0f);
}

TEST_CASE("SubBassEnhancer generates sub-bass from higher fundamentals (200Hz)", "[subbass]")
{
    SubBassEnhancer enhancer;
    enhancer.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::AudioBuffer<float> original(2, kBlockSize);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    // 200Hz sine — should still generate sub-bass via multi-octave division
    for (int block = 0; block < 20; ++block)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            float val = 0.7f * std::sin(2.0f * juce::MathConstants<float>::pi * 200.0f *
                                        (float)(block * kBlockSize + i) / (float)kSampleRate);
            buffer.setSample(0, i, val);
            buffer.setSample(1, i, val);
        }
        original.makeCopyOf(buffer);
        enhancer.process(buffer, amount);
    }

    float totalDiff = 0.0f;
    for (int i = 0; i < kBlockSize; ++i)
    {
        float diff = buffer.getSample(0, i) - original.getSample(0, i);
        totalDiff += diff * diff;
    }

    REQUIRE(totalDiff > 0.001f);
}

TEST_CASE("SubBassEnhancer calcOctavesDown targets 20-50Hz range", "[subbass]")
{
    // 80Hz -> 1 octave -> 40Hz (in range)
    REQUIRE(SubBassEnhancer::calcOctavesDown(80.0f) == 1);
    // 200Hz -> 2 octaves -> 50Hz (in range)
    REQUIRE(SubBassEnhancer::calcOctavesDown(200.0f) == 2);
    // 400Hz -> 3 octaves -> 50Hz (in range)
    REQUIRE(SubBassEnhancer::calcOctavesDown(400.0f) == 3);
    // 30Hz -> already low, but at least 1 octave
    REQUIRE(SubBassEnhancer::calcOctavesDown(30.0f) == 1);
}

TEST_CASE("SubBassEnhancer does not affect high frequency content", "[subbass]")
{
    SubBassEnhancer enhancer;
    enhancer.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::AudioBuffer<float> original(2, kBlockSize);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    // 5kHz sine — should not trigger sub-bass generation
    for (int block = 0; block < 10; ++block)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            float val = 0.8f * std::sin(2.0f * juce::MathConstants<float>::pi * 5000.0f *
                                        (float)(block * kBlockSize + i) / (float)kSampleRate);
            buffer.setSample(0, i, val);
            buffer.setSample(1, i, val);
        }
        original.makeCopyOf(buffer);
        enhancer.process(buffer, amount);
    }

    // Difference should be minimal (bandpass rejects 5kHz, small residual is acceptable)
    float totalDiff = 0.0f;
    for (int i = 0; i < kBlockSize; ++i)
    {
        float diff = buffer.getSample(0, i) - original.getSample(0, i);
        totalDiff += diff * diff;
    }

    REQUIRE(totalDiff < 0.1f);
}
