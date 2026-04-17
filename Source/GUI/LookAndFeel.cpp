#include "LookAndFeel.h"

HKEnhancerLookAndFeel::HKEnhancerLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::lightgrey);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff222244));
    setColour(juce::Label::textColourId, juce::Colours::lightgrey);
}

void HKEnhancerLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                             int x,
                                             int y,
                                             int width,
                                             int height,
                                             float sliderPos,
                                             float rotaryStartAngle,
                                             float rotaryEndAngle,
                                             juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Background arc (track)
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(
        centreX, centreY, radius - 2.0f, radius - 2.0f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(0xff333355));
    g.strokePath(backgroundArc,
                 juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc
    if (sliderPos > 0.0f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f, 0.0f, rotaryStartAngle, angle, true);

        // Try to use the slider's custom colour, fallback to white
        auto colour = slider.findColour(juce::Slider::thumbColourId);
        if (colour == juce::Colour())
            colour = juce::Colours::white;
        g.setColour(colour);
        g.strokePath(valueArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Knob body
    auto knobRadius = radius * 0.65f;
    g.setColour(juce::Colour(0xff2a2a4a));
    g.fillEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

    // Pointer
    juce::Path pointer;
    auto pointerLength = knobRadius * 0.8f;
    auto pointerThickness = 2.5f;
    pointer.addRoundedRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength, 1.0f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    auto colour = slider.findColour(juce::Slider::thumbColourId);
    if (colour == juce::Colour())
        colour = juce::Colours::white;
    g.setColour(colour);
    g.fillPath(pointer);
}
