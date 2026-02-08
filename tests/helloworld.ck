PluginHost plugin => dac;

// tell the plugin to run in synchronous mode (default true)
plugin.forceSynchronous(true);

// set the block size (default 16)
plugin.blockSize(64);

// load the plugin
plugin.load("/Library/Audio/Plug-Ins/VST3/Pianoteq 8.vst3");
//plugin.load("/Library/Audio/Plug-Ins/VST3/Graphiti.vst3");
//plugin.load("/Library/Audio/Plug-Ins/Components/Guitar Rig 7.component");

// show the editor
plugin.showEditor();

// wait for events (when not in synchronous mode)
// while (plugin.asyncEventRunning())
    // 1::ms => now;
// plugin.waitForAsyncEvent();
// plugin.asyncEvent() => now; (not quite fully thread safe...)

// save state
plugin.saveState("pluginState");
// load state
plugin.loadState("pluginState");

// print parameters
<<< plugin.numParams(), "params:" >>>;
for(0 => int i; i < plugin.numNonMidiParams(); i++)
    <<< i, ":", plugin.paramName(i), "(" + plugin.paramLabel(i) + ")" >>>;

// print "programs"
<<< plugin.numPrograms(), "programs:" >>>;
for(0 => int i; i < plugin.numPrograms(); i++)
    <<< i, ":", plugin.programName(i) >>>;

fun void playNotes()
{
    for (0 => int i; i < 10; ++i)
    {
        0.25::second => now;
        plugin.noteOn(60+i, 1.0);
        0.25::second => now;
        plugin.noteOff(60+i);
    }
} spork ~playNotes();

// window title
GG.windowTitle("ChucK/ChuGl/JUCE");

// time loop
while( true )
{
    GG.nextFrame() => now;
}