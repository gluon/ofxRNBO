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

// Visual charter, shared across all ofxRNBO examples.
// Dark anthracite ground, monospace type, one cool grey-green accent, thin lines.
namespace {
	const int   kWidth   = 640;
	const int   kHeight  = 400;
	const int   kMargin  = 48;
	const float kCharW   = 8.0f;   // oF bitmap font advance
	const float kLineH   = 14.0f;

	const ofColor kBackground(26, 28, 30);   // deep neutral grey, not pure black
	const ofColor kText(150, 156, 158);      // neutral grey
	const ofColor kDim(96, 101, 104);        // dimmed grey
	const ofColor kLine(58, 62, 65);         // thin rule
	const ofColor kAccent(122, 168, 148);    // cool grey-green, active elements

	ofRectangle textZone(const std::string & s, float x, float y) {
		// Zone around a bitmap string drawn with its baseline at (x, y).
		return ofRectangle(x - 2.0f, y - 11.0f, s.size() * kCharW + 4.0f, kLineH);
	}
}

//--------------------------------------------------------------
void ofApp::setup()
{
	ofSetWindowTitle("ofxRNBO . basic tone");
	ofSetWindowShape(kWidth, kHeight);
	ofSetVerticalSync(true);
	ofSetBackgroundAuto(true);

	ofSoundStreamSettings settings;
	settings.numOutputChannels = 2;
	settings.numInputChannels = 0;
	settings.sampleRate = 44100;
	settings.bufferSize = 512;
	settings.numBuffers = 4;

	if (!rnbo.setup(settings)) {
		ofLogError("ofApp") << "ofxRNBO setup failed. Export a RNBO patch into rnbo-export/ first.";
		return;
	}

	ofLogNotice("ofApp") << "RNBO ready: "
						 << rnbo.getNumOutputChannels() << " out, "
						 << rnbo.getNumInputChannels() << " in, "
						 << rnbo.getNumParameters() << " params";

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

	// Footer link zones, positioned once. Layout is fixed, the window does not resize.
	const float linksY = kHeight - 30.0f;
	const std::string techLabel = "tech structure-void.com";
	const std::string artLabel = "art julienbayle.net";
	techLinkZone = textZone(techLabel, kMargin, linksY);
	artLinkZone = textZone(artLabel, kMargin + techLabel.size() * kCharW + 3 * kCharW, linksY);

	settings.setOutListener(this);
	soundStream.setup(settings);
}

//--------------------------------------------------------------
void ofApp::update()
{
	rnbo.update(); // drain RNBO events on the main thread
}

//--------------------------------------------------------------
void ofApp::draw()
{
	ofBackground(kBackground);
	drawParameter();
	drawFooter();
}

//--------------------------------------------------------------
void ofApp::drawParameter()
{
	const float contentW = kWidth - 2.0f * kMargin;

	if (!rnbo.isReady()) {
		ofSetColor(kAccent);
		ofDrawBitmapString("no export loaded", kMargin, 96);
		ofSetColor(kDim);
		ofDrawBitmapString("export a RNBO patch into rnbo-export/ and rebuild", kMargin, 118);
		return;
	}

	// Parameter id, left. Value, accent, right aligned.
	ofSetColor(kText);
	ofDrawBitmapString(paramId, kMargin, 96);

	if (paramIndex >= 0) {
		const std::string value = ofToString(paramValue, 2);
		ofSetColor(kAccent);
		ofDrawBitmapString(value, kWidth - kMargin - value.size() * kCharW, 96);

		// Slider, thin. Border grey, fill accent.
		const float y = 124.0f;
		const float h = 6.0f;
		const float t = (paramMax > paramMin)
			? ofClamp((paramValue - paramMin) / (paramMax - paramMin), 0.0f, 1.0f)
			: 0.0f;

		ofSetColor(kLine);
		ofNoFill();
		ofDrawRectangle(kMargin, y, contentW, h);
		ofFill();
		ofSetColor(kAccent);
		ofDrawRectangle(kMargin, y, contentW * t, h);

		// Range, dimmed.
		ofSetColor(kDim);
		ofDrawBitmapString(ofToString(paramMin, 0), kMargin, y + 26);
		const std::string maxLabel = ofToString(paramMax, 0);
		ofDrawBitmapString(maxLabel, kWidth - kMargin - maxLabel.size() * kCharW, y + 26);

		// Controls hint.
		ofSetColor(kDim);
		ofDrawBitmapString("left / right  coarse      up / down  fine", kMargin, 196);
	} else {
		ofSetColor(kAccent);
		ofDrawBitmapString("parameter '" + paramId + "' not in this export", kMargin, 124);
	}
}

//--------------------------------------------------------------
void ofApp::drawFooter()
{
	const float contentW = kWidth - 2.0f * kMargin;

	// Separating rule.
	ofSetColor(kLine);
	ofDrawLine(kMargin, kHeight - 108, kMargin + contentW, kHeight - 108);

	// Name, two words.
	ofSetColor(kText);
	ofDrawBitmapString("basic tone", kMargin, kHeight - 88);

	// One line describing what it does.
	ofSetColor(kDim);
	ofDrawBitmapString("one scalar parameter, a sine that glides", kMargin, kHeight - 70);

	// Author.
	ofSetColor(kText);
	ofDrawBitmapString("Julien Bayle / Structure Void", kMargin, kHeight - 46);

	// Clickable links, accent. Brighter on hover, underlined.
	const float linksY = kHeight - 30.0f;
	const std::string techLabel = "tech structure-void.com";
	const std::string artLabel = "art julienbayle.net";

	ofSetColor(hoverLink == 1 ? ofColor(160, 206, 184) : kAccent);
	ofDrawBitmapString(techLabel, techLinkZone.x + 2, linksY);
	if (hoverLink == 1) {
		ofDrawLine(techLinkZone.x + 2, linksY + 3, techLinkZone.x + 2 + techLabel.size() * kCharW, linksY + 3);
	}

	ofSetColor(hoverLink == 2 ? ofColor(160, 206, 184) : kAccent);
	ofDrawBitmapString(artLabel, artLinkZone.x + 2, linksY);
	if (hoverLink == 2) {
		ofDrawLine(artLinkZone.x + 2, linksY + 3, artLinkZone.x + 2 + artLabel.size() * kCharW, linksY + 3);
	}
}

//--------------------------------------------------------------
bool ofApp::hit(const ofRectangle & r, int x, int y) const
{
	return r.inside(static_cast<float>(x), static_cast<float>(y));
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
void ofApp::mouseMoved(int x, int y)
{
	hoverLink = 0;
	if (hit(techLinkZone, x, y)) hoverLink = 1;
	else if (hit(artLinkZone, x, y)) hoverLink = 2;
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{
	if (hit(techLinkZone, x, y)) {
		ofLaunchBrowser(techUrl);
	} else if (hit(artLinkZone, x, y)) {
		ofLaunchBrowser(artUrl);
	}
}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer & buffer)
{
	rnbo.audioOut(buffer);
}
