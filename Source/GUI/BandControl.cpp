#include "BandControl.h"

#include <utility>

BandControl::BandControl(juce::String name,
                         juce::Colour colour,
                         juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& paramId)
    : bandName(std::move(name)), bandColour(colour)
{
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    knob.setColour(juce::Slider::thumbColourId, colour);
    knob.setColour(juce::Slider::rotarySliderFillColourId, colour);
    addAndMakeVisible(knob);

    nameLabel.setText(bandName, juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, colour);
    addAndMakeVisible(nameLabel);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, knob);
}

void BandControl::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(8.0f);
    g.setColour(juce::Colour(0xff222244).withAlpha(0.5f));
    g.fillRoundedRectangle(bounds, 8.0f);

    // Subtle border
    g.setColour(bandColour.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
}

void BandControl::resized()
{
    auto area = getLocalBounds().reduced(12);

    nameLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);

    auto knobArea = area.withSizeKeepingCentre(juce::jmin(area.getWidth(), 120), juce::jmin(area.getHeight(), 140));
    knob.setBounds(knobArea);
}
