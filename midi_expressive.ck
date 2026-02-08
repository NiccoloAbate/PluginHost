// midi_expressive.ck
@import "PluginHost";

PluginHost synth;

// Load a MIDI-controllable synth
synth.load("/Library/Audio/Plug-Ins/VST3/Pianoteq 8.vst3");
synth => dac;

fun void playExpressive()
{
    while( true )
    {
        60 => int note;
        synth.noteOn(note, 0.8);
        
        // Bend note up and back over 1 second
        for( 0 => int i; i <= 100; i++ )
        {
            // Map 0-100 to -1.0 to 1.0 bend
            (i / 50.0) - 1.0 => float bend;
            synth.pitchBend(bend);
            
            // Sweep Mod Wheel (CC 1)
            (i / 100.0 * 127) => float ccVal;
            synth.controlChange(1, ccVal $ int);
            
            10::ms => now;
        }
        
        synth.noteOff(note);
        0.5::second => now;
    }
} spork ~playExpressive();

synth.showEditor();

while( true ) 1::second => now;
