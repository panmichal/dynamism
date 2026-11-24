#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EnvelopeEditor.h"
#include "CustomWidgets.h"

class VelocityEnvelopeEditor : public juce::AudioProcessorEditor
{
public:
    explicit VelocityEnvelopeEditor (VelocityEnvelopeProcessor& p);
    ~VelocityEnvelopeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& key) override;

private:
    VelocityEnvelopeProcessor& proc;
    EnvelopeComponent envComp;

    PaperSwitch      enabledSwitch { "Enabled" };
    PaperSwitch      drawSwitch    { "Draw (B)" };
    PaperSwitch      snapSwitch    { "Snap (S)" };
    PaperSwitch      loopSwitch    { "Loop" };
    juce::Label      gridLabel     { {}, "GRID" };
    SegmentedControl gridSeg;
    juce::Label      bpmLabel      { {}, "BPM (no transport)" };
    FlatKnob         bpmKnob;
    juce::TextButton resetButton   { "Reset envelope" };
    LcdReadout       lcd;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAtt, snapAtt, loopAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bpmAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VelocityEnvelopeEditor)
};
