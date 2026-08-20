// ofxRNBO
// Wrapper implementation: CoreObject setup, block chunking, de/interleave, parameters, MIDI.
//
// Julien Bayle / Structure Void
// https://julienbayle.net
// https://structure-void.com
//
// MIT for this addon's glue. The RNBO runtime and your export remain under
// their own Cycling '74 license and are not part of this repository.

#include "ofxRNBO.h"

ofxRNBO::ofxRNBO()
{
	// Attach this object as the event handler and keep the returned interface alive.
	// createParameterInterface returns a UniquePtr; if it is dropped the managed interface
	// is destroyed and the handler stops receiving events, with no error. Hold it as a member.
	paramInterface = rnbo.createParameterInterface(
		RNBO::ParameterEventInterface::MultiProducer, this);
}

ofxRNBO::~ofxRNBO() = default;

bool ofxRNBO::setup(const ofSoundStreamSettings & settings, std::size_t maxBlockSize)
{
	return setup(static_cast<double>(settings.sampleRate),
				 settings.bufferSize,
				 settings.numInputChannels,
				 maxBlockSize);
}

bool ofxRNBO::setup(double sampleRate, std::size_t bufferSize, std::size_t /*numInputChannels*/,
					std::size_t maxBlockSize)
{
	if (maxBlockSize == 0) {
		ofLogError("ofxRNBO") << "maxBlockSize must be > 0";
		return false;
	}

	sampleRate_ = sampleRate;
	maxBlockSize_ = maxBlockSize;

	// Channel layout comes from the export itself, not from a hardcoded guess.
	// The number of process buffers RNBO expects is audio channels plus signal parameters:
	// getNumInputChannels + getNumSignalInParameters in, and likewise for outputs.
	audioIn_ = rnbo.getNumInputChannels();
	audioOut_ = rnbo.getNumOutputChannels();
	signalIn_ = rnbo.getNumSignalInParameters();
	signalOut_ = rnbo.getNumSignalOutParameters();
	inSlots_ = audioIn_ + signalIn_;
	outSlots_ = audioOut_ + signalOut_;

	buildParameterMaps();
	allocateScratch(bufferSize);

	// One fixed, generous max block size, set once. Never call prepareToProcess per block:
	// on a non-fixed-vector export, shrinking the block size lowers the patcher's reported
	// max vector size without freeing the buffers, so that reported bound stops matching the
	// real allocation and can no longer be trusted as an upper limit.
	// force = true so every patcher object's dspsetup runs now.
	if (!rnbo.prepareToProcess(sampleRate_, static_cast<RNBO::Index>(maxBlockSize_), true)) {
		ofLogError("ofxRNBO") << "prepareToProcess returned false, not ready";
		ready = false;
		return false;
	}

	ready = true;
	return true;
}

void ofxRNBO::allocateScratch(std::size_t bufferSize)
{
	// Input staging must hold a whole oF input buffer, deinterleaved. oF can hand a block
	// larger than the nominal buffer size, so keep at least maxBlockSize of headroom.
	stagingFrames_ = std::max(bufferSize, maxBlockSize_);

	inStaging.assign(inSlots_ * stagingFrames_, RNBO::SampleValue(0));
	outScratch.assign(outSlots_ * maxBlockSize_, RNBO::SampleValue(0));

	inPtrs.assign(inSlots_, nullptr);
	outPtrs.assign(outSlots_, nullptr);
	for (std::size_t c = 0; c < outSlots_; ++c) {
		outPtrs[c] = outScratch.data() + c * maxBlockSize_;
	}
}

void ofxRNBO::buildParameterMaps()
{
	idToIndex.clear();
	nameToIndex.clear();

	const std::size_t n = getNumParameters();
	for (std::size_t i = 0; i < n; ++i) {
		const std::string id = getParameterId(i);
		const std::string name = getParameterName(i);

		// Ids are unique. A duplicate would mean a broken export; keep the first and shout.
		if (!idToIndex.emplace(id, i).second) {
			ofLogWarning("ofxRNBO") << "duplicate parameter id '" << id
									<< "', keeping index " << idToIndex[id]
									<< " ignoring index " << i;
		}

		// Names are NOT unique. Collision policy: lowest index wins, logged once.
		auto it = nameToIndex.find(name);
		if (it == nameToIndex.end()) {
			nameToIndex.emplace(name, i);
		} else {
			ofLogWarning("ofxRNBO") << "parameter name '" << name
									<< "' is not unique, keeping index " << it->second
									<< " ignoring index " << i
									<< "; use the parameter id to disambiguate";
		}
	}
}

// -------------------------------------------------------------------------- audio

void ofxRNBO::audioIn(ofSoundBuffer & buffer)
{
	if (!ready || audioIn_ == 0) {
		return;
	}

	const std::size_t frames = buffer.getNumFrames();
	const std::size_t bufChans = buffer.getNumChannels();
	const std::size_t usable = std::min(frames, stagingFrames_);

	// Deinterleave the real audio input channels into the front of the staging region.
	for (std::size_t c = 0; c < audioIn_; ++c) {
		RNBO::SampleValue * dst = inStaging.data() + c * stagingFrames_;
		if (c < bufChans) {
			for (std::size_t f = 0; f < usable; ++f) {
				dst[f] = static_cast<RNBO::SampleValue>(buffer[f * bufChans + c]);
			}
		} else {
			for (std::size_t f = 0; f < usable; ++f) {
				dst[f] = RNBO::SampleValue(0);
			}
		}
	}
}

void ofxRNBO::audioOut(ofSoundBuffer & buffer)
{
	const std::size_t frames = buffer.getNumFrames();
	const std::size_t bufChans = buffer.getNumChannels();

	if (!ready) {
		buffer.set(0);
		return;
	}

	if (frames > stagingFrames_ && audioIn_ > 0 && !warnedOversizeInput) {
		ofLogWarning("ofxRNBO") << "audio buffer of " << frames
								<< " frames exceeds input staging of " << stagingFrames_
								<< ", extra input frames read as silence";
		warnedOversizeInput = true;
	}

	// Chunk so no single process call ever exceeds maxBlockSize_. CoreObject::process does no
	// clamping: too many frames means silence on a fixed-vector export, or a buffer overrun on
	// a non-fixed one, both silent. Staying at or under the prepared max avoids that.
	std::size_t offset = 0;
	while (offset < frames) {
		const std::size_t n = std::min(maxBlockSize_, frames - offset);
		processChunk(offset, n);

		// Interleave the real audio output channels back into the oF buffer.
		for (std::size_t c = 0; c < bufChans; ++c) {
			if (c < audioOut_) {
				const RNBO::SampleValue * src = outPtrs[c];
				for (std::size_t f = 0; f < n; ++f) {
					buffer[(offset + f) * bufChans + c] = static_cast<float>(src[f]);
				}
			} else {
				// oF asked for more channels than the patcher produces: silence them.
				for (std::size_t f = 0; f < n; ++f) {
					buffer[(offset + f) * bufChans + c] = 0.0f;
				}
			}
		}

		offset += n;
	}
}

void ofxRNBO::processChunk(std::size_t stagingOffset, std::size_t frames)
{
	// Input pointers: real audio-in channels point into the staging buffer at this chunk's
	// offset. Signal-parameter input slots stay null on purpose: the generated patcher falls
	// back to the parameter's scalar value when its input pointer is null, so a param~ is
	// driven by setParameter rather than by an audio signal.
	for (std::size_t c = 0; c < inSlots_; ++c) {
		if (c < audioIn_ && stagingOffset < stagingFrames_) {
			inPtrs[c] = inStaging.data() + c * stagingFrames_ + stagingOffset;
		} else {
			inPtrs[c] = nullptr;
		}
	}

	// Bind the exact argument types of the primary, non-converting process overload, so the
	// templated converting overload is never selected. That converting overload allocates on
	// the calling thread whenever the channel or frame count changes; we do our own
	// de/interleave against pre-allocated scratch instead, to keep the audio callback alloc free.
	const RNBO::SampleValue * const * ins =
		inSlots_ > 0 ? inPtrs.data() : nullptr;
	RNBO::SampleValue * const * outs = outPtrs.data();

	rnbo.process(ins, static_cast<RNBO::Index>(inSlots_),
				 outs, static_cast<RNBO::Index>(outSlots_),
				 static_cast<RNBO::Index>(frames));
}

void ofxRNBO::update()
{
	// Drain on the main thread, only when the audio thread has flagged events waiting.
	// Go through the inherited EventHandler::drainEvents, which forwards to the interface
	// linked at construction; the interface's own drainEvents is private and cannot be called.
	if (pendingEvents.exchange(false)) {
		drainEvents();
	}
}

void ofxRNBO::eventsAvailable()
{
	// Called on the audio thread. Draining here would block audio, so just flag it and let
	// update() drain on the main thread.
	pendingEvents.store(true);
}

// ---------------------------------------------------------------------- parameters

std::size_t ofxRNBO::getNumParameters() const
{
	return static_cast<std::size_t>(rnbo.getNumParameters());
}

std::string ofxRNBO::getParameterId(std::size_t index) const
{
	const char * id = rnbo.getParameterId(static_cast<RNBO::ParameterIndex>(index));
	return id ? std::string(id) : std::string();
}

std::string ofxRNBO::getParameterName(std::size_t index) const
{
	const char * name = rnbo.getParameterName(static_cast<RNBO::ParameterIndex>(index));
	return name ? std::string(name) : std::string();
}

bool ofxRNBO::getParameterInfo(std::size_t index, RNBO::ParameterInfo & info) const
{
	if (index >= getNumParameters()) {
		return false;
	}
	rnbo.getParameterInfo(static_cast<RNBO::ParameterIndex>(index), &info);
	return true;
}

int ofxRNBO::getParameterIndexForId(const std::string & id) const
{
	auto it = idToIndex.find(id);
	return it == idToIndex.end() ? -1 : static_cast<int>(it->second);
}

int ofxRNBO::getParameterIndexForName(const std::string & name) const
{
	auto it = nameToIndex.find(name);
	return it == nameToIndex.end() ? -1 : static_cast<int>(it->second);
}

void ofxRNBO::setParameter(std::size_t index, double value)
{
	// Time is RNBOTimeNow, meaning as soon as possible. Passing any other value does not
	// schedule the change in the future, it moves patcher time to that value; scheduling ahead
	// is done through the engine instead.
	rnbo.setParameterValue(static_cast<RNBO::ParameterIndex>(index),
						   static_cast<RNBO::ParameterValue>(value),
						   RNBO::RNBOTimeNow);
}

void ofxRNBO::setParameterNormalized(std::size_t index, double normalized)
{
	// Go through the normalized setter so any nonlinear parameter scaling, an exponent or a
	// custom curve defined in the patch, is honoured instead of a plain min/max interpolation.
	rnbo.setParameterValueNormalized(static_cast<RNBO::ParameterIndex>(index),
									 static_cast<RNBO::ParameterValue>(normalized),
									 RNBO::RNBOTimeNow);
}

double ofxRNBO::getParameter(std::size_t index) const
{
	return static_cast<double>(
		const_cast<RNBO::CoreObject &>(rnbo).getParameterValue(
			static_cast<RNBO::ParameterIndex>(index)));
}

double ofxRNBO::getParameterNormalized(std::size_t index) const
{
	RNBO::CoreObject & obj = const_cast<RNBO::CoreObject &>(rnbo);
	const RNBO::ParameterIndex i = static_cast<RNBO::ParameterIndex>(index);
	// CoreObject has no getParameterNormalized; read the real value and convert it to normalized.
	return static_cast<double>(
		obj.convertToNormalizedParameterValue(i, obj.getParameterValue(i)));
}

bool ofxRNBO::setParameterById(const std::string & id, double value)
{
	const int i = getParameterIndexForId(id);
	if (i < 0) {
		return false;
	}
	setParameter(static_cast<std::size_t>(i), value);
	return true;
}

bool ofxRNBO::setParameterByName(const std::string & name, double value)
{
	const int i = getParameterIndexForName(name);
	if (i < 0) {
		return false;
	}
	setParameter(static_cast<std::size_t>(i), value);
	return true;
}

bool ofxRNBO::setParameterNormalizedById(const std::string & id, double normalized)
{
	const int i = getParameterIndexForId(id);
	if (i < 0) {
		return false;
	}
	setParameterNormalized(static_cast<std::size_t>(i), normalized);
	return true;
}

bool ofxRNBO::setParameterNormalizedByName(const std::string & name, double normalized)
{
	const int i = getParameterIndexForName(name);
	if (i < 0) {
		return false;
	}
	setParameterNormalized(static_cast<std::size_t>(i), normalized);
	return true;
}

// ---------------------------------------------------------------------------- MIDI

std::size_t ofxRNBO::getNumMidiInputPorts() const
{
	return static_cast<std::size_t>(
		const_cast<RNBO::CoreObject &>(rnbo).getNumMidiInputPorts());
}

bool ofxRNBO::sendMidi(const unsigned char * bytes, std::size_t length, int port)
{
	if (bytes == nullptr || length == 0) {
		return false;
	}
	// RNBO::MidiEvent stores at most 3 bytes and truncates anything longer silently, so sysex
	// and other long messages cannot pass. Refuse rather than truncate, so a caller never
	// thinks a long message went through.
	if (length > 3) {
		ofLogWarning("ofxRNBO") << "MIDI message of " << length
								<< " bytes refused, RNBO::MidiEvent holds at most 3";
		return false;
	}
	rnbo.scheduleEvent(RNBO::MidiEvent(
		RNBO::RNBOTimeNow, port,
		static_cast<RNBO::ConstByteArray>(bytes),
		static_cast<RNBO::Index>(length)));
	return true;
}

bool ofxRNBO::sendMidiNote(int channel, int pitch, int velocity, bool noteOn, int port)
{
	const unsigned char status =
		static_cast<unsigned char>((noteOn ? 0x90 : 0x80) | (channel & 0x0F));
	const unsigned char bytes[3] = {
		status,
		static_cast<unsigned char>(pitch & 0x7F),
		static_cast<unsigned char>(velocity & 0x7F),
	};
	return sendMidi(bytes, 3, port);
}
