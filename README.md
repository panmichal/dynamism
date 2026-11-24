# Velocity Envelope

An AU + VST3 **audio effect** for macOS / Ableton Live that shapes the
amplitude of the incoming sound with a drawn, multi-segment envelope — think
of it as drawing a "velocity curve" and applying it to the audio itself
(a synced volume gate / tremolo-like shaper).

- The envelope runs **free-running in beats** (quarter notes): it follows the
  host playhead while playing (and resyncs when the transport restarts),
  otherwise advances in real time using the BPM knob.
- Segments **snap to a note grid** (1/1, 1/2, 1/4, 1/8, 1/16, 1/32); each
  segment can have a different length (half note, quarter, ...).
- The envelope **loops** over its total length (Loop switch to disable and
  hold the last value).
- Editor modes:
  - **EDIT**: click empty space to add a node, drag nodes to move them, drag
    the last node to change the total length, right-click / double-click /
    Delete to remove an inner node.
  - **DRAW** (switch "Draw (B)" or key **B**, like Ableton's automation draw
    mode): press and drag to paint the envelope as a straight line; dragging
    vertically inserts a **vertical step bar**.
- **Snap**: the "Snap (S)" switch + grid selector snap everything. Hold
  **Alt** while adding/dragging a node to make just that part of the envelope
  free (orange square nodes, "~" labels); hold **Shift** to force-snap it
  back. Per-node snap state is saved with the preset.
- Red line = current phase. "Enabled" bypasses (audio passes through).
- Flat "paper/analog" UI with custom widgets: toggle switches, segmented grid
  selector, rotary BPM knob with value arc, dark monospace LCD readout
  (transport state, BPM, phase).

## Build (macOS)

Requirements: Xcode Command Line Tools, CMake >= 3.22 (e.g. `brew install cmake`).

```sh
git clone <this-repo-url> VelocityEnvelope
cd VelocityEnvelope
git clone --depth 1 https://github.com/juce-framework/JUCE.git JUCE   # tested with JUCE 9.0.6 (commit 654bdbc)

cmake -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target VelocityEnvelope_All -j "$(sysctl -n hw.ncpu)"
```

Artefacts land in `build/VelocityEnvelope_artefacts/Release/`:
`VST3/Velocity Envelope.vst3` and `AU/Velocity Envelope.component`.

## Install into Ableton Live

```sh
mkdir -p ~/Library/Audio/Plug-Ins/VST3 ~/Library/Audio/Plug-Ins/Components
cp -R "build/VelocityEnvelope_artefacts/Release/VST3/Velocity Envelope.vst3" ~/Library/Audio/Plug-Ins/VST3/
cp -R "build/VelocityEnvelope_artefacts/Release/AU/Velocity Envelope.component" ~/Library/Audio/Plug-Ins/Components/
```

Then in Live: Options > Preferences > Plug-Ins > rescan (or restart Live).
It shows up under **Audio Effects** ("Velocity Envelope") — the AU (type `aufx`)
and the VST3 (category Fx/Dynamics). Drop it on any track's audio-effect slot.
The envelope and all settings save with the Live set.

Validate the AU from the command line:

```sh
auval -v aufx DyVE Dynm
```

## Dev tool: offscreen UI render

```sh
cmake --build build --config Release --target UiRender
./build/UiRender_artefacts/Release/UiRender [output.png]   # default: /tmp/velocity_envelope_ui.png
```

Renders the plugin editor to a PNG without opening a host — handy for
checking the look in CI or headless machines.

## Forking

If you fork this project, change `PLUGIN_CODE`, `PLUGIN_MANUFACTURER_CODE`,
`BUNDLE_ID` and `COMPANY_NAME` in `CMakeLists.txt` so your build doesn't
clash with this one in hosts (AU/VST3 IDs must be unique).

## Layout

```
CMakeLists.txt            JUCE plugin definition (AU + VST3 audio effect) + UiRender dev tool
Source/EnvelopeModel.h    pure-C++ multi-segment envelope model + grid snap
Source/PluginProcessor.*  audio effect: free-running phase, per-sample amplitude shaping, state
Source/EnvelopeEditor.*   drawable envelope component (draw mode, step bars, per-node snap)
Source/CustomWidgets.*    theme + custom widgets (switch, segmented control, knob, LCD)
Source/PluginEditor.*     editor window + parameter attachments
tools/RenderUI.cpp        offscreen UI renderer (dev tool)
```

## License

MIT — see [LICENSE](LICENSE).
