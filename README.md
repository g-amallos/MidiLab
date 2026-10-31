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


### To-Do List
- [ ] Make multi-purpose Sorted Vector and integrate it into tracks backend.
- [ ] Add .MLB metadata for things like format version.
- [ ] Create new pitch change events for tracks and a new MLB format version supporting this.
- [ ] Create a "humanize" function, where the selected notes are slightly offset by a random value.
- [ ] Learn and revise the pthread code to rewrite it correctly with mutex locks and minimal race conditions.
- [ ] Memory leak and small bug hunt.