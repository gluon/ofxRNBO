# example-basic

The reference example. One scalar parameter driven from openFrameworks into a RNBO
patch: a sine whose pitch glides when you change it.

This is the pattern every other ofxRNBO example copies: a source Max patch that ships,
a generated export that you produce yourself, and a fixed, sober UI.

## What it demonstrates

- A single scalar parameter (`freq`) set from oF with `setParameter`.
- Audio playing through `ofSoundStream`, de/interleaved by the wrapper.
- A gliding pitch: the patch smooths parameter changes, so the sine slides rather
  than jumps.

## Patch to export, the three-step drill

An example does not run out of the box. You produce the export yourself from the
patch that ships with it.

1. Open `rnbo-patch/` in Max and load the provided `.maxpat`.
2. Export it with the RNBO C++ Source Code Export into `example-basic/rnbo-export/`.
   Use the standard export, not the Minimal Export. Keep RNBO's default export name
   so it produces `rnbo_source.cpp`; the build looks for it there.
3. Build the example and listen:

```bash
make
make RunRelease
```

The `rnbo-export/` folder is git-ignored: the RNBO runtime is Cycling '74 licensed
and is never committed. The `rnbo-patch/` folder is versioned: the source patch is
the author's own and ships with the example.

## What to see and hear

A fixed window, dark and monospace. The parameter `freq`, its value, and a thin
slider showing where it sits in range. Arrow keys move it: left and right coarse,
up and down fine. You should hear a sine tone whose pitch glides to each new value.

If the window says "no export loaded", you have not exported the patch into
`rnbo-export/` yet. Do the three steps above and rebuild.

## Note

A green build proves nothing about the sound. The judge is what comes out of
ofSoundStream, checked by ear with the real export.
