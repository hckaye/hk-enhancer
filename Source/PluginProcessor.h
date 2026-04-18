#pragma once
#include "DSP/HarmonicExciter.h"
#include "DSP/MultibandSplitter.h"
#include "DSP/SubBassEnhancer.h"
#include "DSP/SubharmonicGenerator.h"
#include "DSP/TubeSaturator.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class HKEnhancerProcessor : public juce::AudioProcessor
{
public:
    HKEnhancerProcessor();
    ~HKEnhancerProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioProcessorValueTreeState apvts;

    MultibandSplitter splitter;
    SubharmonicGenerator subHarmonic;
    TubeSaturator tubeSaturator;
    HarmonicExciter harmonicExciter;
    SubBassEnhancer subBassEnhancer;

    // Buffers for each band
    juce::AudioBuffer<float> lowBand, midBand, highBand;

    // Pre-allocated buffer for dry signal (avoids malloc in processBlock)
    juce::AudioBuffer<float> dryBuffer;

    // Smoothed parameters
    juce::SmoothedValue<float> lowAmountSmoothed, midAmountSmoothed, highAmountSmoothed;
    juce::SmoothedValue<float> subBassAmountSmoothed;
    juce::SmoothedValue<float> outputGainSmoothed, mixSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HKEnhancerProcessor)
};
