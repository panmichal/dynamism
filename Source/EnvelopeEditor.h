#pragma once

#include <JuceHeader.h>
#include "EnvelopeModel.h"
#include "PluginProcessor.h"

// Interactive multi-segment envelope editor with two modes:
//
//  EDIT mode (default)
//   - click empty space: add a node
//   - drag a node: move it (x snapped to grid unless the node is "free")
//   - drag the last node: change total envelope length
//   - right-click / double-click / Delete: remove an inner node
//
//  DRAW mode (toggle button or key "B", like Ableton's automation draw mode)
//   - press and drag: paints the envelope as a straight line from the stroke
//     start to the cursor, replacing nodes in the traversed range
//   - dragging vertically (no horizontal movement) inserts a zero-width
//     segment = a vertical step bar
//
//  Snap: global "Snap" toggle + grid selector apply to everything; hold
//  ALT while adding/dragging to make just that node free (diamond shape),
//  hold SHIFT to force-snap a free node back to the grid.
class EnvelopeComponent : public juce::Component,
                          private juce::Timer
{
public:
    explicit EnvelopeComponent (VelocityEnvelopeProcessor& p);
    ~EnvelopeComponent() override;

    void paint (juce::Graphics&) override;
    void refreshFromProcessor();

    void setDrawMode (bool d) { drawMode = d; strokeActive = false; repaint(); }
    bool getDrawMode() const { return drawMode; }
    std::function<void (bool)> onDrawModeChanged;

    EnvelopeModel model;

private:
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void timerCallback() override { repaint(); }

    // geometry
    juce::Rectangle<int> area() const;
    double viewBeats() const;
    double xToBeats (int x) const;
    int    beatsToX (double b) const;
    float  yToValue (int y) const;
    int    valueToY (float v) const;
    double gridBeats() const { return proc.gridBeats(); }
    bool   globalSnapOn() const { return proc.snapParam == nullptr || proc.snapParam->get(); }
    double snap (double b) const { return EnvelopeModel::snapBeat (b, gridBeats()); }
    double applySnap (double rawBeat, bool nodeSnapped, bool alt, bool shift) const;
    int    nodeAt (juce::Point<int> p) const;

    // model edits
    int  insertNode (double beat, float value, bool snapped); // replace if a node sits at `beat`
    int  appendStepNode (double beat, float value, bool snapped); // extra node at same beat (vertical bar)
    void removeNodesBetween (double b0, double b1); // exclusive, keeps first/last node
    void beginStroke (juce::Point<int> p, bool alt, bool shift);
    void continueStroke (juce::Point<int> p, bool alt, bool shift);
    void deleteNode (int idx);
    void commit();

    VelocityEnvelopeProcessor& proc;

    bool   drawMode = false;
    bool   strokeActive = false;
    double lastBeat = 0.0, lastValue = 0.0;
    int    draggedIndex = -1;
    int    selectedIndex = -1;

    static constexpr double maxBeats = 64.0; // 16 bars
    static constexpr double eps    = 1e-6;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeComponent)
};
