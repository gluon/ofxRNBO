// ofxRNBO
// Example app implementation: text inport, amplitude params, envelope outport.
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
	const int   kHeight  = 560;
	const int   kMargin  = 48;
	const float kCharW   = 8.0f;   // oF bitmap font advance
	const float kLineH   = 14.0f;

	// Output scope band, below the envelope meter, above the footer. One sample per column.
	const int   kWaveLen = kWidth - 2 * kMargin;  // 544
	const float kWaveTop = 392.0f;
	const float kWaveH   = 56.0f;

	const ofColor kBackground(26, 28, 30);   // deep neutral grey, not pure black
	const ofColor kText(150, 156, 158);      // neutral grey
	const ofColor kDim(96, 101, 104);        // dimmed grey
	const ofColor kLine(58, 62, 65);         // thin rule
	const ofColor kAccent(122, 168, 148);    // cool grey-green, active elements, controls
	const ofColor kOutput(205, 216, 210);    // light tone, values read back from the patch

	ofRectangle textZone(const std::string & s, float x, float y) {
		return ofRectangle(x - 2.0f, y - 11.0f, s.size() * kCharW + 4.0f, kLineH);
	}
}

//--------------------------------------------------------------
void ofApp::setup()
{
	ofSetWindowTitle("ofxRNBO / messages");
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
						 << rnbo.getNumParameters() << " params, "
						 << rnbo.getNumOutports() << " outports";

	// Push the initial amplitudes so the patch matches the sliders. setParameterById is a
	// no-op if the export has no partial1..partial4, which keeps this harmless without a patch.
	for (int i = 0; i < kPartials; ++i) {
		rnbo.setParameterById("partial" + ofToString(i + 1), amp[i]);
	}

	// Text field zone, fixed. Clicking inside it gives the field keyboard focus.
	inputZone = ofRectangle(kMargin, 104.0f, kWidth - 2.0f * kMargin, 24.0f);

	// Pre-size the scope ring buffer so the audio callback never allocates.
	waveMono.assign(kWaveLen, 0.0f);

	// Footer link zones, positioned once. Layout is fixed, the window does not resize.
	const float linksY = kHeight - 24.0f;
	const std::string techLabel = "tech structure-void.com";
	techLinkZone = textZone(techLabel, kMargin, linksY);
	artLinkZone = textZone("art julienbayle.net", kMargin + techLabel.size() * kCharW + 3 * kCharW, linksY);

	settings.setOutListener(this);
	soundStream.setup(settings);
}

//--------------------------------------------------------------
void ofApp::update()
{
	// Drain RNBO events on the main thread, then read the latest envelope the patch sent out.
	rnbo.update();
	envLevel = static_cast<float>(rnbo.getOutportValue("envelope"));
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

	drawInput();
	drawPartials();
	drawEnvelope();
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
	ofDrawBitmapString("ofxRNBO / messages", 0, 0);
	ofPopMatrix();
	ofSetDrawBitmapMode(OF_BITMAPMODE_SIMPLE);

	ofSetColor(kDim);
	ofDrawBitmapString("type partials in, mix amplitudes, read the envelope out", kMargin, 68);

	ofSetColor(kLine);
	ofDrawLine(kMargin, 84, kWidth - kMargin, 84);
}

//--------------------------------------------------------------
void ofApp::drawInput()
{
	const float boxY = inputZone.y;
	const float textX = kMargin + 8.0f;
	const float baselineY = boxY + 16.0f;

	// Field outline: accent when focused, neutral grey otherwise.
	ofSetColor(inputFocused ? kAccent : kLine);
	ofNoFill();
	ofDrawRectangle(inputZone);
	ofFill();

	// Placeholder shows only when the field is empty and not focused.
	if (inputText.empty() && !inputFocused) {
		ofSetColor(kDim);
		ofDrawBitmapString("type partial frequencies, space separated (e.g. 50 100 200 220)",
						   textX, baselineY);
	} else {
		ofSetColor(kText);
		ofDrawBitmapString(inputText, textX, baselineY);
	}

	// Blinking caret at the insertion point, only when focused. Roughly 500 ms on, 500 ms off,
	// driven by elapsed time so it needs no external dependency. The caret sits at its index in
	// the text, not only at the end, so it shows where typing will insert.
	if (inputFocused) {
		const bool caretOn = std::fmod(ofGetElapsedTimef(), 1.0f) < 0.5f;
		if (caretOn) {
			const float caretX = textX + caret * kCharW;
			ofSetColor(kAccent);
			ofDrawLine(caretX, boxY + 5.0f, caretX, boxY + 19.0f);
		}
	}

	// Current frequencies, echoing what was last sent to the patch.
	ofSetColor(kDim);
	std::string cur = "sent: ";
	if (currentFreqs.empty()) {
		cur += "nothing yet, press enter to send";
	} else {
		for (std::size_t i = 0; i < currentFreqs.size(); ++i) {
			cur += ofToString(currentFreqs[i], 0) + (i + 1 < currentFreqs.size() ? " " : " Hz");
		}
	}
	ofDrawBitmapString(cur, kMargin, 152);
}

//--------------------------------------------------------------
void ofApp::drawPartials()
{
	const float contentW = kWidth - 2.0f * kMargin;

	ofSetColor(kText);
	ofDrawBitmapString("amplitudes", kMargin, 182);

	for (int i = 0; i < kPartials; ++i) {
		const float rowY = 202.0f + i * 36.0f;
		const std::string label = "partial" + ofToString(i + 1);

		ofSetColor(kText);
		ofDrawBitmapString(label, kMargin, rowY);

		const std::string value = ofToString(amp[i], 2);
		ofSetColor(kAccent);
		ofDrawBitmapString(value, kWidth - kMargin - value.size() * kCharW, rowY);

		const float barY = rowY + 10.0f;
		const float barH = 6.0f;
		partialBar[i] = ofRectangle(kMargin, barY, contentW, barH);

		ofSetColor(kLine);
		ofNoFill();
		ofDrawRectangle(partialBar[i]);
		ofFill();
		ofSetColor(kAccent);
		ofDrawRectangle(kMargin, barY, contentW * ofClamp(amp[i], 0.0f, 1.0f), barH);
	}
}

//--------------------------------------------------------------
void ofApp::drawEnvelope()
{
	const float contentW = kWidth - 2.0f * kMargin;
	const float rowY = 352.0f;

	ofSetColor(kText);
	ofDrawBitmapString("envelope", kMargin, rowY);

	// This value comes back from the patch on an outport, it is not a control. Draw it in the
	// light output tone, not the accent, so it reads as a reading and not a slider.
	const std::string value = ofToString(envLevel, 3);
	ofSetColor(kOutput);
	ofDrawBitmapString(value, kWidth - kMargin - value.size() * kCharW, rowY);

	// Level meter, same bar shape as the amplitude sliders but the light output tone, and it
	// moves with the value received. The envelope scale depends on the patch; clamp for the bar.
	const float barY = rowY + 10.0f;
	const float barH = 10.0f;
	ofSetColor(kLine);
	ofNoFill();
	ofDrawRectangle(kMargin, barY, contentW, barH);
	ofFill();
	ofSetColor(kOutput);
	ofDrawRectangle(kMargin, barY, contentW * ofClamp(envLevel, 0.0f, 1.0f), barH);
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
void ofApp::sendPartials()
{
	// Parse the typed line into a list of frequencies.
	currentFreqs.clear();
	std::vector<std::string> tokens = ofSplitString(inputText, " ", true, true);
	for (const std::string & t : tokens) {
		if (!t.empty()) {
			currentFreqs.push_back(static_cast<double>(ofToFloat(t)));
		}
	}
	if (currentFreqs.empty()) {
		return;
	}

	// Two ways to send a message into a RNBO patch, both through the same call with a
	// different tag, the destination name hashed to a message tag:
	//   - to a named inport, address it by its name. Here the patch has an [inport partials],
	//     so we send the list to "partials":
	rnbo.sendMessage("partials", currentFreqs);
	//   - to a numbered message input, an [in] object gets a default tag "in1", "in2", and so
	//     on in inlet order. Sending to the second message inlet would be:
	//         rnbo.sendMessage("in2", currentFreqs);
	//     This example uses the named inport; the numbered form is shown for reference.
}

//--------------------------------------------------------------
void ofApp::setPartialFromMouse(int slider, int x)
{
	if (slider < 0 || slider >= kPartials) {
		return;
	}
	const float contentW = kWidth - 2.0f * kMargin;
	const float t = ofClamp((x - kMargin) / contentW, 0.0f, 1.0f);
	amp[slider] = t;
	rnbo.setParameterById("partial" + ofToString(slider + 1), t);
}

//--------------------------------------------------------------
void ofApp::setCaretFromMouse(int x)
{
	// Nearest character boundary to the click, measured with the fixed monospace advance.
	const float textX = kMargin + 8.0f;
	const float rel = (static_cast<float>(x) - textX) / kCharW;
	long idx = std::lround(rel);
	if (idx < 0) {
		idx = 0;
	}
	if (idx > static_cast<long>(inputText.size())) {
		idx = static_cast<long>(inputText.size());
	}
	caret = static_cast<std::size_t>(idx);
}

//--------------------------------------------------------------
void ofApp::insertAtCaret(char c)
{
	inputText.insert(inputText.begin() + caret, c);
	caret += 1;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
	// The field writes only when focused. Click it first to type.
	if (!inputFocused) {
		return;
	}

	switch (key) {
		case OF_KEY_RETURN:
			sendPartials();
			return;
		case OF_KEY_LEFT:
			if (caret > 0) caret -= 1;
			return;
		case OF_KEY_RIGHT:
			if (caret < inputText.size()) caret += 1;
			return;
		case OF_KEY_HOME:
			caret = 0;
			return;
		case OF_KEY_END:
			caret = inputText.size();
			return;
		case OF_KEY_BACKSPACE:
			// Erase the character before the caret.
			if (caret > 0) {
				inputText.erase(inputText.begin() + (caret - 1));
				caret -= 1;
			}
			return;
		case OF_KEY_DEL:
			// Erase the character after the caret.
			if (caret < inputText.size()) {
				inputText.erase(inputText.begin() + caret);
			}
			return;
		default:
			break;
	}

	// Accept digits, spaces, a decimal point, and a sign. Enough to type frequencies.
	if ((key >= '0' && key <= '9') || key == ' ' || key == '.' || key == '-') {
		insertAtCaret(static_cast<char>(key));
	}
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
	// Focus follows the click: inside the field gains focus, anywhere else loses it.
	inputFocused = hit(inputZone, x, y);
	if (inputFocused) {
		// Place the caret at the character nearest the click.
		setCaretFromMouse(x);
		return;
	}

	if (hit(techLinkZone, x, y)) {
		ofLaunchBrowser(techUrl);
		return;
	}
	if (hit(artLinkZone, x, y)) {
		ofLaunchBrowser(artUrl);
		return;
	}
	for (int i = 0; i < kPartials; ++i) {
		// Widen the hit zone vertically so the thin bar is easy to grab.
		ofRectangle grab(partialBar[i].x, partialBar[i].y - 8, partialBar[i].width, partialBar[i].height + 16);
		if (hit(grab, x, y)) {
			draggingSlider = i;
			setPartialFromMouse(i, x);
			return;
		}
	}
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button)
{
	if (draggingSlider >= 0) {
		setPartialFromMouse(draggingSlider, x);
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button)
{
	draggingSlider = -1;
}

//--------------------------------------------------------------
bool ofApp::hit(const ofRectangle & r, int x, int y) const
{
	return r.inside(static_cast<float>(x), static_cast<float>(y));
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

	// Faint baseline, charter thin line.
	ofSetColor(kLine);
	ofDrawLine(x0, centerY, x0 + n, centerY);

	// Single mono trace, accent. Oldest sample on the left, newest on the right.
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
void ofApp::audioOut(ofSoundBuffer & buffer)
{
	rnbo.audioOut(buffer);

	// Feed the mono sum of the output into the scope ring buffer. Audio thread, no lock, no
	// allocation: the buffer is pre-sized in setup and only indexed here. The two channels
	// carry the same signal here, so their average is one representative trace.
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
