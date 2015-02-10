/*
 * SynthesizerRenderer.h
 *
 *  Created on: 02.11.2014
 *      Author: michi
 */

#ifndef SRC_AUDIO_SYNTH_SYNTHESIZERRENDERER_H_
#define SRC_AUDIO_SYNTH_SYNTHESIZERRENDERER_H_


#include "../AudioRenderer.h"

class Synthesizer;
class MidiEvent;
class MidiSource;

class SynthesizerRenderer : public AudioRendererInterface
{
public:
	SynthesizerRenderer(Synthesizer *s);
	virtual ~SynthesizerRenderer();

	void __init__();
	virtual void __delete__();

	Synthesizer *getSynthesizer(){ return s; }
	void setSynthesizer(Synthesizer *s);

	virtual int read(BufferBox &buf);
	virtual void reset();

	void add(const MidiEvent &e);
	void feed(const MidiData &data);
	void endAllNotes();
	void resetMidiData();

	void setAutoStop(bool auto_stop);

private:
	Synthesizer *s;
};

#endif /* SRC_AUDIO_SYNTH_SYNTHESIZERRENDERER_H_ */
