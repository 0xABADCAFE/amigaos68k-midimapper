# amigaos68k-midimapper

Very small command line utility for remapping midi event data. Requires midi.library (http://aminet.net/package/mus/midi/MidiLib20) to be installed and a working serial MIDI interface. 

Loads a config file that defines how to remap various midi data on the fly. Use a basic Amiga MIDI interface and place in between the event sender and recipient. You can define remappings for channel numbers, voices, individual notes, controller values, curves etc. See the example PSS680ToGM.cfg file as an illustration.

## Background
I wrote this years ago and used it to help integrate MIDI devices I had since childhood into a (by the time) slightly more modern setting. I ran it on a bare bones A1200 and it had no significant issues or latencies.

Compiles with Storm C (v3). The project file with the silly paragraph character extension is not properly handled in the modern age and so has been renamed to .prj in the source directory. If you know how to use StormC, you know how to deal with it.

