// plugin_chain.ck

PluginHost synth => PluginHost fx => dac;

// Load a Synth
synth.load("/Library/Audio/Plug-Ins/VST3/Pianoteq 8.vst3");

// Load an Effect
fx.load("/Library/Audio/Plug-Ins/Components/EchoBoyJr.component");

// Repeat a simple chord
fun void play()
{
    while (true)
    {
        synth.noteOn(60, 0.7);
        synth.noteOn(64, 0.7);
        synth.noteOn(67, 0.7);
        1::second => now;
        
        synth.allNotesOff();
        1::second => now;
    }
} spork ~play();

// Show editors
synth.showEditor();
fx.showEditor();

while (true) 1::second => now;
