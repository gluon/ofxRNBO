// ofxRNBO
// Example app implementation: sound stream setup, parameter slider, audio callback.
//
// Julien Bayle / Structure Void
// https://julienbayle.net
// https://structure-void.com
//
// MIT for this addon's glue. The RNBO runtime and your export remain under
// their own Cycling '74 license and are not part of this repository.

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
	ofBackground(20);
	ofSetVerticalSync(true);

	ofSoundStreamSettings settings;
	settings.numOutputChannels = 2;
	settings.numInputChannels = 0;
	settings.sampleRate = 44100;
	settings.bufferSize = 512;
	settings.numBuffers = 4;

	// Prepare RNBO from the same settings before starting the stream.
	if (!rnbo.setup(settings)) {
		ofLogError("ofApp") << "ofxRNBO setup failed. Did you drop a RNBO export into rnbo-export/ ?";
		return;
	}

	ofLogNotice("ofApp") << "RNBO ready: "
						 << rnbo.getNumOutputChannels() << " out, "
						 << rnbo.getNumInputChannels() << " in, "
						 << rnbo.getNumParameters() << " params";
	for (std::size_t i = 0; i < rnbo.getNumParameters(); ++i) {
		ofLogNotice("ofApp") << "  param " << i
							 << " id='" << rnbo.getParameterId(i)
							 << "' name='" << rnbo.getParameterName(i) << "'";
	}

	// Resolve the parameter by id and read its real range for the slider mapping.
	paramIndex = rnbo.getParameterIndexForId(paramId);
	if (paramIndex >= 0) {
		RNBO::ParameterInfo info;
		if (rnbo.getParameterInfo(static_cast<std::size_t>(paramIndex), info)) {
			paramMin = static_cast<float>(info.min);
			paramMax = static_cast<float>(info.max);
			paramValue = static_cast<float>(rnbo.getParameter(static_cast<std::size_t>(paramIndex)));
		}
	} else {
		ofLogWarning("ofApp") << "parameter id '" << paramId << "' not found in this export";
	}

	settings.setOutListener(this);
	soundStream.setup(settings);
}

//--------------------------------------------------------------
void ofApp::update()
{
	// Drain RNBO events on the main thread.
	rnbo.update();
}

//--------------------------------------------------------------
void ofApp::draw()
{
	ofSetColor(230);
	ofDrawBitmapString("ofxRNBO example-basic", 24, 40);

	if (!rnbo.isReady()) {
		ofSetColor(255, 120, 120);
		ofDrawBitmapString("No RNBO export loaded. Drop one into rnbo-export/ and rebuild.", 24, 80);
		return;
	}

	ofDrawBitmapString("Audio out: " + ofToString(rnbo.getNumOutputChannels()) + " ch", 24, 80);

	if (paramIndex >= 0) {
		std::string line = "Parameter '" + paramId + "' = " + ofToString(paramValue, 3)
						   + "   [" + ofToString(paramMin, 2) + " .. " + ofToString(paramMax, 2) + "]";
		ofDrawBitmapString(line, 24, 120);
		ofDrawBitmapString("Left / Right arrows to change, Up / Down for fine steps", 24, 150);

		// Simple slider bar.
		const float x = 24, y = 180, w = 480, h = 18;
		const float t = (paramMax > paramMin) ? (paramValue - paramMin) / (paramMax - paramMin) : 0.0f;
		ofNoFill();
		ofSetColor(120);
		ofDrawRectangle(x, y, w, h);
		ofFill();
		ofSetColor(90, 200, 160);
		ofDrawRectangle(x, y, w * ofClamp(t, 0.0f, 1.0f), h);
	} else {
		ofSetColor(255, 200, 120);
		ofDrawBitmapString("Parameter '" + paramId + "' not present in this export.", 24, 120);
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
	if (paramIndex < 0) {
		return;
	}

	const float coarse = (paramMax - paramMin) * 0.02f;
	const float fine = (paramMax - paramMin) * 0.002f;

	switch (key) {
		case OF_KEY_RIGHT: paramValue += coarse; break;
		case OF_KEY_LEFT:  paramValue -= coarse; break;
		case OF_KEY_UP:    paramValue += fine;   break;
		case OF_KEY_DOWN:  paramValue -= fine;   break;
		default: return;
	}

	paramValue = ofClamp(paramValue, paramMin, paramMax);
	rnbo.setParameter(static_cast<std::size_t>(paramIndex), paramValue);
}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer & buffer)
{
	rnbo.audioOut(buffer);
}
