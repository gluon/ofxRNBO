#pragma once

// ofxRNBO
// Example app: load a RNBO export and play it through ofSoundStream.
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

	void audioOut(ofSoundBuffer & buffer) override;

private:
	ofxRNBO rnbo;
	ofSoundStream soundStream;

	// The parameter this example drives. Change to match your export.
	// The test patch exposes "freq". If your export names it differently, edit this.
	std::string paramId = "freq";
	int paramIndex = -1;
	float paramMin = 0.0f;
	float paramMax = 1.0f;
	float paramValue = 0.0f;
};
