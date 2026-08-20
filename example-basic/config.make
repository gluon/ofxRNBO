################################################################################
# CONFIGURE PROJECT MAKEFILE (optional)
#   This file configures the example project. It wires the RNBO export that the
#   user drops into example-basic/rnbo-export/ (git-ignored, Cycling '74 runtime).
#   The ofxRNBO addon itself is glue only and carries none of this.
################################################################################

################################################################################
# RNBO EXPORT WIRING
#   Include paths into the user's export, plus the header-bug workaround define.
#   Paths are relative to this example directory, where make runs.
################################################################################

# RNBO_NOJSONPRESETS drops the JSON preset helpers in RNBO_Presets.h. In RNBO 1.4.5
# those helpers fail to compile under the strict C++ standard oF uses (a non-const
# reference bound to a temporary in StateHelper::operator=), and they are static so
# every translation unit including RNBO.h hits it. The wrapper does not use JSON
# presets; the non-JSON CoreObject preset API is unaffected. See PITFALLS.md.
PROJECT_DEFINES = RNBO_NOJSONPRESETS

# Include paths into the export. Order:
#   rnbo-export         resolves the shim include of rnbo_source.cpp
#   rnbo-export/rnbo    resolves "RNBO.h", "common/...", "src/..." and the RNBO.cpp shim
#   rnbo-export/rnbo/common  resolves the bare "RNBO_Common.h" that rnbo_source.cpp includes
#   rnbo-export/rnbo/src     resolves the "3rdparty/..." headers RNBO uses internally
PROJECT_CFLAGS = -Irnbo-export -Irnbo-export/rnbo -Irnbo-export/rnbo/common -Irnbo-export/rnbo/src

# Sample precision. RNBO::SampleValue is double by default, RNBO's recommended
# precision. oF samples are float, converted during the de/interleave the wrapper
# already does, at no extra pass. To force 32-bit samples, add RNBO_USE_FLOAT32 to
# PROJECT_DEFINES above AND regenerate the export with the same setting; the define
# must match on both sides or the audio buffers are misread.

################################################################################
# OF ROOT
#   The location of your root openFrameworks installation
#       (default) OF_ROOT = ../../..
################################################################################
# OF_ROOT = ../../..

################################################################################
# PROJECT ROOT
#       (default) PROJECT_ROOT = . (this directory)
################################################################################
# PROJECT_ROOT = .

################################################################################
# PROJECT SPECIFIC CHECKS
################################################################################
# PROJECT_EXTERNAL_SOURCE_PATHS =

################################################################################
# PROJECT LINKER FLAGS
################################################################################
# PROJECT_LDFLAGS = -Wl,-rpath=./libs

################################################################################
# PROJECT OPTIMIZATION CFLAGS
################################################################################
# PROJECT_OPTIMIZATION_CFLAGS_RELEASE =
# PROJECT_OPTIMIZATION_CFLAGS_DEBUG =

################################################################################
# PROJECT COMPILERS
################################################################################
# PROJECT_CXX =
# PROJECT_CC =
