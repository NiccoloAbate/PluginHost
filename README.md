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
- [JUCE 8.0.4](https://juce.com/) (included as a git submodule).
- [Projucer](https://juce.com/get-juce/download) (to generate the static library project).
- [CMake](https://cmake.org/) or `make`.

### Build Instructions

The Chugin links against a static library of JUCE to keep the build process efficient.

1.  **Initialize Submodules**:
    ```bash
    git submodule update --init --recursive
    ```
2.  **Build the JUCE Static Library**:
    - Open `JuceStaticLib/JuceStaticLib.jucer` in the **Projucer**.
    - Ensure the module paths are correct for your system.
    - Click **"Save and Open in IDE"** (Xcode, Visual Studio, etc.).
    - Build the **Release** configuration.
    - Ensure the resulting static library is placed in (or copied to) `Juce/Mac/Release/` (or the equivalent for your OS).
3.  **Build the Chugin**:
    - Ensure `CK_SRC_PATH` in the root `makefile` or `CMakeLists.txt` points to your ChucK `include` directory.
    - Use the root `makefile`:
      ```bash
      make mac    # macOS
      make win32  # Windows
      make linux  # Linux
      ```
    - The resulting `PluginHost.chug` will be created in the project root.

> **Note**: Full CMake support (compiling JUCE modules directly without Projucer) is on the roadmap for a future update.

## Quick Start

```chuck
// Create a PluginHost instance and connect to dac
PluginHost plugin => dac;

// Load a VST3 plugin
plugin.load("/Library/Audio/Plug-Ins/VST3/Plugin.vst3");

// Show the plugin editor GUI
plugin.showEditor();

// Send a MIDI note
plugin.noteOn(60, 0.8);
1::second => now;
plugin.noteOff(60);

// Automate a parameter
0 => int paramIndex;
plugin.param(paramIndex, 0.5)
```

## Synchronous vs. Asynchronous Events

By default, `PluginHost` operates in **Synchronous Mode** (`forceSynchronous(true)`). 

In this mode, any operation that must happen on the main thread (such as `load()`, `showEditor()`, `saveState()`, or `loadState()`) will block the audio process and the ChucK VM until the operation is complete. 

### Why this matters:
- **Simplicity**: You don't have to manage timing (which can be confusing and hard to reason about) or wait for callbacks.
- **Audio Performance**: Blocking the audio process can lead to "dropouts" or glitches in the audio stream if the operation (like loading a heavy plugin) takes too long. In practice that may not matter if all these events happend during program initialization or other non-realtime junctures.
- **Safety**: There is a theoretical risk of deadlocks, though this hasn't been observed in standard ChucK usage.

For high-performance or real-time applications where you want to load plugins without glitching existing audio set `forceSynchronous(false)` and use `asyncEventRunning()` or `waitForAsyncEvents()` to manage the lifecycle of these operations.

## API Reference

### Loading & Metadata
- `void load(string path)`: Load a plugin from the given file path.
- `string name()`: Get the loaded plugin's name.
- `string vendor()`: Get the plugin's manufacturer name.
- `int numInputs()`: Get total number of input channels.
- `int numOutputs()`: Get total number of output channels.
[//] # - `void reset()`: Reset the plugin's internal state.

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
- `void controlChange(int control, int value)`: Send CC (channel 1).
- `void controlChange(int control, int value, int channel)`: Send CC (channel 1-16).
- `void pitchBend(float value)`: Send Pitch Bend (-1.0 to 1.0) (channel 1).
- `void pitchBend(float value, int channel)`: Send Pitch Bend (-1.0 to 1.0) (channel 1-16).
- `void aftertouch(int note, float pressure)`: Polyphonic aftertouch (channel 1).
- `void aftertouch(int note, float pressure, int channel)`: Polyphonic aftertouch (channel 1-16).
- `void aftertouchChannel(float pressure)`: Channel pressure (channel 1).
- `void aftertouchChannel(float pressure, int channel)`: Channel pressure (channel 1-16).
- `void allNotesOff()`: Send All Notes Off (channel 1).
- `void allNotesOff(int channel)`: Send All Notes Off (channel 1-16).
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
- `void addQWERTYMidiInput()`: Open the computer keyboard MIDI input window.
- `void removeQWERTYMidiInput()`: Close the computer keyboard MIDI input window.
- `void toggleQWERTYMidiInput()`: Toggle the computer keyboard MIDI input window.

### Async & Configuration
- `void forceSynchronous(int b)`: If true (default), wait for async events (like loading) to complete before returning.
- `int forceSynchronous()`: Check if synchronous mode is active.
- `int asyncEventRunning()`: Returns true (1) if an asynchronous operation is currently in progress.
- `void waitForAsyncEvents()`: Blocks the current ChucK shred until all pending async events are finished. **Warning:** This is not real-time safe.
- `void blockSize(int size)` / `int blockSize()`: Set/get processing block size (default 16). Larger sizes are more efficient but introduce more latency.
- `int latency()`: Get plugin latency in samples.
- `void bypass(int b)` / `int bypass()`: Set/get whether the plugin is bypassed.
- `void realtime(int b)` / `int realtime()`: Set/get whether the plugin operates in realtime mode.

## Roadmap

- **MIDI Output**: Support for plugins that generate MIDI.
- **MPE (MIDI Polyphonic Expression)**: Support for expressive MIDI controllers.
- **Easier Plugin Search**: Improved workflow for locating installed plugins.
- **Full Linux Support**: Theoretically should work, but it needs to be built and tested.
- **ChucK Event Support For Async Event Synchronization**: plugin.asyncEvent() => now; (Current asyncEventRunning() or waitForAsyncEvents() must be used).

**Please reach out to me with any requests!**

## Examples

Check the `tests/` directory for comprehensive examples:
- `helloworld.ck`: Basic loading and MIDI playback.
- `param_modulation.ck`: Automating parameters from ChucK.
- `transport_sync.ck`: Synchronizing LFOs and sequencers via the playhead.
- `plugin_chain.ck`: Chaining multiple `PluginHost` instances.
- `midi_expressive.ck`: Expressive midi controls such as pitch bend and mod wheel.
- `destroy.ck`: Destructive of a plugin during runtime.

## License

This project is licensed under the MIT License - see the LICENSE file for details (if applicable). JUCE is used under its own licensing terms.
