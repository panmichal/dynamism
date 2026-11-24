#pragma once

#include <JuceHeader.h>
#include "EnvelopeModel.h"

// Audio effect: multiplies the audio amplitude by a user-drawn, multi-segment
// envelope at the current free-running phase (like drawing a velocity curve,
// but applied to the sound itself). The envelope runs in beats (quarter
// notes); segment lengths snap to a selectable note grid (1/1 .. 1/32).
// The phase is driven by the host playhead when available (resyncing on
// transport restart) and otherwise free-runs in real time at the current /
// fallback BPM.
class VelocityEnvelopeProcessor : public juce::AudioProcessor
{
public:
    VelocityEnvelopeProcessor();
    ~VelocityEnvelopeProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        const auto& mainOut = layouts.getMainOutputChannelSet();
        return mainOut.isDisabled() || mainOut == juce::AudioChannelSet::mono()
            || mainOut == juce::AudioChannelSet::stereo();
    }
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Velocity Envelope"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void reset() override;

    // ---- envelope <-> editor ----
    void setEnvelope (const EnvelopeModel& m);
    EnvelopeModel getEnvelope() const;
    void resetEnvelopeToDefault();

    // ---- parameters ----
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioParameterBool*   enabledParam = nullptr;
    juce::AudioParameterChoice* gridParam    = nullptr;
    juce::AudioParameterBool*   snapParam    = nullptr;
    juce::AudioParameterBool*   loopParam    = nullptr;
    juce::AudioParameterFloat*  bpmParam     = nullptr;

    double gridBeats() const;
    float envelopeValueAtPhase (double phaseBeats) const;

    // ---- live display state for the editor ----
    std::atomic<double> displayPhase  { 0.0 };
    std::atomic<float>  displayBpm    { 120.f };
    std::atomic<bool>   displayPlaying { false };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void loadEnvelopeFromState (const juce::ValueTree& state);

    mutable std::mutex envMutex;
    EnvelopeModel envelope { EnvelopeModel::defaultModel() };

    double phaseBeats   = 0.0;
    double lastPpq      = -1.0;
    double lastKnownBpm = 120.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VelocityEnvelopeProcessor)
};
