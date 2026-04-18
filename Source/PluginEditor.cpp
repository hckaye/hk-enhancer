#include "PluginEditor.h"

HKEnhancerEditor::HKEnhancerEditor(HKEnhancerProcessor& p)
    : AudioProcessorEditor(&p), subBassControl("SUB", juce::Colour(0xffcc44ff), p.getAPVTS(), "subBassAmount"),
      lowControl("LOW", juce::Colour(0xff4a9eff), p.getAPVTS(), "lowAmount"),
      midControl("MID", juce::Colour(0xff7acc52), p.getAPVTS(), "midAmount"),
      highControl("HIGH", juce::Colour(0xffff8c42), p.getAPVTS(), "highAmount"),
      textureControl("TEXTURE", juce::Colour(0xffff5577), p.getAPVTS(), "textureAmount")
{
    setLookAndFeel(&lookAndFeel);
    setSize(820, 400);

    addAndMakeVisible(subBassControl);
    addAndMakeVisible(lowControl);
    addAndMakeVisible(midControl);
    addAndMakeVisible(highControl);
    addAndMakeVisible(textureControl);

    setupSmallKnob(lowMidFreqSlider, lowMidFreqLabel, "LOW/MID");
    setupSmallKnob(midHighFreqSlider, midHighFreqLabel, "MID/HIGH");
    setupSmallKnob(outputGainSlider, outputGainLabel, "OUTPUT");
    setupSmallKnob(mixSlider, mixLabel, "MIX");

    auto& apvts = p.getAPVTS();
    lowMidFreqAttachment = std::make_unique<SliderAttachment>(apvts, "lowMidFreq", lowMidFreqSlider);
    midHighFreqAttachment = std::make_unique<SliderAttachment>(apvts, "midHighFreq", midHighFreqSlider);
    outputGainAttachment = std::make_unique<SliderAttachment>(apvts, "outputGain", outputGainSlider);
    mixAttachment = std::make_unique<SliderAttachment>(apvts, "mix", mixSlider);
}

HKEnhancerEditor::~HKEnhancerEditor()
{
    setLookAndFeel(nullptr);
}

void HKEnhancerEditor::setupSmallKnob(juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(11.0f));
    label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(label);
}

void HKEnhancerEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("HK ENHANCER", getLocalBounds().removeFromTop(50), juce::Justification::centred);

    // Divider line
    g.setColour(juce::Colour(0xff333355));
    g.drawHorizontalLine(260, 20.0f, static_cast<float>(getWidth()) - 20.0f);
}

void HKEnhancerEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(50); // title space

    // Main band controls (top section) — 5 controls
    auto bandArea = area.removeFromTop(200);
    int bandWidth = bandArea.getWidth() / 5;
    subBassControl.setBounds(bandArea.removeFromLeft(bandWidth));
    lowControl.setBounds(bandArea.removeFromLeft(bandWidth));
    midControl.setBounds(bandArea.removeFromLeft(bandWidth));
    highControl.setBounds(bandArea.removeFromLeft(bandWidth));
    textureControl.setBounds(bandArea);

    // Bottom controls
    area.removeFromTop(20); // spacing
    auto bottomArea = area.removeFromTop(100);
    int knobWidth = bottomArea.getWidth() / 4;
    int knobSize = 65;
    int labelHeight = 18;

    auto placeKnob = [&](juce::Slider& slider, juce::Label& label, juce::Rectangle<int> col)
    {
        auto centered = col.withSizeKeepingCentre(knobSize, knobSize + labelHeight + 4);
        label.setBounds(centered.removeFromTop(labelHeight));
        slider.setBounds(centered);
    };

    placeKnob(lowMidFreqSlider, lowMidFreqLabel, bottomArea.removeFromLeft(knobWidth));
    placeKnob(midHighFreqSlider, midHighFreqLabel, bottomArea.removeFromLeft(knobWidth));
    placeKnob(outputGainSlider, outputGainLabel, bottomArea.removeFromLeft(knobWidth));
    placeKnob(mixSlider, mixLabel, bottomArea);
}
