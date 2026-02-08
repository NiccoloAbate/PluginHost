// PluginHost Comprehensive Test

PluginHost plugin => dac;

// Load a plugin first
plugin.load("/Library/Audio/Plug-Ins/VST3/Pianoteq 8.vst3");

<<< "--- 1. Testing Playhead & Transport Functions ---" >>>;

// BPM
130.0 => plugin.bpm;
<<< "BPM set to 130.0, getBpm():", plugin.bpm() >>>;

// Time Signature (Setter only)
plugin.timeSig(3, 4);
<<< "Time signature set to 3/4" >>>;

// Position (PPQ)
1.5 => plugin.pos;
<<< "Position set to 1.5, getPos():", plugin.pos() >>>;

// Playing status
1 => plugin.playing;
<<< "Playing status set to 1, getPlaying():", plugin.playing() >>>;

// Recording status
1 => plugin.recording;
<<< "Recording status set to 1, getRecording():", plugin.recording() >>>;

// Last Bar Position
4.0 => plugin.lastBarPos;
<<< "Last bar position set to 4.0, getLastBarPos():", plugin.lastBarPos() >>>;

// Looping
1 => plugin.looping;
<<< "Looping status set to 1, getLooping():", plugin.looping() >>>;

// Loop Points (Batch setter)
plugin.loopPoints(0.0, 8.0);
<<< "Loop points set to 0.0 -> 8.0" >>>;

// Loop Start/End (Individual accessors)
0.5 => plugin.loopStart;
<<< "Loop start set to 0.5, getLoopStart():", plugin.loopStart() >>>;
7.5 => plugin.loopEnd;
<<< "Loop end set to 7.5, getLoopEnd():", plugin.loopEnd() >>>;


<<< "--- 2. Testing Host State & Config Functions ---" >>>;

// Latency
<<< "Current latency (samples):", plugin.latency() >>>;

// Bypass
1 => plugin.bypass;
<<< "Bypass enabled, getBypass():", plugin.bypass() >>>;
0 => plugin.bypass;
<<< "Bypass disabled" >>>;

// Block Size
128 => plugin.blockSize;
<<< "Block size set to 128, getBlockSize():", plugin.blockSize() >>>;

// Realtime mode
1 => plugin.realtime;
<<< "Realtime mode enabled, getRealtime():", plugin.realtime() >>>;

// Synchronicity
1 => plugin.forceSynchronous;
<<< "Force synchronous enabled, getForceSynchronous():", plugin.forceSynchronous() >>>;

// IO Channels
<<< "Input channels:", plugin.numInputs() >>>;
<<< "Output channels:", plugin.numOutputs() >>>;

// Reset
plugin.reset();
<<< "Host reset called" >>>;


<<< "--- 3. Testing MIDI Functions ---" >>>;

// Note On/Off (with channel and default)
plugin.noteOn(60, 0.8, 1);
plugin.noteOff(60, 1);
plugin.noteOn(62, 0.7); // default chan 0
plugin.noteOff(62);     // default chan 0

// All Notes Off
plugin.allNotesOff(1);
plugin.allNotesOff();

// Pitch Bend
plugin.pitchBend(0.5, 1);
plugin.pitchBend(-0.2);

// Aftertouch (Polyphonic)
plugin.aftertouch(64, 0.5, 1);
plugin.aftertouch(64, 0.3);

// Aftertouch (Channel/Pressure)
plugin.aftertouchChannel(0.8, 1);
plugin.aftertouchChannel(0.4);

// Control Change
plugin.controlChange(7, 100, 1);
plugin.controlChange(10, 64);

// Raw MIDI
plugin.midiMsg(144, 60, 100); // Note On chan 1, key 60, vel 100


<<< "--- 4. Testing Plugin & Metadata Functions ---" >>>;

<<< "Plugin Name:", plugin.name() >>>;
<<< "Plugin Vendor:", plugin.vendor() >>>;

// Parameters
plugin.numParams() => int numParams;
plugin.numNonMidiParams() => int numNonMidi;
<<< "Total Params:", numParams, "| Non-MIDI Params:", numNonMidi >>>;

if (numParams > 0) {
    plugin.paramName(0) => string pName;
    <<< "Param 0 Name:", pName >>>;
    <<< "Param 0 Label:", plugin.paramLabel(0) >>>;
    <<< "Param 0 Display:", plugin.paramDisplay(0) >>>;
    
    plugin.param(0, 0.5); // setParam
    <<< "Param 0 Value:", plugin.param(0) >>>; // getParam
    
    <<< "Index of param '" + pName + "':", plugin.findParam(pName) >>>;
}

// Programs
plugin.numPrograms() => int numProgs;
<<< "Number of programs:", numProgs >>>;
if (numProgs > 0) {
    <<< "Program 0 Name:", plugin.programName(0) >>>;
    0 => plugin.program; // setProgram
    <<< "Current Program index:", plugin.program() >>>; // getProgram
}

// State (Uncomment to test file IO)
// plugin.saveState("test_plugin_state.bin");
// plugin.loadState("test_plugin_state.bin");

// Editor (Uncomment to test windowing)
// plugin.showEditor();
// 1::second => now;
// plugin.hideEditor();

// Async Tracking
<<< "Async event currently running?", plugin.asyncEventRunning() >>>;
plugin.waitForAsyncEvents();

<<< "-----------------------------------" >>>;
<<< " Comprehensive Test Complete " >>>;
<<< "-----------------------------------" >>>;