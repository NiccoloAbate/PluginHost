// param_modulation.ck
@import "PluginHost";

PluginHost plugin;
plugin.forceSynchronous(true);

// Load an effect or synth
plugin.load("/Library/Audio/Plug-Ins/VST3/Pianoteq 8.vst3");
plugin => dac;

// Try to find a parameter by name
"Volume" => string targetParam;
plugin.findParam(targetParam) => int paramIdx;

if( paramIdx < 0 ) {
    <<< "Could not find param:", targetParam, "- using index 0" >>>;
    0 => paramIdx;
}

<<< "Modulating:", plugin.paramName(paramIdx) >>>;

// Show editor to see the slider moving
plugin.showEditor();

fun void playNotes()
{
    int i;
    while (true)
    {
        (i++ % 12) => int note;
        0.2::second => now;
        plugin.noteOn(60 + note, 1.0);
        0.2::second => now;
        plugin.noteOff(60 + note);
    }
} spork ~playNotes();

fun void displayParamValue()
{
    while (true)
    {
        // Print display value
        <<< plugin.paramDisplay(paramIdx) >>>;
        1::second => now;
    }
} spork ~displayParamValue();

// LFO Modulation loop
while( true )
{
    // 0.5 Hz Sine wave mapped to [0.0, 1.0]
    (Math.sin(now/second * Math.PI) + 1.0) / 2.0 => float val;
    plugin.param(paramIdx, val);
    
    10::ms => now;
}
