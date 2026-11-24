#include "PluginProcessor.h"
#include "PluginEditor.h"

VelocityEnvelopeProcessor::VelocityEnvelopeProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Audio In",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Audio Out", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    enabledParam = dynamic_cast<juce::AudioParameterBool*>   (apvts.getParameter ("enabled"));
    gridParam    = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("grid"));
    snapParam    = dynamic_cast<juce::AudioParameterBool*>   (apvts.getParameter ("snap"));
    loopParam    = dynamic_cast<juce::AudioParameterBool*>   (apvts.getParameter ("loop"));
    bpmParam     = dynamic_cast<juce::AudioParameterFloat*>  (apvts.getParameter ("bpm"));

    setEnvelope (envelope); // write default shape into the state tree so it is saved with presets
}

juce::AudioProcessorValueTreeState::ParameterLayout VelocityEnvelopeProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "enabled", 1 }, "Enabled", true));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "grid", 1 }, "Grid",
        juce::StringArray { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" }, 2)); // default 1/4

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "snap", 1 }, "Snap", true));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "loop", 1 }, "Loop", true));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "bpm", 1 }, "BPM",
        juce::NormalisableRange<float> (40.f, 300.f, 0.1f), 120.f));

    return layout;
}

//==============================================================================
void VelocityEnvelopeProcessor::prepareToPlay (double sampleRate, int)
{
    phaseBeats = 0.0;
    lastPpq    = -1.0;
    lastKnownBpm = bpmParam != nullptr ? (double) *bpmParam : 120.0;
    juce::ignoreUnused (sampleRate);
}

void VelocityEnvelopeProcessor::reset()
{
    phaseBeats = 0.0;
    lastPpq    = -1.0;
}

//==============================================================================
double VelocityEnvelopeProcessor::gridBeats() const
{
    return EnvelopeModel::gridBeatsForIndex (gridParam != nullptr ? gridParam->getIndex() : 2);
}

float VelocityEnvelopeProcessor::envelopeValueAtPhase (double phaseBeats) const
{
    std::lock_guard<std::mutex> lock (envMutex);

    const double total = envelope.totalBeats();
    if (total <= 0.0) return 0.f;

    if (loopParam != nullptr && loopParam->get())
        phaseBeats = std::fmod (phaseBeats, total);
    else
        phaseBeats = std::min (phaseBeats, total);

    return envelope.valueAt (phaseBeats);
}

//==============================================================================
void VelocityEnvelopeProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
    const int numSamples = buffer.getNumSamples();

    // ---- advance the free-running envelope phase (in beats) ----
    juce::AudioPlayHead* playHead = getPlayHead();
    auto pos = playHead != nullptr ? playHead->getPosition()
                                   : juce::Optional<juce::AudioPlayHead::PositionInfo>();
    const bool hasInfo = pos.hasValue();
    const bool playing = hasInfo && pos->getIsPlaying();

    if (hasInfo && pos->getBpm().hasValue() && *pos->getBpm() > 0.0)
        lastKnownBpm = *pos->getBpm();
    else if (bpmParam != nullptr)
        lastKnownBpm = (double) *bpmParam;

    if (playing && hasInfo && pos->getPpqPosition().hasValue() && *pos->getPpqPosition() >= 0.0)
    {
        const double ppq = *pos->getPpqPosition(); // quarter notes == beats
        if (lastPpq >= 0.0 && ppq < lastPpq - 0.25)
            phaseBeats = ppq;                      // transport jumped back -> resync
        else
            phaseBeats = ppq;
        lastPpq = ppq;
    }
    else
    {
        lastPpq = -1.0; // free-run in real time (advanced per-sample below)
    }

    displayPhase   = phaseBeats;
    displayBpm     = (float) lastKnownBpm;
    displayPlaying = playing;

    juce::ignoreUnused (midiMessages); // pure audio effect: MIDI passes through

    if (enabledParam != nullptr && ! enabledParam->get())
        return; // bypassed: audio passes through untouched

    // ---- shape the audio amplitude with the envelope, sample-accurately ----
    const double beatsPerSample = lastKnownBpm / 60.0 / sr;
    for (int s = 0; s < numSamples; ++s)
    {
        const float g = envelopeValueAtPhase (phaseBeats);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.getWritePointer (ch)[s] *= g;
        phaseBeats += beatsPerSample;
    }
}

//==============================================================================
void VelocityEnvelopeProcessor::setEnvelope (const EnvelopeModel& m)
{
    {
        std::lock_guard<std::mutex> lock (envMutex);
        envelope = m;
    }

    auto state = apvts.copyState();
    auto env = state.getOrCreateChildWithName ("ENVELOPE", nullptr);
    env.removeAllChildren (nullptr);
    for (const auto& n : m.nodes)
    {
        juce::ValueTree c ("NODE");
        c.setProperty ("b", n.beats, nullptr);
        c.setProperty ("v", (double) n.value, nullptr);
        c.setProperty ("s", n.snapped ? 1 : 0, nullptr);
        env.addChild (c, -1, nullptr);
    }
    apvts.replaceState (state);
}

EnvelopeModel VelocityEnvelopeProcessor::getEnvelope() const
{
    std::lock_guard<std::mutex> lock (envMutex);
    return envelope;
}

void VelocityEnvelopeProcessor::resetEnvelopeToDefault()
{
    setEnvelope (EnvelopeModel::defaultModel());
}

void VelocityEnvelopeProcessor::loadEnvelopeFromState (const juce::ValueTree& state)
{
    auto env = state.getChildWithName ("ENVELOPE");
    if (! env.isValid()) return;

    EnvelopeModel m;
    for (auto child : env)
    {
        EnvNode n;
        n.beats   = (double) child.getProperty ("b");
        n.value   = juce::jlimit (0.f, 1.f, (float) (double) child.getProperty ("v"));
        n.snapped = (int) child.getProperty ("s", juce::var (1)) != 0;
        m.nodes.push_back (n);
    }

    if (m.nodes.size() >= 2)
    {
        std::sort (m.nodes.begin(), m.nodes.end(),
                   [] (const EnvNode& a, const EnvNode& b) { return a.beats < b.beats; });
        std::lock_guard<std::mutex> lock (envMutex);
        envelope = m;
    }
}

void VelocityEnvelopeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState(); // includes the ENVELOPE child tree
    auto xml = state.createXml();
    copyXmlToBinary (*xml, destData);
}

void VelocityEnvelopeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr) return;

    auto vt = juce::ValueTree::fromXml (*xml);
    if (! vt.isValid()) return;

    apvts.replaceState (vt);
    loadEnvelopeFromState (vt);
}

//==============================================================================
juce::AudioProcessorEditor* VelocityEnvelopeProcessor::createEditor()
{
    return new VelocityEnvelopeEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VelocityEnvelopeProcessor();
}
