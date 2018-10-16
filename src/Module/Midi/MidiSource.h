/*
 * MidiSource.h
 *
 *  Created on: 02.04.2018
 *      Author: michi
 */

#ifndef SRC_MODULE_MIDI_MIDISOURCE_H_
#define SRC_MODULE_MIDI_MIDISOURCE_H_

#include "../Module.h"
#include "../Port/MidiPort.h"

class BeatPort;

class MidiSource : public Module
{
public:
	MidiSource();

	void _cdecl __init__();
	void _cdecl __delete__() override;

	class Output : public MidiPort
	{
	public:
		Output(MidiSource *s);
		~Output() override {}
		int _cdecl read(MidiEventBuffer &midi) override;
		void _cdecl reset() override;
		MidiSource *source;
	};
	Output *out;

	virtual int _cdecl read(MidiEventBuffer &midi){ return 0; };
	virtual void _cdecl reset(){}

	BeatPort *beat_source;
};

MidiSource *_cdecl CreateMidiSource(Session *session, const string &name);

#endif /* SRC_MODULE_MIDI_MIDISOURCE_H_ */
