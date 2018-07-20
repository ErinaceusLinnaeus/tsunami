/*
 * ActionTrackSampleFromSelection.cpp
 *
 *  Created on: 11.07.2012
 *      Author: michi
 */

#include "ActionTrackSampleFromSelection.h"
#include "ActionTrackPasteAsSample.h"
#include "../Buffer/ActionTrackEditBuffer.h"
#include "../Midi/ActionTrackDeleteMidiNote.h"
#include "../../../Data/SongSelection.h"
#include "../../../Data/Song.h"
#include "../../../Data/Track.h"
#include "../Buffer/ActionTrack__DeleteBuffer.h"

ActionTrackSampleFromSelection::ActionTrackSampleFromSelection(const SongSelection &_sel) :
	sel(_sel)
{
}

void ActionTrackSampleFromSelection::build(Data *d)
{
	Song *s = dynamic_cast<Song*>(d);
	for (Track *t: s->tracks)
		for (TrackLayer *l: t->layers)
		if (sel.has(l)){
			if (t->type == t->Type::AUDIO)
				CreateSamplesFromLayerAudio(l, sel);
			else if (t->type == t->Type::MIDI)
				CreateSamplesFromLayerMidi(l, sel);
		}
}

void ActionTrackSampleFromSelection::CreateSamplesFromLayerAudio(TrackLayer *l, const SongSelection &sel)
{
	foreachib(AudioBuffer &b, l->buffers, bi)
		if (sel.range.covers(b.range())){
			addSubAction(new ActionTrackPasteAsSample(l, b.offset, b, false), l->track->song);
			addSubAction(new ActionTrack__DeleteBuffer(l, bi), l->track->song);
		}
}

void ActionTrackSampleFromSelection::CreateSamplesFromLayerMidi(TrackLayer *l, const SongSelection &sel)
{
	MidiNoteBuffer midi = l->midi.getNotesBySelection(sel);
	midi.samples = sel.range.length;
	if (midi.num == 0)
		return;
	for (auto n: midi)
		n->range.offset -= sel.range.offset;
	addSubAction(new ActionTrackPasteAsSample(l, sel.range.offset, midi, false), l->track->song);
	foreachib(MidiNote *n, l->midi, i)
		if (sel.has(n))
			addSubAction(new ActionTrackDeleteMidiNote(l, i), l->track->song);
}
