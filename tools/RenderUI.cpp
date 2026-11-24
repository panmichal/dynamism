// Dev tool: render the plugin editor offscreen to a PNG (no host needed).
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;

    VelocityEnvelopeProcessor proc;
    proc.prepareToPlay (44100.0, 512);

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setSize (940, 540);

    juce::Image img (juce::Image::ARGB, 940, 540, true);
    juce::Graphics g (img);
    ed->paintEntireComponent (g, true);

    juce::File out (argc > 1 ? juce::String::fromUTF8 (argv[1])
                             : juce::File ("/tmp/velocity_envelope_ui.png"));
    auto stream = out.createOutputStream();
    juce::PNGImageFormat png;
    const bool ok = stream != nullptr && png.writeImageToStream (img, *stream);
    juce::Logger::writeToLog (ok ? "wrote /tmp/velocity_envelope_ui.png" : "FAILED to write png");
    return ok ? 0 : 1;
}
