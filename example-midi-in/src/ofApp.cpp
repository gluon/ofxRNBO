// ofxRNBO
// Example app implementation: mouse-played on-screen keyboard into a RNBO synth.
//
// Julien Bayle / Structure Void
// https://julienbayle.net
// https://structure-void.com
//
// MIT for this addon's glue. The RNBO runtime and your export remain under
// their own Cycling '74 license and are not part of this repository.

#include "ofApp.h"

#include <set>

// Visual charter, shared across all ofxRNBO examples.
// Dark anthracite ground, monospace type, one cool grey-green accent, thin lines.
namespace {
	const int   kWidth   = 640;
	const int   kHeight  = 600;
	const int   kMargin  = 48;
	const float kCharW   = 8.0f;   // oF bitmap font advance
	const float kLineH   = 14.0f;

	// On-screen keyboard, two octaves.
	const float kKbTop   = 100.0f;
	const float kKbH     = 66.0f;
	const int   kKbOctaves = 2;

	// Output scope band, below the parameters, above the footer. One sample per column.
	const int   kWaveLen = kWidth - 2 * kMargin;  // 544
	const float kWaveTop = 452.0f;
	const float kWaveH   = 56.0f;

	const int   kMaxParams = 6;   // cap on how many synth sliders are shown

	const ofColor kBackground(26, 28, 30);   // deep neutral grey, not pure black
	const ofColor kText(150, 156, 158);      // neutral grey
	const ofColor kDim(96, 101, 104);        // dimmed grey
	const ofColor kLine(58, 62, 65);         // thin rule
	const ofColor kAccent(122, 168, 148);    // cool grey-green, active elements
	const ofColor kWhiteKey(40, 43, 46);     // idle white key
	const ofColor kBlackKey(18, 20, 22);     // idle black key

	ofRectangle textZone(const std::string & s, float x, float y) {
		return ofRectangle(x - 2.0f, y - 11.0f, s.size() * kCharW + 4.0f, kLineH);
	}
}

//--------------------------------------------------------------
void ofApp::setup()
{
	ofSetWindowTitle("ofxRNBO / midi-in");
	ofSetWindowShape(kWidth, kHeight);
	ofSetVerticalSync(true);
	ofSetBackgroundAuto(true);

	// Pre-size the scope ring buffer so the audio callback never allocates.
	waveMono.assign(kWaveLen, 0.0f);

	// Build the on-screen keyboard rectangles once, the layout is fixed.
	buildKeyboard();

	ofSoundStreamSettings settings;
	settings.numOutputChannels = 2;
	settings.numInputChannels = 0;
	settings.sampleRate = 44100;
	settings.bufferSize = 512;
	settings.numBuffers = 4;

	if (!rnbo.setup(settings)) {
		// The on-screen "no export loaded" message covers this for the user.
		return;
	}

	// Take whatever parameters the synth exposes, up to the cap, and show a slider each.
	const std::size_t n = std::min<std::size_t>(rnbo.getNumParameters(), kMaxParams);
	for (std::size_t i = 0; i < n; ++i) {
		Param p;
		p.id = rnbo.getParameterId(i);
		p.index = i;
		RNBO::ParameterInfo info;
		if (rnbo.getParameterInfo(i, info)) {
			p.min = static_cast<float>(info.min);
			p.max = static_cast<float>(info.max);
			p.value = static_cast<float>(rnbo.getParameter(i));
		}
		params.push_back(p);
	}

	// Footer link zones, positioned once. Layout is fixed, the window does not resize.
	const float linksY = kHeight - 24.0f;
	const std::string techLabel = "tech structure-void.com";
	techLinkZone = textZone(techLabel, kMargin, linksY);
	artLinkZone = textZone("art julienbayle.net", kMargin + techLabel.size() * kCharW + 3 * kCharW, linksY);

	// Clickable octave-shift buttons, on the line under the keyboard.
	const float octaveY = kKbTop + kKbH + 26.0f;
	octaveDownZone = textZone("[oct-]", kMargin + 12 * kCharW, octaveY);
	octaveUpZone = textZone("[oct+]", kMargin + 19 * kCharW, octaveY);

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

	if (!rnbo.isReady()) {
		ofSetColor(kAccent);
		ofDrawBitmapString("no export loaded", kMargin, 122);
		ofSetColor(kDim);
		ofDrawBitmapString("export a RNBO patch into rnbo-export/ and rebuild", kMargin, 144);
		drawFooter();
		return;
	}

	drawKeyboard();
	drawParams();
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
	ofDrawBitmapString("ofxRNBO / midi-in", 0, 0);
	ofPopMatrix();
	ofSetDrawBitmapMode(OF_BITMAPMODE_SIMPLE);

	ofSetColor(kDim);
	ofDrawBitmapString("click the on-screen keyboard to play a polyphonic RNBO synth", kMargin, 68);

	ofSetColor(kLine);
	ofDrawLine(kMargin, 84, kWidth - kMargin, 84);
}

//--------------------------------------------------------------
void ofApp::buildKeyboard()
{
	// Fixed rectangles for the on-screen keys, the single source used for both drawing and
	// mouse hit-testing. Offsets are semitones above baseNote; the note played is resolved at
	// click time so an octave shift moves the whole keyboard.
	keys.clear();

	const float contentW = kWidth - 2.0f * kMargin;
	const float x0 = kMargin;

	static const int whiteSemi[7] = { 0, 2, 4, 5, 7, 9, 11 };       // C D E F G A B
	static const int blackSemi[5] = { 1, 3, 6, 8, 10 };             // C# D# F# G# A#
	static const int blackAfterWhite[5] = { 0, 1, 3, 4, 5 };        // which white key it sits after

	const int whites = 7 * kKbOctaves;
	const float ww = contentW / whites;

	for (int o = 0; o < kKbOctaves; ++o) {
		for (int i = 0; i < 7; ++i) {
			Key k;
			k.offset = o * 12 + whiteSemi[i];
			k.black = false;
			k.rect = ofRectangle(x0 + (o * 7 + i) * ww, kKbTop, ww - 1.0f, kKbH);
			keys.push_back(k);
		}
	}

	const float bw = ww * 0.62f;
	const float bh = kKbH * 0.6f;
	for (int o = 0; o < kKbOctaves; ++o) {
		for (int j = 0; j < 5; ++j) {
			Key k;
			k.offset = o * 12 + blackSemi[j];
			k.black = true;
			k.rect = ofRectangle(x0 + (o * 7 + blackAfterWhite[j] + 1) * ww - bw * 0.5f, kKbTop, bw, bh);
			keys.push_back(k);
		}
	}
}

//--------------------------------------------------------------
int ofApp::keyNoteAt(int x, int y) const
{
	// Black keys are tested first because they sit on top of the white keys.
	for (std::size_t i = 0; i < keys.size(); ++i) {
		if (keys[i].black && hit(keys[i].rect, x, y)) {
			return static_cast<int>(ofClamp(baseNote + keys[i].offset, 0, 127));
		}
	}
	for (std::size_t i = 0; i < keys.size(); ++i) {
		if (!keys[i].black && hit(keys[i].rect, x, y)) {
			return static_cast<int>(ofClamp(baseNote + keys[i].offset, 0, 127));
		}
	}
	return -1;
}

//--------------------------------------------------------------
void ofApp::drawKeyboard()
{
	// Which note offset is currently held by the mouse and falls on the drawn range. It lights up.
	std::set<int> lit;
	if (mouseNote >= 0) {
		const int off = mouseNote - baseNote;
		if (off >= 0 && off < 12 * kKbOctaves) {
			lit.insert(off);
		}
	}

	// White keys first, then black keys on top.
	for (std::size_t i = 0; i < keys.size(); ++i) {
		if (keys[i].black) {
			continue;
		}
		ofSetColor(lit.count(keys[i].offset) ? kAccent : kWhiteKey);
		ofFill();
		ofDrawRectangle(keys[i].rect);
		ofSetColor(kLine);
		ofNoFill();
		ofDrawRectangle(keys[i].rect);
		ofFill();
	}
	for (std::size_t i = 0; i < keys.size(); ++i) {
		if (!keys[i].black) {
			continue;
		}
		ofSetColor(lit.count(keys[i].offset) ? kAccent : kBlackKey);
		ofFill();
		ofDrawRectangle(keys[i].rect);
		ofSetColor(kLine);
		ofNoFill();
		ofDrawRectangle(keys[i].rect);
		ofFill();
	}

	// Current octave, with two clickable shift buttons on the same line.
	const int octave = baseNote / 12 - 1;
	const float octaveY = kKbTop + kKbH + 26.0f;
	ofSetColor(kText);
	ofDrawBitmapString("octave " + ofToString(octave), kMargin, octaveY);
	ofSetColor(kAccent);
	ofDrawBitmapString("[oct-]", kMargin + 12 * kCharW, octaveY);
	ofDrawBitmapString("[oct+]", kMargin + 19 * kCharW, octaveY);
}

//--------------------------------------------------------------
void ofApp::drawParams()
{
	const float contentW = kWidth - 2.0f * kMargin;
	// Sits well below the keyboard zone, so the parameters read as their own block.
	const float top = 262.0f;

	ofSetColor(kText);
	ofDrawBitmapString("parameters", kMargin, top - 16.0f);

	if (params.empty()) {
		ofSetColor(kDim);
		ofDrawBitmapString("this export exposes no parameters", kMargin, top + 6.0f);
		return;
	}

	for (std::size_t i = 0; i < params.size(); ++i) {
		Param & p = params[i];
		const float rowY = top + i * 30.0f;

		ofSetColor(kText);
		ofDrawBitmapString(p.id, kMargin, rowY);

		const std::string value = ofToString(p.value, 2);
		ofSetColor(kAccent);
		ofDrawBitmapString(value, kWidth - kMargin - value.size() * kCharW, rowY);

		const float barY = rowY + 7.0f;
		const float barH = 6.0f;
		p.bar = ofRectangle(kMargin, barY, contentW, barH);

		const float t = (p.max > p.min) ? ofClamp((p.value - p.min) / (p.max - p.min), 0.0f, 1.0f) : 0.0f;
		ofSetColor(kLine);
		ofNoFill();
		ofDrawRectangle(p.bar);
		ofFill();
		ofSetColor(kAccent);
		ofDrawRectangle(kMargin, barY, contentW * t, barH);
	}
}

//--------------------------------------------------------------
void ofApp::drawWaveform()
{
	if (!rnbo.isReady()) {
		return;
	}
	const int n = static_cast<int>(waveMono.size());
	if (n < 2) {
		return;
	}

	const float x0 = kMargin;
	const float centerY = kWaveTop + kWaveH * 0.5f;
	const float amp = kWaveH * 0.5f * 0.9f;

	ofSetColor(kLine);
	ofDrawLine(x0, centerY, x0 + n, centerY);

	ofSetLineWidth(1.0f);
	ofSetColor(kAccent);
	for (int i = 1; i < n; ++i) {
		const std::size_t a = (wavePos + i - 1) % n;
		const std::size_t b = (wavePos + i) % n;
		ofDrawLine(x0 + (i - 1), centerY - waveMono[a] * amp,
				   x0 + i,       centerY - waveMono[b] * amp);
	}
}

//--------------------------------------------------------------
void ofApp::drawFooter()
{
	const float contentW = kWidth - 2.0f * kMargin;

	ofSetColor(kLine);
	ofDrawLine(kMargin, kHeight - 64, kMargin + contentW, kHeight - 64);

	ofSetColor(kText);
	ofDrawBitmapString("Julien Bayle / Structure Void", kMargin, kHeight - 44);

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
void ofApp::setParamFromMouse(int index, int x)
{
	if (index < 0 || index >= static_cast<int>(params.size())) {
		return;
	}
	Param & p = params[index];
	const float contentW = kWidth - 2.0f * kMargin;
	const float t = ofClamp((x - kMargin) / contentW, 0.0f, 1.0f);
	p.value = p.min + t * (p.max - p.min);
	rnbo.setParameter(p.index, p.value);
}


//--------------------------------------------------------------
void ofApp::sendNoteOn(int note)
{
	rnbo.sendMidiNote(channel, note, velocity, true);
}

//--------------------------------------------------------------
void ofApp::sendNoteOff(int note)
{
	// Note-off carries velocity 0, the convention the official RNBO MIDI examples use.
	rnbo.sendMidiNote(channel, note, 0, false);
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
		return;
	}
	if (hit(artLinkZone, x, y)) {
		ofLaunchBrowser(artUrl);
		return;
	}
	// Octave shift buttons. They move the whole keyboard, not any note currently held.
	if (hit(octaveDownZone, x, y)) {
		baseNote = std::max(0, baseNote - 12);
		return;
	}
	if (hit(octaveUpZone, x, y)) {
		baseNote = std::min(108, baseNote + 12);
		return;
	}
	// On-screen keyboard, played with the mouse. One note per press, held until release.
	const int kbNote = keyNoteAt(x, y);
	if (kbNote >= 0) {
		mouseNote = kbNote;
		sendNoteOn(mouseNote);
		return;
	}
	for (std::size_t i = 0; i < params.size(); ++i) {
		// Widen the hit zone vertically so the thin bar is easy to grab.
		ofRectangle grab(params[i].bar.x, params[i].bar.y - 8, params[i].bar.width, params[i].bar.height + 16);
		if (hit(grab, x, y)) {
			draggingParam = static_cast<int>(i);
			setParamFromMouse(draggingParam, x);
			return;
		}
	}
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button)
{
	if (draggingParam >= 0) {
		setParamFromMouse(draggingParam, x);
		return;
	}
	// Glissando: while a mouse note is held, dragging onto another key stops the note being
	// left and starts the note being entered, one note at a time, the usual keyboard legato.
	// Dragging off the keyboard holds the current note until release.
	if (mouseNote >= 0) {
		const int n = keyNoteAt(x, y);
		if (n >= 0 && n != mouseNote) {
			sendNoteOff(mouseNote);
			sendNoteOn(n);
			mouseNote = n;
		}
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button)
{
	draggingParam = -1;
	// Release the note the mouse was holding, on the exact note it started.
	if (mouseNote >= 0) {
		sendNoteOff(mouseNote);
		mouseNote = -1;
	}
}

//--------------------------------------------------------------
bool ofApp::hit(const ofRectangle & r, int x, int y) const
{
	return r.inside(static_cast<float>(x), static_cast<float>(y));
}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer & buffer)
{
	rnbo.audioOut(buffer);

	// Feed the mono sum of the output into the scope ring buffer. Audio thread, no lock, no
	// allocation: the buffer is pre-sized in setup and only indexed here.
	const std::size_t frames = buffer.getNumFrames();
	const std::size_t chans = buffer.getNumChannels();
	const std::size_t n = waveMono.size();
	if (n == 0 || chans == 0) {
		return;
	}
	for (std::size_t f = 0; f < frames; ++f) {
		float sum = 0.0f;
		for (std::size_t c = 0; c < chans; ++c) {
			sum += buffer[f * chans + c];
		}
		waveMono[wavePos] = sum / static_cast<float>(chans);
		wavePos = (wavePos + 1) % n;
	}
}
