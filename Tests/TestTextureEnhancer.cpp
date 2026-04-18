#include "DSP/TextureEnhancer.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

using Catch::Matchers::WithinAbs;

namespace
{
constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 512;
} // namespace

TEST_CASE("TextureEnhancer passes silence", "[texture]")
{
    TextureEnhancer tex;
    tex.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    tex.process(buffer, amount);

    // With silence input, envelope is zero, so noise is gated — output should be near-silent
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE_THAT((double)buffer.getSample(ch, i), WithinAbs(0.0, 1e-4));
}

TEST_CASE("TextureEnhancer with zero amount is bypass", "[texture]")
{
    TextureEnhancer tex;
    tex.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::AudioBuffer<float> original(2, kBlockSize);

    for (int i = 0; i < kBlockSize; ++i)
    {
        float val = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * (float)i / (float)kSampleRate);
        buffer.setSample(0, i, val);
        buffer.setSample(1, i, val);
    }
    original.makeCopyOf(buffer);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(0.0f);

    tex.process(buffer, amount);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE_THAT((double)buffer.getSample(ch, i), WithinAbs((double)original.getSample(ch, i), 1e-6));
}

TEST_CASE("TextureEnhancer adds content to signal", "[texture]")
{
    TextureEnhancer tex;
    tex.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::AudioBuffer<float> original(2, kBlockSize);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    // Process multiple blocks to let envelope settle
    for (int block = 0; block < 10; ++block)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            float val = 0.7f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f *
                                        (float)(block * kBlockSize + i) / (float)kSampleRate);
            buffer.setSample(0, i, val);
            buffer.setSample(1, i, val);
        }
        original.makeCopyOf(buffer);
        tex.process(buffer, amount);
    }

    float totalDiff = 0.0f;
    for (int i = 0; i < kBlockSize; ++i)
    {
        float diff = buffer.getSample(0, i) - original.getSample(0, i);
        totalDiff += diff * diff;
    }

    REQUIRE(totalDiff > 0.001f);
}

TEST_CASE("TextureEnhancer output stays bounded", "[texture]")
{
    TextureEnhancer tex;
    tex.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    for (int block = 0; block < 10; ++block)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            float val = 0.9f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f *
                                        (float)(block * kBlockSize + i) / (float)kSampleRate);
            buffer.setSample(0, i, val);
            buffer.setSample(1, i, val);
        }
        tex.process(buffer, amount);
    }

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            REQUIRE(std::fabs(buffer.getSample(ch, i)) < 3.0f);
}

TEST_CASE("TextureEnhancer stereo channels differ (decorrelated noise)", "[texture]")
{
    TextureEnhancer tex;
    tex.prepare(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);

    juce::SmoothedValue<float> amount;
    amount.reset(kSampleRate, 0.0);
    amount.setCurrentAndTargetValue(1.0f);

    // Feed identical signal to both channels
    for (int block = 0; block < 10; ++block)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            float val = 0.6f * std::sin(2.0f * juce::MathConstants<float>::pi * 1000.0f *
                                        (float)(block * kBlockSize + i) / (float)kSampleRate);
            buffer.setSample(0, i, val);
            buffer.setSample(1, i, val);
        }
        tex.process(buffer, amount);
    }

    // Output should differ between channels due to decorrelated noise
    float channelDiff = 0.0f;
    for (int i = 0; i < kBlockSize; ++i)
    {
        float diff = buffer.getSample(0, i) - buffer.getSample(1, i);
        channelDiff += diff * diff;
    }

    REQUIRE(channelDiff > 1e-6f);
}
