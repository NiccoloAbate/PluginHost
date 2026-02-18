{
    PluginHost plugin => dac;

    // tell the plugin to run in synchronous mode (default true)
    plugin.forceSynchronous(true);

    // set the block size (default 16)
    plugin.blockSize(64);

    // load the plugin
    plugin.load("/Library/Audio/Plug-Ins/VST3/Pianoteq 8.vst3");
    //plugin.load("/Library/Audio/Plug-Ins/VST3/Graphiti.vst3");
    //plugin.load("/Library/Audio/Plug-Ins/Components/Guitar Rig 7.component");
    //plugin.load("C:/Program Files/Common Files/VST3/Graphiti.vst3");
    //plugin.load("C:/Program Files/Common Files/VST3/Pianoteq 9.vst3");

    // show the editor
    plugin.showEditor();

    10::second => now;
}

10::second => now;
