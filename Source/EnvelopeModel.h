#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

// One node of the envelope: a position in beats (quarter notes) from the
// envelope start, and a value in the range 0..1 (0 = silence, 1 = full level).
// `snapped` = this node's x-position is locked to the grid (per-node snap
// override; false lets just this part of the envelope float freely).
// Nodes with equal `beats` form a zero-width segment = a vertical step bar.
struct EnvNode
{
    double beats   = 0.0;
    float  value   = 0.f;
    bool   snapped = true;
};

// Multi-segment envelope. Nodes are sorted by `beats`; the first node sits at
// beat 0 and the last node defines the total envelope length. Values are
// linearly interpolated between nodes, so each segment is a straight ramp
// (like classic ADSR segments, but any number of them).
struct EnvelopeModel
{
    std::vector<EnvNode> nodes;

    double totalBeats() const { return nodes.empty() ? 0.0 : nodes.back().beats; }

    float valueAt (double phaseBeats) const
    {
        if (nodes.empty())       return 0.f;
        if (nodes.size() < 2)    return nodes.front().value;
        if (phaseBeats <= nodes.front().beats) return nodes.front().value;

        for (size_t i = 0; i + 1 < nodes.size(); ++i)
        {
            if (phaseBeats <= nodes[i + 1].beats)
            {
                const double b0 = nodes[i].beats,     b1 = nodes[i + 1].beats;
                const float  v0 = nodes[i].value,     v1 = nodes[i + 1].value;
                const double t  = (b1 > b0) ? (phaseBeats - b0) / (b1 - b0) : 0.0;
                return v0 + (float) t * (v1 - v0);
            }
        }
        return nodes.back().value;
    }

    // Default shape: up to full in 1 beat, down to 0 in 1 beat, hold at 0
    // until beat 4 (one bar) - i.e. an ADSR-like start, looped over 1 bar.
    static EnvelopeModel defaultModel()
    {
        EnvelopeModel m;
        m.nodes = { { 0.0, 0.f }, { 1.0, 1.f }, { 2.0, 0.f }, { 4.0, 0.f } };
        return m;
    }

    // Grid unit in beats for a note-value index:
    // 0 -> 1/1 (4 beats), 1 -> 1/2, 2 -> 1/4, 3 -> 1/8, 4 -> 1/16, 5 -> 1/32
    static double gridBeatsForIndex (int index)
    {
        index = std::clamp (index, 0, 5);
        return 4.0 / std::pow (2.0, (double) index);
    }

    static double snapBeat (double beats, double gridBeats)
    {
        if (gridBeats <= 0.0) return beats;
        return std::round (beats / gridBeats) * gridBeats;
    }
};
