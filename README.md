# PluginHost

`PluginHost` is a ChucK chugin that allows you to load and host external VST3, VST (Legacy), and Audio Unit (AU) plugins directly within ChucK. It includes parameter automation, MIDI routing, and graphical editor support.

## Features

- **Multi-Format Support**: Load VST3, VST (Legacy), and AU (macOS only) plugins.
- **Parameter Automation**: Access, get, and set plugin parameters by index or name.
- **MIDI Integration**: Send MIDI Note On, Note Off, Pitch Bend, Aftertouch, and Control Change messages to plugins.
- **GUI Support**: Show and hide the plugin's native graphical editor window.
- **State Management**: Save and load plugin state (presets) to/from files.
- **Transport Sync**: Synchronize plugins with ChucK's timing using a built-in playhead (BPM, time signature, position, etc.).
- **QWERTY MIDI**: Optional QWERTY keyboard window for playing plugins with your computer keyboard.
- **Synchronous/Asynchronous Modes**: Choose between simplified synchronous operations or non-blocking asynchronous events.

## Building

### Requirements

- [ChucK](https://chuck.cs.princeton.edu/) source code (for `chugin.h`).
- [CMake](https://cmake.org/) (recommended) or `make`.
- [JUCE](https://juce.com/) (included as a static library in the repository).

## Quick Start

```chuck
// Create a PluginHost instance and connect to dac
PluginHost host => dac;

// Load a VST3 plugin
host.load("/Library/Audio/Plug-Ins/VST3/YourPlugin.vst3");

// Show the plugin editor GUI
host.showEditor();

// Send a MIDI note
host.noteOn(60, 0.8);
1::second => now;
host.noteOff(60);

// Automate a parameter
0 => int paramIndex;
0.5 => host.param; // host.param(paramIndex, value)
```

## API Reference

### Loading & Metadata
- `void load(string path)`: Load a plugin from the given file path.
- `string name()`: Get the loaded plugin's name.
- `string vendor()`: Get the plugin's manufacturer name.
- `void reset()`: Reset the plugin's internal state.

### Parameters & Programs
- `int numParams()`: Total number of parameters.
- `int numNonMidiParams()`: Number of parameters excluding MIDI CC mappings.
- `float param(int index)`: Get parameter value (0.0 to 1.0).
- `float param(int index, float value)`: Set parameter value.
- `string paramName(int index)`: Get parameter name.
- `string paramLabel(int index)`: Get parameter unit label (e.g., "dB", "Hz").
- `string paramDisplay(int index)`: Get parameter value as text (e.g., "-3.0 dB").
- `int findParam(string name)`: Find parameter index by name.
- `int numPrograms()`: Get number of factory programs/presets.
- `int program()`: Get current program index.
- `void program(int index)`: Set current program index.
- `string programName(int index)`: Get name of a program.

### MIDI Input
- `void noteOn(int note, float velocity)`: Send Note On (channel 1).
- `void noteOn(int note, float velocity, int channel)`: Send Note On (channel 1-16).
- `void noteOff(int note)`: Send Note Off (channel 1).
- `void noteOff(int note, int channel)`: Send Note Off (channel 1-16).
- `void controlChange(int control, int value)`: Send CC.
- `void pitchBend(float value)`: Send Pitch Bend (-1.0 to 1.0).
- `void aftertouch(int note, float pressure)`: Polyphonic aftertouch.
- `void aftertouchChannel(float pressure)`: Channel pressure.
- `void allNotesOff()`: Send All Notes Off.
- `void midiMsg(int b1, int b2, int b3)`: Send raw 3-byte MIDI message.

### Transport & Playhead
- `float bpm(float value)` / `float bpm()`: Set/get BPM.
- `void timeSig(int num, int den)`: Set time signature.
- `float pos(float ppq)` / `float pos()`: Set/get position in pulses per quarter note.
- `int playing(int isPlaying)` / `int playing()`: Set/get play status.
- `int recording(int isRecording)` / `int recording()`: Set/get recording status.
- `int looping(int isLooping)` / `int looping()`: Set/get loop status.
- `void loopPoints(float start, float end)`: Set loop boundaries.
- `float loopStart(float ppq)` / `float loopStart()`: Set/get loop start.
- `float loopEnd(float ppq)` / `float loopEnd()`: Set/get loop end.
- `float lastBarPos(float ppq)` / `float lastBarPos()`: Set/get last bar position.

### State & GUI
- `void saveState(string path)`: Save plugin state to a file.
- `void loadState(string path)`: Load plugin state from a file.
- `void showEditor()`: Open the plugin's GUI window.
- `void hideEditor()`: Close the plugin's GUI window.
- `void toggleQWERTYMidiInput()`: Toggle the computer keyboard MIDI input window.

### Configuration
- `void forceSynchronous(int b)`: If true (default), wait for async events (like loading) to complete before returning.
- `void blockSize(int size)`: Set processing block size (default 16). Larger sizes are more efficient but introduce more latency.
- `int latency()`: Get plugin latency in samples.
- `void bypass(int b)`: Bypass/unbypass the plugin.

## Roadmap

- **MIDI Output**: Support for plugins that generate MIDI.
- **MPE (MIDI Polyphonic Expression)**: Support for expressive MIDI controllers.
- **Easier Plugin Search**: Improved workflow for locating installed plugins.
- **Full Linux Support**: Currently implemented but needs testing and validation.

## Examples

Check the `tests/` directory for comprehensive examples:
- `helloworld.ck`: Basic loading and MIDI playback.
- `param_modulation.ck`: Automating parameters from ChucK.
- `transport_sync.ck`: Synchronizing LFOs and sequencers via the playhead.
- `plugin_chain.ck`: Chaining multiple `PluginHost` instances.

### Build Instructions

1.  Ensure the `CK_SRC_PATH` in `CMakeLists.txt` or `makefile` points to your ChucK `include` directory.
2.  Use the provided `makefile`:
    ```bash
    make mac    # macOS
    make win32  # Windows
    make linux  # Linux
    ```
    Alternatively, using CMake directly:
    ```bash
    mkdir build && cd build
    cmake ..
    cmake --build .
    ```
3.  The resulting `PluginHost.chug` file will be created in the project root.

## License

This project is licensed under the MIT License - see the LICENSE file for details (if applicable). JUCE is used under its own licensing terms.
