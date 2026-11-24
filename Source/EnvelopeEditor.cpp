#include "EnvelopeEditor.h"
#include "CustomWidgets.h"

namespace
{
juce::String noteNameForBeats (double beats)
{
    static const std::pair<double, const char*> table[] = {
        { 4.0,       "1/1"   }, { 3.0,       "1/2."  }, { 2.0,       "1/2"  },
        { 1.5,       "1/4."  }, { 1.0,       "1/4"   }, { 2.0 / 3.0, "1/8T" },
        { 0.75,      "1/8."  }, { 0.5,       "1/8"   }, { 1.0 / 3.0, "1/16T"},
        { 0.375,     "1/16." }, { 0.25,      "1/16"  }, { 0.125,     "1/32" },
        { 0.0625,    "1/64"  }
    };
    for (auto& e : table)
        if (std::abs (e.first - beats) < 1e-6)
            return e.second;
    return juce::String (beats, 3) + "bt";
}

const juce::Colour& accent = Theme::blue;   // envelope line
const juce::Colour& cyan   = Theme::orange; // "free" (unsnapped) nodes
}

EnvelopeComponent::EnvelopeComponent (VelocityEnvelopeProcessor& p) : proc (p)
{
    model = proc.getEnvelope();
    setWantsKeyboardFocus (true);
    startTimerHz (30);
}

EnvelopeComponent::~EnvelopeComponent() { stopTimer(); }

void EnvelopeComponent::refreshFromProcessor()
{
    model = proc.getEnvelope();
    repaint();
}

//==============================================================================
juce::Rectangle<int> EnvelopeComponent::area() const
{
    auto r = getLocalBounds();
    r.removeFromLeft (20);
    r.removeFromRight (14);
    r.removeFromTop (24);
    r.removeFromBottom (30);
    return r;
}

double EnvelopeComponent::viewBeats() const
{
    const double grid  = gridBeats();
    const double total = model.totalBeats();
    const double v = std::max (8.0, total + grid);
    return std::ceil (v / 4.0) * 4.0; // whole bars, at least 2
}

double EnvelopeComponent::xToBeats (int x) const
{
    auto a = area();
    return (double) (x - a.getX()) / (double) a.getWidth() * viewBeats();
}

int EnvelopeComponent::beatsToX (double b) const
{
    auto a = area();
    return a.getX() + (int) std::lround (b / viewBeats() * (double) a.getWidth());
}

float EnvelopeComponent::yToValue (int y) const
{
    auto a = area();
    return juce::jlimit (0.f, 1.f, 1.f - (float) (y - a.getY()) / (float) a.getHeight());
}

int EnvelopeComponent::valueToY (float v) const
{
    auto a = area();
    return a.getY() + (int) std::lround ((1.f - v) * (float) a.getHeight());
}

double EnvelopeComponent::applySnap (double rawBeat, bool nodeSnapped, bool alt, bool shift) const
{
    if (shift) return snap (rawBeat);                    // force snap
    if (alt)   return rawBeat;                           // free
    if (globalSnapOn() && nodeSnapped) return snap (rawBeat);
    return rawBeat;
}

int EnvelopeComponent::nodeAt (juce::Point<int> p) const
{
    for (int i = (int) model.nodes.size() - 1; i >= 0; --i)
    {
        const juce::Point<float> c ((float) beatsToX (model.nodes[(size_t) i].beats),
                                    (float) valueToY (model.nodes[(size_t) i].value));
        if (p.toFloat().getDistanceFrom (c) <= 9.f)
            return i;
    }
    return -1;
}

//==============================================================================
int EnvelopeComponent::insertNode (double beat, float value, bool snapped)
{
    for (size_t i = 0; i < model.nodes.size(); ++i)
    {
        if (std::abs (model.nodes[i].beats - beat) < eps)
        {
            model.nodes[i].value   = value;
            model.nodes[i].snapped = snapped;
            return (int) i;
        }
    }
    auto it = std::lower_bound (model.nodes.begin(), model.nodes.end(), beat,
                                [] (const EnvNode& a, double v) { return a.beats < v; });
    const int idx = (int) (it - model.nodes.begin());
    model.nodes.insert (it, EnvNode { beat, value, snapped });
    return idx;
}

int EnvelopeComponent::appendStepNode (double beat, float value, bool snapped)
{
    // insert right after the last node with beats <= beat (allows duplicate x = vertical bar)
    int idx = 0;
    for (size_t i = 0; i < model.nodes.size(); ++i)
        if (model.nodes[i].beats <= beat + eps)
            idx = (int) i + 1;
    model.nodes.insert (model.nodes.begin() + idx, EnvNode { beat, value, snapped });
    return idx;
}

void EnvelopeComponent::removeNodesBetween (double b0, double b1)
{
    const double lo = std::min (b0, b1), hi = std::max (b0, b1);
    for (int i = (int) model.nodes.size() - 2; i > 0; --i) // keep first and last node
        if (model.nodes[(size_t) i].beats > lo + eps && model.nodes[(size_t) i].beats < hi - eps)
            model.nodes.erase (model.nodes.begin() + i);
}

void EnvelopeComponent::deleteNode (int idx)
{
    if (idx > 0 && idx < (int) model.nodes.size() - 1)
    {
        model.nodes.erase (model.nodes.begin() + idx);
        if (selectedIndex == idx) selectedIndex = -1;
        commit();
        repaint();
    }
}

void EnvelopeComponent::commit()
{
    proc.setEnvelope (model);
}

//==============================================================================
void EnvelopeComponent::beginStroke (juce::Point<int> p, bool alt, bool shift)
{
    double b = juce::jlimit (0.0, maxBeats, applySnap (xToBeats (p.x), true, alt, shift));
    const float v = yToValue (p.y);
    const bool sflag = shift ? true : (alt ? false : globalSnapOn());
    const double total = model.totalBeats();

    if (b >= total - eps)
    {
        // extend / move the end node
        b = std::max (b, model.nodes[model.nodes.size() - 2].beats);
        model.nodes.back().beats   = b;
        model.nodes.back().value   = v;
        model.nodes.back().snapped = sflag;
    }
    else if (b < eps)
    {
        model.nodes.front().value = v;
    }
    else
    {
        insertNode (b, v, sflag);
    }

    lastBeat   = (b >= total - eps) ? model.nodes.back().beats : b;
    lastValue  = v;
    strokeActive = true;
    commit();
    repaint();
}

void EnvelopeComponent::continueStroke (juce::Point<int> p, bool alt, bool shift)
{
    if (! strokeActive) return;

    double b = juce::jlimit (0.0, maxBeats, applySnap (xToBeats (p.x), true, alt, shift));
    const float v = yToValue (p.y);
    const bool sflag = shift ? true : (alt ? false : globalSnapOn());
    const double total = model.totalBeats();

    if (b >= total - eps)
    {
        b = std::max (b, model.nodes[model.nodes.size() - 2].beats);
        model.nodes.back().beats   = b;
        model.nodes.back().value   = v;
        model.nodes.back().snapped = sflag;
    }
    else if (std::abs (b - lastBeat) < eps)
    {
        // vertical drag -> step bar (zero-width segment)
        if (std::abs (v - lastValue) > 1e-6)
            appendStepNode (lastBeat, v, sflag);
    }
    else if (b > lastBeat)
    {
        removeNodesBetween (lastBeat, b);
        if (b < eps) model.nodes.front().value = v;
        else         insertNode (b, v, sflag);
    }
    else // b < lastBeat: drawing back to the left
    {
        removeNodesBetween (b, lastBeat);
        if (b < eps) model.nodes.front().value = v;
        else         insertNode (b, v, sflag);
    }

    lastBeat  = b;
    lastValue = v;
    commit();
    repaint();
}

//==============================================================================
void EnvelopeComponent::mouseDown (const juce::MouseEvent& e)
{
    if (! area().contains (e.getPosition()))
        return;

    const auto p = e.getPosition();
    const bool alt = e.mods.isAltDown(), shift = e.mods.isShiftDown();
    const int idx = nodeAt (p);

    if (e.mods.isPopupMenu()) // right-click: delete inner node
    {
        deleteNode (idx);
        return;
    }

    if (drawMode)
    {
        draggedIndex = -1;
        beginStroke (p, alt, shift);
        grabKeyboardFocus();
        return;
    }

    if (idx >= 0)
    {
        draggedIndex  = idx;
        selectedIndex = idx;
        if (alt)   model.nodes[(size_t) idx].snapped = false;
        if (shift) model.nodes[(size_t) idx].snapped = true;
    }
    else
    {
        double b = juce::jlimit (0.0, maxBeats, applySnap (xToBeats (p.x), globalSnapOn(), alt, shift));
        const bool sflag = shift ? true : (alt ? false : globalSnapOn());
        const double total = model.totalBeats();

        if (b >= total - eps)
        {
            b = std::max (b, model.nodes[model.nodes.size() - 2].beats);
            model.nodes.back().beats   = b;
            model.nodes.back().snapped = sflag;
            draggedIndex = (int) model.nodes.size() - 1;
        }
        else if (b > eps)
        {
            draggedIndex = insertNode (b, yToValue (p.y), sflag);
        }
        else
        {
            return;
        }
        selectedIndex = draggedIndex;
        commit();
    }
    grabKeyboardFocus();
    repaint();
}

void EnvelopeComponent::mouseDrag (const juce::MouseEvent& e)
{
    const auto p = e.getPosition();
    const bool alt = e.mods.isAltDown(), shift = e.mods.isShiftDown();

    if (drawMode)
    {
        continueStroke (p, alt, shift);
        return;
    }

    if (draggedIndex < 0 || draggedIndex >= (int) model.nodes.size())
        return;

    double b = applySnap (xToBeats (p.x), model.nodes[(size_t) draggedIndex].snapped, alt, shift);
    float  v = juce::jlimit (0.f, 1.f, yToValue (p.y));

    auto& n = model.nodes[(size_t) draggedIndex];
    if (alt)   n.snapped = false;
    if (shift) n.snapped = true;

    if (draggedIndex == 0)
    {
        b = 0.0; // first node stays at beat 0
    }
    else
    {
        const double lo = model.nodes[(size_t) draggedIndex - 1].beats;
        const double hi = (draggedIndex == (int) model.nodes.size() - 1)
                              ? maxBeats
                              : model.nodes[(size_t) draggedIndex + 1].beats;
        b = juce::jlimit (lo, hi, b); // equality allowed -> vertical step bar
    }

    n.beats = b;
    n.value = v;
    commit();
    repaint();
}

void EnvelopeComponent::mouseUp (const juce::MouseEvent&)
{
    draggedIndex = -1;
    strokeActive = false;
    commit();
    repaint();
}

void EnvelopeComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    deleteNode (nodeAt (e.getPosition()));
}

bool EnvelopeComponent::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::backspaceKey || key == juce::KeyPress::deleteKey)
    {
        deleteNode (selectedIndex);
        return true;
    }
    if (key == juce::KeyPress ('b'))
    {
        setDrawMode (! drawMode);
        if (onDrawModeChanged) onDrawModeChanged (drawMode);
        return true;
    }
    if (key == juce::KeyPress ('s') && proc.snapParam != nullptr)
    {
        proc.snapParam->setValueNotifyingHost (proc.snapParam->convertTo0to1 (! proc.snapParam->get()));
        return true;
    }
    return false;
}

//==============================================================================
void EnvelopeComponent::paint (juce::Graphics& g)
{
    // flat paper background
    g.fillAll (Theme::paper);

    auto a = area().toFloat();
    const double vb   = viewBeats();
    const double grid = gridBeats();

    // panel: lighter paper with a hairline border
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (a, 6.f);
    g.setColour (Theme::hairline);
    g.drawRoundedRectangle (a, 6.f, 1.f);

    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip;
        clip.addRoundedRectangle (a, 6.f);
        g.reduceClipRegion (clip);

        // grid + bar numbers
        for (double b = 0.0; b <= vb + 1e-9; b += grid)
        {
            const float x = (float) beatsToX (b);
            const bool isBar = std::abs (std::fmod (b, 4.0)) < 1e-9;
            g.setColour (isBar ? Theme::gridBar : Theme::grid);
            g.drawLine (x, a.getY(), x, a.getBottom(), 1.f);
            if (isBar)
            {
                g.setColour (Theme::faint);
                g.setFont (Theme::mono (10.f));
                g.drawText ("bar " + juce::String ((int) (b / 4.0) + 1),
                            (int) x + 4, (int) a.getY() + 3, 60, 13, juce::Justification::left);
            }
        }

        // mid line
        g.setColour (Theme::hairline);
        g.drawLine (a.getX(), (float) valueToY (0.5f), a.getRight(), (float) valueToY (0.5f), 1.f);

        // envelope
        if (model.nodes.size() >= 2)
        {
            juce::Path line, fill;
            fill.startNewSubPath ((float) beatsToX (model.nodes.front().beats), a.getBottom());
            for (size_t i = 0; i < model.nodes.size(); ++i)
            {
                const float x = (float) beatsToX (model.nodes[i].beats);
                const float y = (float) valueToY (model.nodes[i].value);
                fill.lineTo (x, y);
                if (i == 0) line.startNewSubPath (x, y);
                else        line.lineTo (x, y);
            }
            fill.lineTo ((float) beatsToX (model.nodes.back().beats), a.getBottom());
            fill.closeSubPath();

            // flat fill under the curve + flat line
            g.setColour (accent.withAlpha (0.13f));
            g.fillPath (fill);
            g.setColour (accent);
            g.strokePath (line, juce::PathStrokeType (2.f));

            // segment length labels (orange + "~" when the segment is free / unsnapped)
            g.setFont (Theme::mono (10.f));
            for (size_t i = 0; i + 1 < model.nodes.size(); ++i)
            {
                const double b0 = model.nodes[i].beats, b1 = model.nodes[i + 1].beats;
                const bool freeSeg = ! model.nodes[i].snapped || ! model.nodes[i + 1].snapped;
                const int x0 = beatsToX (b0), x1 = beatsToX (b1);
                g.setColour (freeSeg ? cyan : Theme::faint);
                g.drawText ((freeSeg ? "~ " : "") + noteNameForBeats (b1 - b0),
                            (x0 + x1) / 2 - 32, (int) a.getBottom() - 17, 64, 13,
                            juce::Justification::centred);
            }

            // nodes: flat filled dots (snapped) / orange squares (free) / red when hot
            for (size_t i = 0; i < model.nodes.size(); ++i)
            {
                const auto& n = model.nodes[i];
                const juce::Point<float> c ((float) beatsToX (n.beats), (float) valueToY (n.value));
                const bool isHot = ((int) i == draggedIndex || (int) i == selectedIndex);

                if (n.snapped)
                {
                    g.setColour (isHot ? Theme::danger : accent);
                    g.fillEllipse (c.x - 5.f, c.y - 5.f, 10.f, 10.f);
                    g.setColour (Theme::panel);
                    g.fillEllipse (c.x - 1.8f, c.y - 1.8f, 3.6f, 3.6f);
                }
                else
                {
                    g.setColour (isHot ? Theme::danger : cyan);
                    g.drawRect (juce::Rectangle<float> (c.x - 5.f, c.y - 5.f, 10.f, 10.f), 1.5f);
                }
            }
        }

        // phase marker: flat red line + small triangle
        const double total = model.totalBeats();
        if (total > 0.0)
        {
            double ph = proc.displayPhase.load();
            if (proc.loopParam != nullptr && proc.loopParam->get())
                ph = std::fmod (ph, total);
            else
                ph = std::min (ph, total);

            const float x = (float) beatsToX (ph);
            if (x >= a.getX() && x <= a.getRight())
            {
                g.setColour (Theme::danger);
                g.drawLine (x, a.getY(), x, a.getBottom(), 1.5f);
                juce::Path tri;
                tri.startNewSubPath (x - 4.5f, a.getY());
                tri.lineTo (x + 4.5f, a.getY());
                tri.lineTo (x, a.getY() + 6.f);
                tri.closeSubPath();
                g.fillPath (tri);
            }
        }
    }

    // hint line (monospace, like the reference UI)
    g.setColour (Theme::faint);
    g.setFont (Theme::mono (10.f));
    const juce::String mode = drawMode ? "DRAW" : "EDIT";
    g.drawText ("[" + mode + "]  " +
                juce::String (drawMode ? "drag: paint envelope (vertical drag = step bar)"
                                      : "click: add node   drag: move node") +
                "    alt: no snap    shift: force snap    right-click / double-click / del: delete    B: draw    S: snap",
                (int) a.getX(), (int) a.getBottom() + 5, (int) a.getWidth(), 15, juce::Justification::left);
}
