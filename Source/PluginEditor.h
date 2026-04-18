#pragma once
#include "GUI/BandControl.h"
#include "GUI/LookAndFeel.h"
#include "PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

class HKEnhancerEditor : public juce::AudioProcessorEditor
{
public:
    explicit HKEnhancerEditor(HKEnhancerProcessor&);
    ~HKEnhancerEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    HKEnhancerLookAndFeel lookAndFeel;

    BandControl subBassControl, lowControl, midControl, highControl, textureControl;

    juce::Slider lowMidFreqSlider, midHighFreqSlider, outputGainSlider, mixSlider;
    juce::Label lowMidFreqLabel, midHighFreqLabel, outputGainLabel, mixLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> lowMidFreqAttachment, midHighFreqAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment, mixAttachment;

    void setupSmallKnob(juce::Slider& slider, juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HKEnhancerEditor)
};
