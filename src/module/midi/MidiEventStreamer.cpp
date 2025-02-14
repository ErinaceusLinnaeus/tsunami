/*
 * MidiEventStreamer.cpp
 *
 *  Created on: 07.10.2017
 *      Author: michi
 */

#include "MidiEventStreamer.h"

#include "../beats/BeatSource.h"

namespace tsunami {

MidiEventStreamer::MidiEventStreamer() {
	module_class = "MidiEventStreamer";
	module_category = ModuleCategory::MidiSource;
	offset = 0;
	ignore_end = false;
	loop = false;
}

int MidiEventStreamer::read(MidiEventBuffer& _midi) {
	int n = min(midi.samples - offset, _midi.samples);
	if (ignore_end)
		n = _midi.samples;
	if (n <= 0) {
		if (loop)
			offset = 0;
		else if (!ignore_end)
			return Return::EndOfStream;
	}

	Range r = Range(offset, n);
	//midi.read(_midi, r);
	for (MidiEvent &e : midi)
		if (r.is_inside(e.pos))
			_midi.add(e.shifted(- offset));
	offset += n;
	return n;
}

void MidiEventStreamer::reset_state() {
	offset = 0;
}

void MidiEventStreamer::set_data(const MidiEventBuffer &_midi) {
	midi = _midi;
}

void MidiEventStreamer::set_pos(int pos) {
	offset = pos;
}

int MidiEventStreamer::get_pos() const {
	return offset;
}

}


