@import "PluginHost";

PluginHost plugin;

// tell the plugin to run in synchronous mode
plugin.forceSynchronous(true);

// set the block size
plugin.blockSize(16);

// load the plugin
plugin.load("/Library/Audio/Plug-Ins/VST3/Pianoteq 8.vst3");
//plugin.load("/Library/Audio/Plug-Ins/VST3/Graphiti.vst3");
//plugin.load("/Library/Audio/Plug-Ins/Components/Guitar Rig 7.component");

// show the editor
plugin.showEditor();

// wait for events
// while (plugin.asyncEventRunning())
    // 1::ms => now;
// plugin.waitForAsyncEvent();

// save state
//plugin.saveState("/Users/niccoloabate/Downloads/chuckGraphitiState");
plugin.loadState("/Users/niccoloabate/Downloads/chuckGraphitiState");

// print parameters
<<< plugin.numParams(), "params:" >>>;
for(0 => int i; i < plugin.numNonMidiParams(); i++)
    <<< i, ":", plugin.paramName(i), "(" + plugin.paramLabel(i) + ")" >>>;

// print "programs"
<<< plugin.numPrograms(), "programs:" >>>;
for(0 => int i; i < plugin.numPrograms(); i++)
    <<< i, ":", plugin.programName(i) >>>;

plugin => dac;

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