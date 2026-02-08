// plugin_chain.ck
@import "PluginHost";

PluginHost synth;
PluginHost fx;

synth.forceSynchronous(true);
fx.forceSynchronous(true);

// Load a Synth
synth.load("/Library/Audio/Plug-Ins/VST3/Pianoteq 8.vst3");

// Load an Effect (Update path to a reverb, delay, or distortion)
fx.load("/Library/Audio/Plug-Ins/Components/EchoBoyJr.component");

// Chain: Synth -> FX -> DAC
synth => fx => dac;

// Play a simple chord through the chain
fun void play()
{
    while( true )
    {
        synth.noteOn(60, 0.7);
        synth.noteOn(64, 0.7);
        synth.noteOn(67, 0.7);
        1::second => now;
        
        synth.allNotesOff();
        1::second => now;
    }
} spork ~play();

// show editors
synth.showEditor();
fx.showEditor();

while( true ) 1::second => now;
