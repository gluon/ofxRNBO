# ofxRNBO

Bake a RNBO C++ export into a self-contained openFrameworks binary, running on ofSoundStream. One finished, playable artifact, not a live runner.

## Status

Validated on macOS with openFrameworks 0.12.1. The example plays through ofSoundStream against a real RNBO 1.4.5 C++ export and has been validated by ear. Linux and Windows are not tested yet. This is R&D code.

## What it does

You export a RNBO patch as C++ Source Code. This addon compiles that export directly into an openFrameworks application: a single binary, no runtime patcher, audio flowing through ofSoundStream. The wrapper drives a `RNBO::CoreObject`, hands it audio block by block with its own de/interleave, and exposes the patch parameters and MIDI input to your oF app.

It targets the standard C++ Source Code Export, not the Minimal Export.

## Objects

The addon is one class, `ofxRNBO`, declared in `src/ofxRNBO.h`. It owns a `RNBO::CoreObject` and is itself the RNBO event handler.

Setup, from an ofSoundStream configuration:

```cpp
bool setup(const ofSoundStreamSettings & settings, std::size_t maxBlockSize = 4096);
bool setup(double sampleRate, std::size_t bufferSize, std::size_t numInputChannels,
           std::size_t maxBlockSize = 4096);
bool isReady() const;
```

Audio, wired to your ofApp callbacks:

```cpp
void audioOut(ofSoundBuffer & buffer);   // chunks, runs RNBO, interleaves back
void audioIn(ofSoundBuffer & buffer);    // deinterleaves input, for patches that take audio in
void update();                           // call from the main thread, drains RNBO events
```

Channel layout, reported by the export itself:

```cpp
std::size_t getNumInputChannels() const;
std::size_t getNumOutputChannels() const;
std::size_t getNumSignalInParameters() const;
std::size_t getNumSignalOutParameters() const;
```

Parameters. Ids are unique, names are not, so prefer ids. The by-name map keeps the lowest index on a name clash and logs it once.

```cpp
std::size_t getNumParameters() const;
std::string getParameterId(std::size_t index) const;
std::string getParameterName(std::size_t index) const;
bool        getParameterInfo(std::size_t index, RNBO::ParameterInfo & info) const;

int getParameterIndexForId(const std::string & id) const;
int getParameterIndexForName(const std::string & name) const;

void   setParameter(std::size_t index, double value);
void   setParameterNormalized(std::size_t index, double normalized);
double getParameter(std::size_t index) const;
double getParameterNormalized(std::size_t index) const;

bool setParameterById(const std::string & id, double value);
bool setParameterByName(const std::string & name, double value);
bool setParameterNormalizedById(const std::string & id, double normalized);
bool setParameterNormalizedByName(const std::string & name, double normalized);
```

MIDI input. RNBO carries at most three bytes per event, so longer messages are refused rather than truncated.

```cpp
std::size_t getNumMidiInputPorts() const;
bool sendMidi(const unsigned char * bytes, std::size_t length, int port = 0);
bool sendMidiNote(int channel, int pitch, int velocity, bool noteOn, int port = 0);
```

Escape hatch, for anything not wrapped:

```cpp
RNBO::CoreObject & getCoreObject();
```

## Installation

Clone into your openFrameworks `addons/` directory:

```bash
cd openFrameworks/addons
git clone https://github.com/gluon/ofxRNBO.git
```

Generate a project with the Project Generator and add `ofxRNBO`, or start from `example-basic`.

Then drop your own RNBO export in. The addon expects it, relative to the addon root:

```
rnbo-export/rnbo_source.cpp
rnbo-export/rnbo/RNBO.cpp
rnbo-export/rnbo/RNBO.h
rnbo-export/rnbo/common/
rnbo-export/rnbo/src/
```

The `rnbo-export/` directory is git-ignored and never shipped. The RNBO runtime and your export are Cycling '74 licensed; you bring your own.

## Quick start

A minimal app that loads the export and plays it, driving one parameter by id:

```cpp
#include "ofMain.h"
#include "ofxRNBO.h"

class ofApp : public ofBaseApp {
public:
    void setup() override {
        ofSoundStreamSettings settings;
        settings.numOutputChannels = 2;
        settings.numInputChannels = 0;
        settings.sampleRate = 44100;
        settings.bufferSize = 512;
        settings.numBuffers = 4;

        if (!rnbo.setup(settings)) {
            ofLogError() << "No RNBO export found in rnbo-export/";
            return;
        }

        settings.setOutListener(this);
        soundStream.setup(settings);
    }

    void update() override {
        rnbo.update();                      // drain RNBO events on the main thread
    }

    void keyPressed(int key) override {
        if (key == OF_KEY_UP)   rnbo.setParameterById("freq", 440.0);
        if (key == OF_KEY_DOWN) rnbo.setParameterById("freq", 110.0);
    }

    void audioOut(ofSoundBuffer & buffer) override {
        rnbo.audioOut(buffer);
    }

private:
    ofxRNBO rnbo;
    ofSoundStream soundStream;
};
```

## Examples

- `example-basic` loads an export, plays it through ofSoundStream, and drives a `freq` parameter from a slider on the arrow keys. If your export names its parameter differently, change `paramId` in `example-basic/src/ofApp.h`.

## Compatibility

- openFrameworks 0.12.1
- macOS, tested
- Linux, Windows, not tested yet
- RNBO C++ Source Code Export, developed against RNBO 1.4.5

## Layout

```
ofxRNBO/
  src/
    ofxRNBO.h            the wrapper class
    ofxRNBO.cpp
  example-basic/         minimal oF app playing an export
    src/
  addon_config.mk        include paths and defines for the export you drop in
  rnbo-export/           your RNBO export goes here, git-ignored, never shipped
  README.md
  LICENSE
  ACKNOWLEDGEMENTS.md
```

## Author

Julien Bayle
tech: Structure Void <https://structure-void.com>
art: <https://julienbayle.net>
Ableton Certified Trainer · Max Certified Trainer.

## Acknowledgements

RNBO and the RNBO C++ runtime are made by Cycling '74. This addon is glue around a RNBO export; it does not include, modify, or redistribute the RNBO runtime. See <https://rnbo.cycling74.com>.

openFrameworks is made by the openFrameworks community. See <https://openframeworks.cc>.

## License

The glue in this repository is MIT, see [LICENSE](LICENSE). The RNBO C++ runtime and the export you drop in are licensed by Cycling '74, are not included here, and are not redistributed by this repository. You bring your own export. This is R&D code, provided without warranty.
