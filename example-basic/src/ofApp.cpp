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

	// Output scope band, between the controls and the footer. One sample per pixel column.
	const int   kWaveLen = kWidth - 2 * kMargin;  // 544
	const float kWaveTop = 232.0f;
	const float kWaveH   = 60.0f;

	const ofColor kBackground(26, 28, 30);   // deep neutral grey, not pure black
	const ofColor kText(150, 156, 158);      // neutral grey
	const ofColor kDim(96, 101, 104);        // dimmed grey
	const ofColor kLine(58, 62, 65);         // thin rule
	const ofColor kAccent(122, 168, 148);    // cool grey-green, active elements
	const ofColor kWaveR(140, 145, 148);     // light discreet grey, right channel

	ofRectangle textZone(const std::string & s, float x, float y) {
		// Zone around a bitmap string drawn with its baseline at (x, y).
		return ofRectangle(x - 2.0f, y - 11.0f, s.size() * kCharW + 4.0f, kLineH);
	}
}

//--------------------------------------------------------------
void ofApp::setup()
{
	ofSetWindowTitle("ofxRNBO / basic");
	ofSetWindowShape(kWidth, kHeight);
	ofSetVerticalSync(true);
	ofSetBackgroundAuto(true);

	// Pre-size the scope ring buffers so the audio callback never allocates.
	waveL.assign(kWaveLen, 0.0f);
	waveR.assign(kWaveLen, 0.0f);

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
	const float linksY = kHeight - 24.0f;
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
	drawHeader();
	drawParameter();
	drawWaveform();
	drawFooter();
}

//--------------------------------------------------------------
void ofApp::drawHeader()
{
	// Top zone: a plugin-style title, larger than the rest, then a short description, then a
	// thin separator. The title is the monospace bitmap font scaled up; MODEL bitmap mode makes
	// the transform apply to it.
	ofSetColor(kText);
	ofSetDrawBitmapMode(OF_BITMAPMODE_MODEL);
	ofPushMatrix();
	ofTranslate(kMargin, 42.0f);
	ofScale(2.0f, 2.0f);
	ofDrawBitmapString("ofxRNBO / basic", 0, 0);
	ofPopMatrix();
	ofSetDrawBitmapMode(OF_BITMAPMODE_SIMPLE);

	ofSetColor(kDim);
	ofDrawBitmapString("one scalar parameter, a sine that glides", kMargin, 68);

	ofSetColor(kLine);
	ofDrawLine(kMargin, 84, kWidth - kMargin, 84);
}

//--------------------------------------------------------------
void ofApp::drawParameter()
{
	const float contentW = kWidth - 2.0f * kMargin;

	if (!rnbo.isReady()) {
		ofSetColor(kAccent);
		ofDrawBitmapString("no export loaded", kMargin, 122);
		ofSetColor(kDim);
		ofDrawBitmapString("export a RNBO patch into rnbo-export/ and rebuild", kMargin, 144);
		return;
	}

	// Parameter id, left. Value, accent, right aligned.
	ofSetColor(kText);
	ofDrawBitmapString(paramId, kMargin, 120);

	if (paramIndex >= 0) {
		const std::string value = ofToString(paramValue, 2);
		ofSetColor(kAccent);
		ofDrawBitmapString(value, kWidth - kMargin - value.size() * kCharW, 120);

		// Slider, thin. Border grey, fill accent.
		const float y = 148.0f;
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
		ofDrawBitmapString("left / right  coarse      up / down  fine", kMargin, 208);
	} else {
		ofSetColor(kAccent);
		ofDrawBitmapString("parameter '" + paramId + "' not in this export", kMargin, 148);
	}
}

//--------------------------------------------------------------
void ofApp::drawWaveform()
{
	if (!rnbo.isReady()) {
		return;
	}

	const int n = static_cast<int>(waveL.size());
	if (n < 2) {
		return;
	}

	const float x0 = kMargin;
	const float centerY = kWaveTop + kWaveH * 0.5f;
	const float amp = kWaveH * 0.5f * 0.9f;

	// Faint baseline, charter thin line.
	ofSetColor(kLine);
	ofDrawLine(x0, centerY, x0 + n, centerY);

	ofSetLineWidth(1.0f);

	// Both channels share the same rectangle, drawn on top of each other, so the small
	// left/right offset shows. The patch delays the right channel, so R trails L.
	// Right first, light discreet grey, so the accent left reads on top.
	ofSetColor(kWaveR);
	for (int i = 1; i < n; ++i) {
		const std::size_t a = (wavePos + i - 1) % n;
		const std::size_t b = (wavePos + i) % n;
		ofDrawLine(x0 + (i - 1), centerY - waveR[a] * amp,
				   x0 + i,       centerY - waveR[b] * amp);
	}

	// Left channel, accent.
	ofSetColor(kAccent);
	for (int i = 1; i < n; ++i) {
		const std::size_t a = (wavePos + i - 1) % n;
		const std::size_t b = (wavePos + i) % n;
		ofDrawLine(x0 + (i - 1), centerY - waveL[a] * amp,
				   x0 + i,       centerY - waveL[b] * amp);
	}
}

//--------------------------------------------------------------
void ofApp::drawFooter()
{
	const float contentW = kWidth - 2.0f * kMargin;

	// Bottom zone: a thin separator, then the credits and the clickable links.
	ofSetColor(kLine);
	ofDrawLine(kMargin, kHeight - 64, kMargin + contentW, kHeight - 64);

	ofSetColor(kText);
	ofDrawBitmapString("Julien Bayle / Structure Void", kMargin, kHeight - 44);

	// Clickable links, accent. Brighter on hover, underlined.
	const float linksY = kHeight - 24.0f;
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

	// Feed the freshly produced output into the scope ring buffers. Audio thread, no lock,
	// no allocation: the buffers are pre-sized in setup and only indexed here.
	const std::size_t frames = buffer.getNumFrames();
	const std::size_t chans = buffer.getNumChannels();
	const std::size_t n = waveL.size();
	if (n == 0 || chans == 0) {
		return;
	}
	for (std::size_t f = 0; f < frames; ++f) {
		const float l = buffer[f * chans + 0];
		const float r = chans > 1 ? buffer[f * chans + 1] : l;
		waveL[wavePos] = l;
		waveR[wavePos] = r;
		wavePos = (wavePos + 1) % n;
	}
}
