#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Flat "paper / analog" theme (dreamcoder-style: cream paper, ink text,
// hairline borders, muted accents, monospace details — no gloss, no glow).
namespace Theme
{
    const juce::Colour paper    { 0xfff2ede1 };
    const juce::Colour panel    { 0xfffbf8f0 };
    const juce::Colour ink      { 0xff2b2b26 };
    const juce::Colour faint    { 0xff8a8577 };
    const juce::Colour hairline { 0xffd9d3c3 };
    const juce::Colour grid     { 0xffe6e0d0 };
    const juce::Colour gridBar  { 0xffd3ccba };
    const juce::Colour blue     { 0xff3d5a80 };
    const juce::Colour blueSoft { 0xff4d7fb8 };
    const juce::Colour orange   { 0xffc97b3d };
    const juce::Colour danger   { 0xffc0533d };
    const juce::Colour lcdBg    { 0xff26241f };
    const juce::Colour lcdText  { 0xffe0a94e };

    inline juce::FontOptions mono (float h) { return juce::FontOptions (h).withName ("Menlo"); }
    inline juce::FontOptions sans (float h) { return juce::FontOptions (h); }
}

// Flat toggle switch with a sliding knob (label on the left, switch on the right).
class PaperSwitch : public juce::ToggleButton
{
public:
    using juce::ToggleButton::ToggleButton;
    void paint (juce::Graphics& g) override;
};

// Flat segmented control (used for the note-grid selector), attached to an
// AudioParameterChoice.
class SegmentedControl : public juce::Component,
                         private juce::AudioProcessorValueTreeState::Listener
{
public:
    SegmentedControl (juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& paramID,
                      juce::StringArray items);
    ~SegmentedControl() override;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;

private:
    void parameterChanged (const juce::String& paramID, float newValue) override;

    juce::AudioProcessorValueTreeState& apvts;
    juce::String paramID;
    juce::StringArray items;
    juce::AudioParameterChoice* param = nullptr;
    int selected = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SegmentedControl)
};

// Flat rotary knob (vertical drag), with a hairline value arc.
class FlatKnob : public juce::Slider
{
public:
    FlatKnob();
    void paint (juce::Graphics& g) override;
};

// Dark monospace LCD readout: transport state, BPM, envelope phase.
class LcdReadout : public juce::Component,
                   private juce::Timer
{
public:
    explicit LcdReadout (VelocityEnvelopeProcessor& p);
    ~LcdReadout() override;

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override { repaint(); }

    VelocityEnvelopeProcessor& proc;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LcdReadout)
};
