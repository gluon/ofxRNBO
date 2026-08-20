# ofxRNBO addon_config.mk
#
# This addon compiles a RNBO C++ export into the openFrameworks binary. The RNBO runtime
# and the exported patcher are Cycling '74 licensed and are NOT part of this addon. You
# drop your own export into  rnbo-export/  at the addon root (git-ignored, never shipped).
#
# Expected layout, relative to this addon:
#   rnbo-export/rnbo_source.cpp        the exported patcher
#   rnbo-export/rnbo/RNBO.cpp          the RNBO runtime, one translation unit
#   rnbo-export/rnbo/RNBO.h
#   rnbo-export/rnbo/common/*.h
#   rnbo-export/rnbo/src/*.h  (+ 3rdparty/)

meta:
	ADDON_NAME = ofxRNBO
	ADDON_DESCRIPTION = Bake a RNBO C++ export into a self-contained openFrameworks binary on ofSoundStream
	ADDON_AUTHOR = Julien Bayle / Structure Void
	ADDON_TAGS = "audio" "dsp" "rnbo" "sound"
	ADDON_URL = https://github.com/gluon/ofxRNBO

common:
	# Sample precision. RNBO::SampleValue is double by default, which is RNBO's recommended
	# precision. oF samples are float, so we convert during the de/interleave we already do,
	# at no extra pass. To force 32-bit samples instead, uncomment the next line AND regenerate
	# the export with the same setting. RNBO_USE_FLOAT32 changes the sample type in the RNBO
	# headers, so the addon and the export must agree; a mismatch corrupts the audio buffers.
	# ADDON_DEFINES += RNBO_USE_FLOAT32

	# Workaround for a RNBO 1.4.5 header bug. Under the strict C++ standard oF compiles
	# with, StateHelper::operator= in RNBO_PatcherStateInterface.h fails to compile when the
	# JSON preset helpers in RNBO_Presets.h are instantiated (a non-const reference bound to
	# a temporary). Those helpers are static, so every translation unit that includes RNBO.h
	# hits it. RNBO_NOJSONPRESETS drops exactly those JSON preset (de)serialization helpers,
	# which this addon does not use; the non-JSON CoreObject preset API is unaffected.
	# Remove this once the export is regenerated with a RNBO version that fixes the header.
	# See PITFALLS.md.
	ADDON_DEFINES += RNBO_NOJSONPRESETS
	
	# RNBO include paths, inside the user-provided export. Order matters:
	#  rnbo        resolves "RNBO.h", "common/...", "src/..."
	#  rnbo/common resolves the bare "RNBO_Common.h" that rnbo_source.cpp includes
	#  rnbo/src    resolves the "3rdparty/..." headers used internally
	ADDON_INCLUDES += rnbo-export/rnbo
	ADDON_INCLUDES += rnbo-export/rnbo/common
	ADDON_INCLUDES += rnbo-export/rnbo/src
	
	# Keep the include parser from walking the export tree on its own.
	ADDON_INCLUDES_EXCLUDE += rnbo-export/rnbo/test
	ADDON_INCLUDES_EXCLUDE += rnbo-export/rnbo/adapters/%
	ADDON_INCLUDES_EXCLUDE += rnbo-export/rnbo/externals/%
	
	# RNBO sources. RNBO.cpp #includes every other runtime .cpp, so compile only these two
	# translation units, never the individual src/*.cpp files. They live outside src/ and
	# libs/, so the addon parser does not pick them up automatically; add them explicitly.
	ADDON_SOURCES += rnbo-export/rnbo/RNBO.cpp
	ADDON_SOURCES += rnbo-export/rnbo_source.cpp

