#pragma once

// ofxRNBO
// Example app: two-way messaging with a RNBO patch, inports in and outport out.
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
	void keyPressed(int key) override;
	void mouseMoved(int x, int y) override;
	void mousePressed(int x, int y, int button) override;
	void mouseDragged(int x, int y, int button) override;
	void mouseReleased(int x, int y, int button) override;

	void audioOut(ofSoundBuffer & buffer) override;

private:
	void sendPartials();
	void setPartialFromMouse(int slider, int x);
	void setCaretFromMouse(int x);
	void insertAtCaret(char c);

	void drawHeader();
	void drawInput();
	void drawPartials();
	void drawEnvelope();
	void drawWaveform();
	void drawFooter();
	bool hit(const ofRectangle & r, int x, int y) const;

	ofxRNBO rnbo;
	ofSoundStream soundStream;

	// Text input: partial frequencies typed by the user, sent as a list on inport "partials".
	// The field only captures the keyboard when focused, and focus is gained by clicking it.
	std::string inputText;
	std::vector<double> currentFreqs;
	ofRectangle inputZone;
	bool inputFocused = false;
	// Insertion point, an index into inputText in [0, size]. Typing, arrows, Home, End,
	// Backspace, Delete and a click all move or act at this position.
	std::size_t caret = 0;

	// Four partial amplitudes, params partial1..partial4, mixed live from sliders.
	static const int kPartials = 4;
	float amp[kPartials] = { 0.5f, 0.5f, 0.5f, 0.5f };
	ofRectangle partialBar[kPartials];
	int draggingSlider = -1;

	// Level read back from the patch on outport "envelope".
	float envLevel = 0.0f;

	// Output scope, mono. The two output channels carry the same signal here, so the ring
	// buffer holds their average as one trace. Filled in audioOut, drawn in draw. Pre-sized in
	// setup so the audio callback never allocates.
	std::vector<float> waveMono;
	std::size_t wavePos = 0;

	// Footer link zones, resolved in setup, tested in mousePressed.
	ofRectangle techLinkZone;
	ofRectangle artLinkZone;
	std::string techUrl = "https://structure-void.com";
	std::string artUrl = "https://julienbayle.net";
	int hoverLink = 0; // 0 none, 1 tech, 2 art
};
