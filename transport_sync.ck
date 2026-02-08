// transport_sync.ck
@import "PluginHost";

PluginHost plugin;
plugin.forceSynchronous(true);

// Load a plugin with a sequencer or synced delay
plugin.load("/Library/Audio/Plug-Ins/VST3/Pianoteq 8.vst3");
plugin => dac;

120.0 => float bpm;
plugin.bpm(bpm);
plugin.timeSig(4, 4);
plugin.playing(true);

0.0 => float ppq;

while( true )
{
    // Calculate PPQ based on BPM and ChucK time
    // 1 beat = 60 / BPM seconds
    (10::ms / second) * (bpm / 60.0) +=> ppq;
    plugin.pos(ppq);
    
    // Print every beat
    if( ppq % 1.0 < 0.05 ) <<< "Beat:", ppq $ int >>>;
    
    10::ms => now;
}
