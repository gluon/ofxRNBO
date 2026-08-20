// ofxRNBO
// Example entry point.
//
// Julien Bayle / Structure Void
// https://julienbayle.net
// https://structure-void.com
//
// MIT for this addon's glue. The RNBO runtime and your export remain under
// their own Cycling '74 license and are not part of this repository.

#include "ofMain.h"
#include "ofApp.h"

int main()
{
	ofGLWindowSettings settings;
	settings.setSize(720, 360);
	settings.windowMode = OF_WINDOW;

	auto window = ofCreateWindow(settings);
	ofRunApp(window, std::make_shared<ofApp>());
	ofRunMainLoop();
}
