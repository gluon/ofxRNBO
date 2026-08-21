// ofxRNBO
// Build shim: compiles the RNBO runtime translation unit from the user's export.
//
// Julien Bayle / Structure Void
// https://julienbayle.net
// https://structure-void.com
//
// MIT for this addon's glue. The RNBO runtime and your export remain under
// their own Cycling '74 license and are not part of this repository.
//
// RNBO.cpp is a unity file: it #includes every runtime source. Compiling this one
// shim pulls the whole runtime in as a single translation unit. Resolved through
// the -Irnbo-export/rnbo path set in config.make. Present only once the user has
// exported the patch into rnbo-export/.

#include "RNBO.cpp"
