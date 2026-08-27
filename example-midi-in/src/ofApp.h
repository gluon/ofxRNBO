#pragma once

// ofxRNBO
// Example app: play a polyphonic RNBO synth from the on-screen keyboard, with the mouse.
//
// Julien Bayle / Structure Void
// https://julienbayle.net
// https://structure-void.com
//
// MIT for this addon's glue. The RNBO runtime and your export remain under
// their own Cycling '74 license and are not part of this repository.

#include "ofMain.h"
#include "ofxRNBO.h"

class ofApp : public ofBaseApp {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void mouseMoved(int x, int y) override;
	void mousePressed(int x, int y, int button) override;
	void mouseDragged(int x, int y, int button) override;
	void mouseReleased(int x, int y, int button) override;

	void audioOut(ofSoundBuffer & buffer) override;

private:
	void setParamFromMouse(int index, int x);

	void sendNoteOn(int note);
	void sendNoteOff(int note);
	void buildKeyboard();     // fixed rectangles for the on-screen keys, one source for draw and clicks
	int  keyNoteAt(int x, int y) const;   // MIDI note under the point, -1 if not on a key

	void drawHeader();
	void drawKeyboard();   // legend, current octave, and the lit on-screen keys
	void drawParams();
	void drawWaveform();
	void drawFooter();
	bool hit(const ofRectangle & r, int x, int y) const;

	ofxRNBO rnbo;
	ofSoundStream soundStream;

	// MIDI note sending state.
	int baseNote = 60;   // MIDI note for the leftmost key, middle C by default
	int velocity = 100;
	int channel = 0;

	// On-screen keyboard, played with the mouse. Each key carries its offset from baseNote so a
	// click sends baseNote + offset. Black keys are tested before white keys because they sit on
	// top.
	struct Key {
		ofRectangle rect;
		int offset = 0;   // semitones above baseNote
		bool black = false;
	};
	std::vector<Key> keys;
	int mouseNote = -1;   // MIDI note currently held by the mouse, -1 if none

	// Clickable octave-shift buttons, resolved in setup.
	ofRectangle octaveDownZone;
	ofRectangle octaveUpZone;

	// Synth parameters exposed by the export, shown as sliders. Whatever the patch exposes,
	// so this adapts to the actual synth without assuming names.
	struct Param {
		std::string id;
		std::size_t index = 0;
		float min = 0.0f;
		float max = 1.0f;
		float value = 0.0f;
		ofRectangle bar;
	};
	std::vector<Param> params;
	int draggingParam = -1;

	// Output scope, mono.
	std::vector<float> waveMono;
	std::size_t wavePos = 0;

	// Footer link zones.
	ofRectangle techLinkZone;
	ofRectangle artLinkZone;
	std::string techUrl = "https://structure-void.com";
	std::string artUrl = "https://julienbayle.net";
	int hoverLink = 0; // 0 none, 1 tech, 2 art
};
