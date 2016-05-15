/*
 * Clipboard.cpp
 *
 *  Created on: 21.12.2012
 *      Author: michi
 */

#include "Clipboard.h"
#include "../View/AudioView.h"
#include "../Action/Track/Sample/ActionTrackPasteAsSample.h"
#include "../Tsunami.h"
#include "../Stuff/Log.h"
#include <assert.h>
#include "../Data/Song.h"
#include "../Data/SongSelection.h"

Clipboard::Clipboard() :
	Observable("Clipboard")
{
	temp = new Song;
	temp->reset();
}

Clipboard::~Clipboard()
{
	delete(temp);
}

void Clipboard::clear()
{
	if (temp->tracks.num > 0){
		temp->reset();
		notify();
	}
	ref_uid.clear();
}

void Clipboard::append_track(Track *t, AudioView *view)
{
	if (t->type == Track::TYPE_TIME)
		return;

	Track *tt = temp->addTrack(t->type);

	if (t->type == Track::TYPE_AUDIO){
		tt->levels[0].buffers.add(t->readBuffers(view->cur_level, view->sel.range));
		tt->levels[0].buffers[0].make_own();
	}else if (t->type == Track::TYPE_MIDI){
		tt->midi = t->midi.getNotesSafe(view->sel.range);
		tt->midi.samples = view->sel.range.length;
		tt->midi.sanify(view->sel.range);
		foreach(MidiNote &n, tt->midi)
			n.range.offset -= view->sel.range.offset;
	}

	ref_uid.add(-1);
}

void Clipboard::copy(AudioView *view)
{
	if (!canCopy(view))
		return;
	clear();

	Song *s = view->song;

	temp->sample_rate = s->sample_rate;

	SongSelection sel = view->getEditSeletion();
	foreach(Track *t, s->tracks)
		if (sel.has(t))
			append_track(t, view);

	notify();
}

void Clipboard::paste_track(int source_index, Track *target, AudioView *view)
{
	Song *s = target->song;
	Track *source = temp->tracks[source_index];

	int ref_index = s->get_sample_by_uid(ref_uid[source_index]);
	if (ref_index >= 0){
		target->addSample(view->sel.range.start(), ref_index);
	}else{
		if (target->type == Track::TYPE_AUDIO){
			s->execute(new ActionTrackPasteAsSample(target, view->sel.range.start(), &source->levels[0].buffers[0]));
			ref_uid[source_index] = s->samples.back()->uid;
		}else if (target->type == Track::TYPE_MIDI){
			s->execute(new ActionTrackPasteAsSample(target, view->sel.range.start(), &source->midi));
			ref_uid[source_index] = s->samples.back()->uid;
		}
	}
}

void Clipboard::paste(AudioView *view)
{
	if (!hasData())
		return;
	Song *a = view->song;

	Array<string> temp_type, dest_type;
	foreach(Track *t, a->tracks){
		if (!view->sel.has(t))
			continue;
		if (t->type == Track::TYPE_TIME)
			continue;
		dest_type.add(track_type(t->type));
	}

	foreach(Track *t, temp->tracks)
		temp_type.add(track_type(t->type));

	// only 1 track in clipboard => paste into current
	bool paste_single = (temp->tracks.num == 1);
	if (paste_single){
		dest_type.clear();
		dest_type.add(track_type(view->cur_track->type));
	}

	if (dest_type.num != temp->tracks.num){
		tsunami->log->error(format(_("%d tracks selected for pasting (ignoring the metronome), but %d tracks in clipboard"), dest_type.num, temp->tracks.num));
		return;
	}
	string t1 = "[" + implode(temp_type, ", ") + "]";
	string t2 = "[" + implode(dest_type, ", ") + "]";
	if (t1 != t2){
		tsunami->log->error(format(_("Track types in clipboard (%s) don't match those you want to paste into (%s)"), t1.c_str(), t2.c_str()));
		return;
	}


	a->action_manager->beginActionGroup();

	if (paste_single){
		paste_track(0, view->cur_track, view);
	}else{
		int ti = 0;
		foreach(Track *t, a->tracks){
			if (!view->sel.has(t))
				continue;
			if (t->type == Track::TYPE_TIME)
				continue;

			paste_track(ti, t, view);
			ti ++;
		}
	}
	a->action_manager->endActionGroup();
}

bool Clipboard::hasData()
{
	return (temp->tracks.num > 0);
}

bool Clipboard::canCopy(AudioView *view)
{
	return !view->sel.range.empty();// or (view->audio->GetNumSelectedSamples() > 0);
}

