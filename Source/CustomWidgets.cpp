#include "CustomWidgets.h"

//==============================================================================
void PaperSwitch::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const bool on = getToggleState();
    const bool hover = isEnabled() && isMouseOver();

    g.setColour (on ? Theme::ink : Theme::faint);
    g.setFont (Theme::sans (12.f));
    g.drawText (getButtonText(), b.withTrimmedRight (42.f), juce::Justification::centredLeft);

    auto sw = b.removeFromRight (34.f)
                 .withSizeKeepingCentre (34.f, 18.f);

    g.setColour (on ? Theme::blue : (hover ? Theme::hairline.darker (0.15f) : Theme::hairline));
    g.fillRoundedRectangle (sw, 9.f);
    if (! on)
    {
        g.setColour (Theme::hairline.darker (0.2f));
        g.drawRoundedRectangle (sw, 9.f, 1.f);
    }

    const float kx = on ? sw.getRight() - 9.f : sw.getX() + 9.f;
    const float ky = sw.getCentreY();
    g.setColour (Theme::panel);
    g.fillEllipse (kx - 7.f, ky - 7.f, 14.f, 14.f);
    g.setColour (Theme::hairline.darker (0.25f));
    g.drawEllipse (kx - 7.f, ky - 7.f, 14.f, 14.f, 1.f);
}

//==============================================================================
SegmentedControl::SegmentedControl (juce::AudioProcessorValueTreeState& state,
                                    const juce::String& id,
                                    juce::StringArray itemsToUse)
    : apvts (state), paramID (id), items (std::move (itemsToUse))
{
    param = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (paramID));
    if (param != nullptr)
        selected = param->getIndex();
    apvts.addParameterListener (paramID, this);
    setWantsKeyboardFocus (false);
}

SegmentedControl::~SegmentedControl()
{
    apvts.removeParameterListener (paramID, this);
}

void SegmentedControl::parameterChanged (const juce::String&, float newValue)
{
    selected = juce::jlimit (0, (int) items.size() - 1,
                             (int) std::lround (newValue * (float) (items.size() - 1)));
    repaint();
}

void SegmentedControl::mouseDown (const juce::MouseEvent& e)
{
    const int idx = juce::jlimit (0, (int) items.size() - 1,
                                  (int) (e.x / (getWidth() / (double) items.size())));
    if (param != nullptr)
        param->setValueNotifyingHost (param->convertTo0to1 ((float) idx));
}

void SegmentedControl::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float segW = b.getWidth() / (float) items.size();

    g.setColour (Theme::panel);
    g.fillRoundedRectangle (b, 6.f);
    g.setColour (Theme::hairline);
    g.drawRoundedRectangle (b, 6.f, 1.f);

    g.setFont (Theme::mono (11.f));
    for (int i = 0; i < (int) items.size(); ++i)
    {
        auto r = b.withX (b.getX() + segW * (float) i).withWidth (segW);
        if (i == selected)
        {
            auto sr = r.reduced (2.f);
            g.setColour (Theme::blue);
            g.fillRoundedRectangle (sr, 4.f);
            g.setColour (Theme::panel);
        }
        else
        {
            g.setColour (isMouseOver() ? Theme::ink : Theme::faint);
        }
        g.drawText (items[(size_t) i], r, juce::Justification::centred);

        if (i > 0)
        {
            g.setColour (Theme::hairline);
            g.drawLine (r.getX(), b.getY() + 5.f, r.getX(), b.getBottom() - 5.f, 1.f);
        }
    }
}

//==============================================================================
FlatKnob::FlatKnob()
{
    setSliderStyle (juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, 0, 0, 0);
    setRotaryParameters ({ juce::MathConstants<float>::pi * 0.75f,
                           juce::MathConstants<float>::pi * 2.25f, true });
    setDoubleClickReturnValue (true, 120.0);
    setRange (40.0, 300.0, 0.1);
}

void FlatKnob::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float cx = b.getCentreX();
    const float cy = b.getCentreY() - 3.f;
    const float r  = std::min (b.getWidth(), b.getHeight()) * 0.5f - 9.f;

    const double n = (getValue() - getMinimum()) / (getMaximum() - getMinimum());
    const float start = juce::MathConstants<float>::pi * 0.75f;
    const float end   = juce::MathConstants<float>::pi * 2.25f;
    const float aVal  = start + (float) n * (end - start);

    // value arc (track + filled portion), flat hairline style
    juce::Path track, val;
    track.addCentredArc (cx, cy, r, r, 0.f, start, end, true);
    val.addCentredArc   (cx, cy, r, r, 0.f, start, aVal, true);
    g.setColour (Theme::hairline);
    g.strokePath (track, juce::PathStrokeType (3.f));
    g.setColour (Theme::blue);
    g.strokePath (val, juce::PathStrokeType (3.f));

    // knob body
    g.setColour (Theme::panel);
    g.fillEllipse (cx - r + 5.f, cy - r + 5.f, (r - 5.f) * 2.f, (r - 5.f) * 2.f);
    g.setColour (Theme::hairline.darker (0.3f));
    g.drawEllipse (cx - r + 5.f, cy - r + 5.f, (r - 5.f) * 2.f, (r - 5.f) * 2.f, 1.f);

    // indicator tick
    const float ix = cx + (r - 11.f) * std::cos (aVal);
    const float iy = cy + (r - 11.f) * std::sin (aVal);
    g.setColour (Theme::ink);
    g.drawLine (cx, cy, ix, iy, 2.f);

    // value readout (monospace)
    g.setFont (Theme::mono (10.f));
    g.setColour (Theme::faint);
    g.drawText (juce::String (getValue(), 1), b.withY (b.getBottom() - 13.f).withHeight (12.f),
                juce::Justification::centred);
}

//==============================================================================
LcdReadout::LcdReadout (VelocityEnvelopeProcessor& p) : proc (p)
{
    startTimerHz (20);
}

LcdReadout::~LcdReadout() { stopTimer(); }

void LcdReadout::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    g.setColour (Theme::lcdBg);
    g.fillRoundedRectangle (b, 5.f);
    g.setColour (Theme::hairline.darker (0.4f));
    g.drawRoundedRectangle (b, 5.f, 1.f);

    const float bpm = proc.displayBpm.load();
    double phase = proc.displayPhase.load();
    const double total = proc.getEnvelope().totalBeats();
    if (total > 0.0 && proc.loopParam != nullptr && proc.loopParam->get())
        phase = std::fmod (phase, total);
    const bool playing = proc.displayPlaying.load();

    g.setFont (Theme::mono (11.f));
    g.setColour (Theme::lcdText);
    g.drawText (juce::String (playing ? "RUN " : "STOP") + "  " +
                juce::String (bpm, 1) + " BPM   PH " +
                juce::String (phase, 2) + " / " + juce::String (total, 1),
                b.reduced (8.f, 0.f), juce::Justification::centredLeft);
}
