#include "PluginProcessor.h"

#include "PluginEditor.h"

HKEnhancerProcessor::HKEnhancerProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout HKEnhancerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"lowAmount", 1},
                                                                 "Low Amount",
                                                                 juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
                                                                 0.0f,
                                                                 "%"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"midAmount", 1},
                                                                 "Mid Amount",
                                                                 juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
                                                                 0.0f,
                                                                 "%"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"highAmount", 1},
                                                                 "High Amount",
                                                                 juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
                                                                 0.0f,
                                                                 "%"));

    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"lowMidFreq", 1},
                                                    "Low/Mid Crossover",
                                                    juce::NormalisableRange<float>(80.0f, 500.0f, 1.0f, 0.5f),
                                                    200.0f,
                                                    "Hz"));

    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"midHighFreq", 1},
                                                    "Mid/High Crossover",
                                                    juce::NormalisableRange<float>(1000.0f, 8000.0f, 1.0f, 0.5f),
                                                    4000.0f,
                                                    "Hz"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"outputGain", 1},
                                                                 "Output Gain",
                                                                 juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
                                                                 0.0f,
                                                                 "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 1}, "Dry/Wet Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f, "%"));

    return {params.begin(), params.end()};
}

void HKEnhancerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare band buffers
    lowBand.setSize(2, samplesPerBlock);
    midBand.setSize(2, samplesPerBlock);
    highBand.setSize(2, samplesPerBlock);

    // Prepare DSP
    splitter.prepare(sampleRate, samplesPerBlock);
    subHarmonic.prepare(sampleRate, samplesPerBlock);
    tubeSaturator.prepare(sampleRate, samplesPerBlock);
    harmonicExciter.prepare(sampleRate, samplesPerBlock);

    // Prepare smoothed values
    double smoothTime = 0.02; // 20ms
    lowAmountSmoothed.reset(sampleRate, smoothTime);
    midAmountSmoothed.reset(sampleRate, smoothTime);
    highAmountSmoothed.reset(sampleRate, smoothTime);
    outputGainSmoothed.reset(sampleRate, smoothTime);
    mixSmoothed.reset(sampleRate, smoothTime);
}

void HKEnhancerProcessor::releaseResources()
{
    lowBand.setSize(0, 0);
    midBand.setSize(0, 0);
    highBand.setSize(0, 0);
}

void HKEnhancerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Clear unused channels
    for (int ch = numChannels; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, numSamples);

    if (numChannels < 2 || numSamples == 0)
        return;

    // Read parameters
    float lowAmount = apvts.getRawParameterValue("lowAmount")->load() / 100.0f;
    float midAmount = apvts.getRawParameterValue("midAmount")->load() / 100.0f;
    float highAmount = apvts.getRawParameterValue("highAmount")->load() / 100.0f;
    float lowMidFreq = apvts.getRawParameterValue("lowMidFreq")->load();
    float midHighFreq = apvts.getRawParameterValue("midHighFreq")->load();
    float outputGainDb = apvts.getRawParameterValue("outputGain")->load();
    float mix = apvts.getRawParameterValue("mix")->load() / 100.0f;

    lowAmountSmoothed.setTargetValue(lowAmount);
    midAmountSmoothed.setTargetValue(midAmount);
    highAmountSmoothed.setTargetValue(highAmount);
    outputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(outputGainDb));
    mixSmoothed.setTargetValue(mix);

    // Store dry signal for mix
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Update crossover frequencies
    splitter.setCrossoverFrequencies(lowMidFreq, midHighFreq);

    // Ensure band buffers are large enough
    if (lowBand.getNumSamples() < numSamples)
    {
        lowBand.setSize(2, numSamples, false, false, true);
        midBand.setSize(2, numSamples, false, false, true);
        highBand.setSize(2, numSamples, false, false, true);
    }

    lowBand.clear();
    midBand.clear();
    highBand.clear();

    // Split into 3 bands
    splitter.process(buffer, lowBand, midBand, highBand);

    // Process each band
    subHarmonic.process(lowBand, lowAmountSmoothed);
    tubeSaturator.process(midBand, midAmountSmoothed);
    harmonicExciter.process(highBand, highAmountSmoothed);

    // Sum bands back together
    buffer.clear();
    for (int ch = 0; ch < 2; ++ch)
    {
        buffer.addFrom(ch, 0, lowBand, ch, 0, numSamples);
        buffer.addFrom(ch, 0, midBand, ch, 0, numSamples);
        buffer.addFrom(ch, 0, highBand, ch, 0, numSamples);
    }

    // Apply output gain and dry/wet mix
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float gain = outputGainSmoothed.getNextValue();
        float mixVal = mixSmoothed.getNextValue();

        for (int ch = 0; ch < 2; ++ch)
        {
            float wet = buffer.getSample(ch, sample) * gain;
            float dry = dryBuffer.getSample(ch, sample);
            buffer.setSample(ch, sample, dry * (1.0f - mixVal) + wet * mixVal);
        }
    }
}

void HKEnhancerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HKEnhancerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* HKEnhancerProcessor::createEditor()
{
    return new HKEnhancerEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HKEnhancerProcessor();
}
