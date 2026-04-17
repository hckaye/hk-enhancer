#include "DSP/TubeSaturator.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

using Catch::Matchers::WithinAbs;

namespace
{
constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 512;
} // namespace

TEST_CASE("TubeSaturator passes silence", "[saturator]")
{
    TubeSaturator sat;
    sat.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    sat.process(buffer, amount);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE_THAT((double)buffer.getSample(ch, i), WithinAbs(0.0, 1e-5));
}

TEST_CASE("TubeSaturator with zero amount is bypass", "[saturator]")
{
    TubeSaturator sat;
    sat.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::AudioBuffer<float> original(2, kBlockSize);

    // Fill with sine
    for (int i = 0; i < kBlockSize; ++i)
    {
        float val = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 1000.0f * (float)i / (float)kSampleRate);
        buffer.setSample(0, i, val);
        buffer.setSample(1, i, val);
    }
    original.makeCopyOf(buffer);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(0.0f);

    sat.process(buffer, amount);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE_THAT((double)buffer.getSample(ch, i), WithinAbs((double)original.getSample(ch, i), 1e-4));
}

TEST_CASE("TubeSaturator adds harmonics", "[saturator]")
{
    TubeSaturator sat;
    sat.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::AudioBuffer<float> original(2, kBlockSize);

    // Fill with sine
    for (int i = 0; i < kBlockSize; ++i)
    {
        float val = 0.8f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * (float)i / (float)kSampleRate);
        buffer.setSample(0, i, val);
        buffer.setSample(1, i, val);
    }
    original.makeCopyOf(buffer);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    sat.process(buffer, amount);

    // The processed signal should differ from the original (harmonics added)
    float totalDiff = 0.0f;
    for (int i = 0; i < kBlockSize; ++i)
    {
        float diff = buffer.getSample(0, i) - original.getSample(0, i);
        totalDiff += diff * diff;
    }

    REQUIRE(totalDiff > 0.01f);
}

TEST_CASE("TubeSaturator does not clip beyond reasonable bounds", "[saturator]")
{
    TubeSaturator sat;
    sat.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);

    for (int i = 0; i < kBlockSize; ++i)
    {
        float val = 0.9f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * (float)i / (float)kSampleRate);
        buffer.setSample(0, i, val);
        buffer.setSample(1, i, val);
    }

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    sat.process(buffer, amount);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE(std::fabs(buffer.getSample(ch, i)) < 3.0f);
}
