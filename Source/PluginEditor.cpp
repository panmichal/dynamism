#include "PluginEditor.h"

namespace
{
// Flat paper LookAndFeel for the remaining stock widgets (reset button, popups).
class PaperLaf : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.f);
        g.setColour (highlighted ? Theme::hairline : Theme::panel);
        g.fillRoundedRectangle (r, 6.f);
        g.setColour (Theme::hairline.darker (0.25f));
        g.drawRoundedRectangle (r, 6.f, 1.f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        g.setColour (Theme::ink);
        g.setFont (Theme::sans (12.f));
        g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
    }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (Theme::panel);
        g.setColour (Theme::hairline);
        g.drawRect (0, 0, w, h, 1);
    }

    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool, bool isHighlighted, bool, bool,
                            const juce::String& text, const juce::String&, const juce::Drawable*,
                            const juce::Colour* textColour) override
    {
        if (isSeparator)
        {
            g.setColour (Theme::hairline);
            g.fillRect (area.reduced (4, 0));
            return;
        }
        if (isHighlighted)
        {
            g.setColour (Theme::hairline);
            g.fillRect (area);
        }
        g.setColour (textColour != nullptr ? *textColour : Theme::ink);
        g.setFont (Theme::sans (12.f));
        g.drawText (text, area.reduced (8, 0), juce::Justification::centredLeft);
    }
};
}

VelocityEnvelopeEditor::VelocityEnvelopeEditor (VelocityEnvelopeProcessor& p)
    : AudioProcessorEditor (&p), proc (p), envComp (p), gridSeg (p.apvts, "grid", { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" }),
      lcd (p)
{
    static PaperLaf laf;
    setLookAndFeel (&laf);
    setWantsKeyboardFocus (true);

    for (auto* c : { &enabledSwitch, &drawSwitch, &snapSwitch, &loopSwitch })
        addAndMakeVisible (c);

    gridLabel.setColour (juce::Label::textColourId, Theme::faint);
    gridLabel.setFont (Theme::mono (9.f));
    addAndMakeVisible (gridLabel);
    addAndMakeVisible (gridSeg);

    bpmLabel.setColour (juce::Label::textColourId, Theme::faint);
    bpmLabel.setFont (Theme::mono (9.f));
    addAndMakeVisible (bpmLabel);
    addAndMakeVisible (bpmKnob);

    addAndMakeVisible (resetButton);
    addAndMakeVisible (lcd);

    envComp.setWantsKeyboardFocus (true);
    addAndMakeVisible (envComp);

    enabledAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "enabled", enabledSwitch);
    snapAtt    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "snap", snapSwitch);
    loopAtt    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "loop", loopSwitch);
    bpmAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "bpm", bpmKnob);

    auto toggleDraw = [this]
    {
        envComp.setDrawMode (drawSwitch.getToggleState());
        envComp.grabKeyboardFocus();
    };
    drawSwitch.onClick = toggleDraw;
    envComp.onDrawModeChanged = [this] (bool d)
    {
        drawSwitch.setToggleState (d, juce::dontSendNotification);
    };

    resetButton.onClick = [this]
    {
        proc.resetEnvelopeToDefault();
        envComp.refreshFromProcessor();
    };

    setSize (940, 540);
}

VelocityEnvelopeEditor::~VelocityEnvelopeEditor()
{
    setLookAndFeel (nullptr);
}

bool VelocityEnvelopeEditor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress ('b'))
    {
        drawSwitch.setToggleState (! drawSwitch.getToggleState(), juce::dontSendNotification);
        envComp.setDrawMode (drawSwitch.getToggleState());
        return true;
    }
    return AudioProcessorEditor::keyPressed (key);
}

void VelocityEnvelopeEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::paper);

    // hairline frame
    g.setColour (Theme::hairline);
    g.drawRect (getLocalBounds().toFloat(), 1.f);

    g.setColour (Theme::ink);
    g.setFont (Theme::sans (15.f).withStyle ("Bold"));
    g.drawText ("VELOCITY ENVELOPE", 14, 6, 320, 20, juce::Justification::left);

    g.setColour (Theme::faint);
    g.setFont (Theme::mono (10.f));
    g.drawText ("draw a velocity envelope for the audio   |   B draw mode   |   S snap   |   alt free   |   shift snap",
                14, 25, 700, 14, juce::Justification::left);
}

void VelocityEnvelopeEditor::resized()
{
    auto r = getLocalBounds().reduced (12);
    r.removeFromTop (40);            // title + subtitle

    // LCD readout, top-right of the title row
    lcd.setBounds (getLocalBounds().reduced (12).withTrimmedTop (6).removeFromRight (230).withHeight (22));

    auto top = r.removeFromTop (74); // controls row (knob needs the height)

    enabledSwitch.setBounds (top.removeFromLeft (92).withY (top.getY() + 8).withHeight (26));
    top.removeFromLeft (6);
    drawSwitch.setBounds    (top.removeFromLeft (100).withY (top.getY() + 8).withHeight (26));
    top.removeFromLeft (6);
    snapSwitch.setBounds    (top.removeFromLeft (92).withY (top.getY() + 8).withHeight (26));
    loopSwitch.setBounds    (top.removeFromLeft (80).withY (top.getY() + 8).withHeight (26));
    top.removeFromLeft (14);

    // grid selector with label above
    auto gridArea = top.removeFromLeft (280);
    gridLabel.setBounds (gridArea.withHeight (12));
    gridSeg.setBounds (gridArea.withTrimmedTop (14).withHeight (26));
    top.removeFromLeft (14);

    // BPM knob with label above
    auto knobArea = top.removeFromLeft (80);
    bpmLabel.setBounds (knobArea.withHeight (12));
    bpmKnob.setBounds (knobArea.withTrimmedTop (13).withHeight (58));
    top.removeFromLeft (14);

    resetButton.setBounds (top.removeFromLeft (110).withY (top.getY() + 8).withHeight (26));

    envComp.setBounds (r.reduced (0, 6));
}
