// ofxRNBO
// Build shim: compiles the exported patcher translation unit from the user's export.
//
// Julien Bayle / Structure Void
// https://julienbayle.net
// https://structure-void.com
//
// MIT for this addon's glue. The RNBO runtime and your export remain under
// their own Cycling '74 license and are not part of this repository.
//
// rnbo_source.cpp is the generated patcher. Export with RNBO's default name so it
// lands as rnbo-export/rnbo_source.cpp. Resolved through the -Irnbo-export path set
// in config.make. Present only once the user has exported the patch.

#include "rnbo_source.cpp"
