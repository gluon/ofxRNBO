# example-midi-in

MIDI notes from openFrameworks into a RNBO synth. A transposable computer keyboard
plays a polyphonic RNBO voice with an ADSR envelope, a bit-crusher and a lowpass
filter: press keys to hold notes, release to let them go.

| patch | openFrameworks |
| --- | --- |
| ![Max patch](docs/patch-max.png)<br>![RNBO patch](docs/patch-rnbo.png) | ![openFrameworks](docs/of.png) |

## What it demonstrates

- Sending MIDI note-on and note-off from oF into RNBO with
  `ofxRNBO::sendMidiNote`, the upstream MIDI path.
- Polyphony from the keyboard: several keys held at once send several notes, and
  each note-off matches the note its key started.
- Absorbing the operating system key auto-repeat, so a held key sends one
  note-on, not a stream.
- The synth parameters, read from the export and driven live from sliders: the
  ADSR (`a d s r`), the crusher and the cutoff.

This example uses the machine keyboard only, no external MIDI and no ofxMidi. An
external MIDI input example comes separately.

## Keyboard

Click the on-screen keyboard to play. This is the universal way and works on any
keyboard layout: press a key with the mouse for note-on, release for note-off. The
keys light up in the accent colour on the notes that are held. `w` and `x` shift
the octave, shown next to the keyboard.

As a bonus on a French AZERTY keyboard, the machine keys play too:

- `q s d f g h j k` play the naturals, C D E F G A B and the next C.
- `z e   t y u` play the sharps in between.
- `w` and `x` shift the octave down and up.

Chords need several keys at once, so use the machine keyboard for polyphony; the
mouse plays one note at a time.

## Patch to export, the three-step drill

An example does not run out of the box. You produce the export yourself from the
patch that ships with it.

1. Open `rnbo-patch/` in Max and load the provided `.maxpat`.
2. Export it with the RNBO C++ Source Code Export into `example-midi-in/rnbo-export/`.
   Use the standard export, not the Minimal Export. Keep RNBO's default export name
   so it produces `rnbo_source.cpp`; the build looks for it there.
3. Build the example. See Building below.

The `rnbo-export/` folder is git-ignored: the RNBO runtime is Cycling '74 licensed
and is never committed. The `rnbo-patch/` folder is versioned: the source patch is
the author's own and ships with the example.

The patch is a MIDI-driven polyphonic synth: a notein feeds a voice with an ADSR,
a bit-crusher and a lowpass filter, stereo out. Its parameters `a d s r`, `crusher`
and `cutoff` appear as sliders.

## Building

Two ways to build. On recent macOS the openFrameworks template still links the AGL
framework, which the current SDK no longer ships, so the link fails until AGL is
removed. The Project Generator route below shows exactly how; a command-line build
needs the same framework dropped from your openFrameworks core linker configuration.

### Command line

From this example's folder:

```bash
make
make RunRelease
```

### Project Generator and Xcode

1. Open the openFrameworks Project Generator, point it at this example, and make
   sure the `ofxRNBO` addon is included. Generate the project.
2. Open the generated `.xcodeproj`.
3. Remove the AGL framework. In Build Settings, search for `AGL`. Everywhere
   `-framework AGL` appears, remove only that flag and leave the other frameworks
   in place:
   - Other Linker Flags, under Linking - General
   - OF_CORE_FRAMEWORKS, under User-Defined

   Do this at both the Project level and the Target level.
4. Build.

## What to see and hear

A fixed window, dark and monospace. A two-octave keyboard that lights up the held
notes, the current octave, and the key legend. Below, a slider for each synth
parameter. At the bottom, a mono scope of the output. Play the keyboard and you
should hear the RNBO synth, one voice per held key, with its ADSR shaping each note.

If the window says "no export loaded", you have not exported the patch into
`rnbo-export/` yet. Do the three steps above and rebuild.

## Note

A green build proves nothing about the sound. The judge is what comes out of
ofSoundStream, checked by ear with the real export.
