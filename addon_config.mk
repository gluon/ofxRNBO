# ofxRNBO addon_config.mk
#
# This addon is glue only: the ofxRNBO wrapper in src/. It carries no RNBO runtime and
# no export. RNBO C++ runtime and exports are Cycling '74 licensed and are never part of
# this addon.
#
# The consumer (an example here, or your own oF project) provides the RNBO export and
# wires it in its own config.make: include paths into the export, the RNBO_NOJSONPRESETS
# define, and the two RNBO translation units. See example-basic/config.make and
# example-basic/README.md for the reference wiring the wrapper is built against.

meta:
	ADDON_NAME = ofxRNBO
	ADDON_DESCRIPTION = Bake a RNBO C++ export into a self-contained openFrameworks binary on ofSoundStream
	ADDON_AUTHOR = Julien Bayle / Structure Void
	ADDON_TAGS = "audio" "dsp" "rnbo" "sound"
	ADDON_URL = https://github.com/gluon/ofxRNBO

