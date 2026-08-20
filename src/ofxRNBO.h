#pragma once

// ofxRNBO
// Bake a RNBO C++ export into a self-contained openFrameworks binary, on ofSoundStream.
//
// Julien Bayle / Structure Void
// https://julienbayle.net
// https://structure-void.com
//
// MIT for this addon's glue. The RNBO runtime and your export remain under
// their own Cycling '74 license and are not part of this repository.

#include "ofMain.h"

#include "RNBO.h"

#include <atomic>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

class ofxRNBO : public RNBO::EventHandler {
public:
	/// Fixed maximum block size handed to prepareToProcess once, in setup.
	/// Every call to CoreObject::process is guaranteed to be at most this many frames.
	/// process does not clamp: exceeding it means silence on a fixed-vector export, or a
	/// buffer overrun on a non-fixed one, both silent, so the addon never exceeds it.
	static const std::size_t DefaultMaxBlockSize = 4096;

	ofxRNBO();
	~ofxRNBO() override;

	ofxRNBO(const ofxRNBO &) = delete;
	ofxRNBO & operator=(const ofxRNBO &) = delete;

	/// Prepare the RNBO object from an ofSoundStreamSettings.
	/// Call before ofSoundStream::setup. Returns false if RNBO refused to prepare.
	bool setup(const ofSoundStreamSettings & settings, std::size_t maxBlockSize = DefaultMaxBlockSize);

	/// Same, without an ofSoundStreamSettings to hand.
	/// bufferSize is the block size oF is expected to deliver, used only to size the
	/// input staging buffer. It does not have to match maxBlockSize.
	bool setup(double sampleRate, std::size_t bufferSize, std::size_t numInputChannels,
			   std::size_t maxBlockSize = DefaultMaxBlockSize);

	bool isReady() const { return ready; }

	/// ofSoundStream input callback. Deinterleaves into the staging buffer.
	/// Safe to leave unwired for output-only patches.
	void audioIn(ofSoundBuffer & buffer);

	/// ofSoundStream output callback. Chunks, runs RNBO, interleaves back into buffer.
	void audioOut(ofSoundBuffer & buffer);

	/// Call from the oF main thread, typically in ofApp::update.
	/// Drains RNBO's event queue. Never call this from the audio thread.
	void update();

	/* Channel layout, as reported by the export itself */

	std::size_t getNumInputChannels() const { return audioIn_; }
	std::size_t getNumOutputChannels() const { return audioOut_; }
	std::size_t getNumSignalInParameters() const { return signalIn_; }
	std::size_t getNumSignalOutParameters() const { return signalOut_; }

	/* Parameters */

	std::size_t getNumParameters() const;
	std::string getParameterId(std::size_t index) const;
	std::string getParameterName(std::size_t index) const;
	bool getParameterInfo(std::size_t index, RNBO::ParameterInfo & info) const;

	/// Index for a parameter id. Ids are unique. Returns -1 if unknown.
	int getParameterIndexForId(const std::string & id) const;

	/// Index for a parameter name. Names are NOT unique: when two parameters share a
	/// name the lowest index wins and the collision is logged once at setup time.
	/// Prefer ids. Returns -1 if unknown.
	int getParameterIndexForName(const std::string & name) const;

	void setParameter(std::size_t index, double value);
	void setParameterNormalized(std::size_t index, double normalized);
	double getParameter(std::size_t index) const;
	double getParameterNormalized(std::size_t index) const;

	bool setParameterById(const std::string & id, double value);
	bool setParameterByName(const std::string & name, double value);
	bool setParameterNormalizedById(const std::string & id, double normalized);
	bool setParameterNormalizedByName(const std::string & name, double normalized);

	/* MIDI in */

	std::size_t getNumMidiInputPorts() const;

	/// Send raw MIDI bytes to the patcher.
	/// RNBO::MidiEvent holds at most 3 bytes and silently truncates anything longer, so sysex
	/// cannot pass. Longer messages are refused here rather than silently cut.
	bool sendMidi(const unsigned char * bytes, std::size_t length, int port = 0);
	bool sendMidiNote(int channel, int pitch, int velocity, bool noteOn, int port = 0);

	/// Direct access, for anything this wrapper does not cover.
	RNBO::CoreObject & getCoreObject() { return rnbo; }

private:
	// RNBO::EventHandler. Called on the audio thread, so it must not drain.
	void eventsAvailable() override;

	void buildParameterMaps();
	void allocateScratch(std::size_t bufferSize);
	void processChunk(std::size_t stagingOffset, std::size_t frames);

	RNBO::CoreObject rnbo;

	// Declared after rnbo so it is destroyed first. Holding this is mandatory:
	// createParameterInterface returns a UniquePtr, and if it dies the managed interface is
	// destroyed and the EventHandler stops receiving events, with no error.
	RNBO::ParameterEventInterfaceUniquePtr paramInterface;

	bool ready = false;
	double sampleRate_ = 0.0;
	std::size_t maxBlockSize_ = 0;

	std::size_t audioIn_ = 0;
	std::size_t audioOut_ = 0;
	std::size_t signalIn_ = 0;
	std::size_t signalOut_ = 0;
	std::size_t inSlots_ = 0;   // audioIn_ + signalIn_
	std::size_t outSlots_ = 0;  // audioOut_ + signalOut_

	// Non-interleaved input staging, one contiguous region per channel of
	// stagingFrames_ samples. Written by audioIn, read chunk by chunk by audioOut.
	std::vector<RNBO::SampleValue> inStaging;
	std::size_t stagingFrames_ = 0;

	// Non-interleaved output scratch, one region per slot of maxBlockSize_ samples.
	std::vector<RNBO::SampleValue> outScratch;

	// Pointer arrays handed to CoreObject::process. Signal parameter slots stay null
	// on purpose so the patcher falls back to their scalar value.
	std::vector<RNBO::SampleValue *> inPtrs;
	std::vector<RNBO::SampleValue *> outPtrs;

	std::unordered_map<std::string, std::size_t> idToIndex;
	std::unordered_map<std::string, std::size_t> nameToIndex;

	std::atomic<bool> pendingEvents{false};
	bool warnedOversizeInput = false;
};
