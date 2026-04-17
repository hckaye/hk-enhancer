#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class BandControl : public juce::Component
{
public:
    BandControl(juce::String name,
                juce::Colour colour,
                juce::AudioProcessorValueTreeState& apvts,
                const juce::String& paramId);

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::String bandName;
    juce::Colour bandColour;

    juce::Slider knob;
    juce::Label nameLabel;
    juce::Label valueLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BandControl)
};
