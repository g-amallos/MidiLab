# MidiLab
**MidiLab** is a WIP DAW built with [raylib](https://www.raylib.com/) and [FluidSynth](https://www.fluidsynth.org/).


### Compilation
For windows (which is more thoroughly tested on) run:
```bash
$ make os=win
```

For Linux:
```bash
$ make
```


### Known issues
- **FluidSynth** needs to be installed if on Linux.


### Notes
- Memory leaks and other such bugs and issues may be possible, as I may have missed something in my own code.
- Use the POSIX Threads standard when compiling with gcc-mingw.