/*
 * ViewModeMidi.cpp
 *
 *  Created on: 12.11.2015
 *      Author: michi
 */

#include "ViewModeMidi.h"
#include "../AudioView.h"
#include "../AudioViewTrack.h"
#include "../../Device/OutputStream.h"
#include "../../Audio/Renderer/MidiRenderer.h"
#include "../../Audio/Synth/Synthesizer.h"
#include "../../Audio/Renderer/SongRenderer.h"
#include "../../Midi/Clef.h"
#include "../../Midi/MidiSource.h"
#include "../../Data/SongSelection.h"
#include "../../TsunamiWindow.h"

void align_to_beats(Song *s, Range &r, int beat_partition);

const int PITCH_SHOW_COUNT = 30;

class MidiPreviewSource : public MidiSource
{
public:
	MidiPreviewSource()
	{
		started = ended = false;
		end_of_stream = false;
	}
	virtual int _cdecl read(MidiRawData &midi)
	{
		if (end_of_stream)
			return 0;
		if (started){
			for (int p: pitch)
				midi.add(MidiEvent(0, p, 1));
			started = false;
		}
		if (ended){
			for (int p: pitch)
				midi.add(MidiEvent(0, p, 0));
			ended = false;
			end_of_stream = true;
		}
		return midi.samples;
	}

	void start(const Array<int> &_pitch)
	{
		if (ended)
			return;
		pitch = _pitch;
		started = true;
		end_of_stream = false;
	}
	void end()
	{
		ended = true;
	}

	bool started, ended;
	bool end_of_stream;

	Array<int> pitch;
};

static MidiPreviewSource *preview_source;

ViewModeMidi::ViewModeMidi(AudioView *view) :
	ViewModeDefault(view)
{
	cur_track = NULL;
	beat_partition = 4;
	win->setInt("beat_partition", beat_partition);
	mode_wanted = AudioView::MIDI_MODE_CLASSICAL;
	creation_mode = CREATION_MODE_NOTE;
	midi_interval = 3;
	chord_type = 0;
	chord_inversion = 0;
	modifier = MODIFIER_NONE;

	deleting = false;

	scroll_offset = 0;
	scroll_bar = rect(0, 0, 0, 0);

	preview_source = new MidiPreviewSource;

	preview_synth = NULL;
	preview_renderer = new MidiRenderer(preview_synth, preview_source);
	preview_stream = new OutputStream(preview_renderer);
	preview_stream->setBufferSize(2048);
}

ViewModeMidi::~ViewModeMidi()
{
	delete preview_stream;
	delete preview_renderer;
	if (preview_synth)
		delete preview_synth;
}

void ViewModeMidi::setMode(int _mode)
{
	mode_wanted = _mode;
	view->forceRedraw();
}

void ViewModeMidi::setCreationMode(int _mode)
{
	creation_mode = _mode;
	//view->forceRedraw();
}

void ViewModeMidi::onLeftButtonDown()
{
	ViewModeDefault::onLeftButtonDown();

	if (selection->type == Selection::TYPE_MIDI_NOTE){
		selection->track->deleteMidiNote(selection->index);
		hover->clear();
		deleting = true;
	}else if (selection->type == Selection::TYPE_MIDI_PITCH){
		if (preview_synth)
			delete preview_synth;
		preview_synth = (Synthesizer*)view->cur_track->synth->copy();
		preview_renderer->setSynthesizer(preview_synth);

		preview_source->start(getCreationPitch(selection->pitch));
		if (!preview_stream->isPlaying())
			preview_stream->play();
	}else if (selection->type == Selection::TYPE_SCROLL){
		scroll_offset = view->my - scroll_bar.y1;
	}
}

void ViewModeMidi::onLeftButtonUp()
{
	ViewModeDefault::onLeftButtonUp();

	if (selection->type == Selection::TYPE_MIDI_PITCH){
		view->song->action_manager->beginActionGroup();
		MidiData notes = getCreationNotes(selection, mouse_possibly_selecting_start);
		for (MidiNote *n: notes)
			view->cur_track->addMidiNote(*n);
		view->song->action_manager->endActionGroup();

		preview_source->end();
	}
	deleting = false;
}

void ViewModeMidi::onMouseMove()
{
	ViewModeDefault::onMouseMove();

	if (deleting){
		*hover = getHover();
		if ((hover->type == Selection::TYPE_MIDI_NOTE) and (hover->track == view->cur_track)){
			selection->track->deleteMidiNote(hover->index);
			hover->clear();
		}
	}

	// drag & drop
	if (selection->type == Selection::TYPE_MIDI_PITCH){
		view->forceRedraw();
	}else if (selection->type == Selection::TYPE_SCROLL){
		int _pitch_max = (cur_track->area.y2 + scroll_offset - view->my) / cur_track->area.height() * (MAX_PITCH - 1.0f);
		cur_track->setPitchMinMax(_pitch_max - PITCH_SHOW_COUNT, _pitch_max);
	}
}

void ViewModeMidi::onKeyDown(int k)
{
	if (k == KEY_1){
		modifier = MODIFIER_NONE;
		view->notify(view->MESSAGE_SETTINGS_CHANGE);
	}else if (k == KEY_2){
		modifier = MODIFIER_SHARP;
		view->notify(view->MESSAGE_SETTINGS_CHANGE);
	}else if (k == KEY_3){
		modifier = MODIFIER_FLAT;
		view->notify(view->MESSAGE_SETTINGS_CHANGE);
	}else if (k == KEY_4){
		modifier = MODIFIER_NATURAL;
		view->notify(view->MESSAGE_SETTINGS_CHANGE);
	}else{
		ViewModeDefault::onKeyDown(k);
	}
}

void ViewModeMidi::updateTrackHeights()
{
	for (AudioViewTrack *t: view->vtrack){
		t->height_min = view->TIME_SCALE_HEIGHT;
		if (t->track->type == Track::TYPE_AUDIO){
			t->height_wish = view->MAX_TRACK_CHANNEL_HEIGHT;
		}else if (t->track->type == Track::TYPE_MIDI){
			if (t->track == view->cur_track){
				if (view->midi_view_mode == view->MIDI_MODE_CLASSICAL)
					t->height_wish = view->MAX_TRACK_CHANNEL_HEIGHT * 4;
				else
					t->height_wish = 5000;
			}else{
				t->height_wish = view->MAX_TRACK_CHANNEL_HEIGHT;
			}
		}else{
			t->height_wish = view->TIME_SCALE_HEIGHT * 2;
		}
	}
}

void ViewModeMidi::onCurTrackChange()
{
	view->thm.dirty = true;
}


Range get_allowed_midi_range(Track *t, Array<int> pitch, int start)
{
	Range allowed = Range::ALL;
	for (MidiNote *n: t->midi){
		for (int p: pitch)
			if (n->pitch == p){
				if (n->range.is_inside(start))
					return Range::EMPTY;
			}
	}

	MidiRawData midi = midi_notes_to_events(t->midi);
	for (MidiEvent &e: midi)
		for (int p: pitch)
			if (e.pitch == p){
				if ((e.pos >= start) and (e.pos < allowed.end()))
					allowed.set_end(e.pos);
				if ((e.pos < start) and (e.pos >= allowed.start()))
					allowed.set_start(e.pos);
			}
	return allowed;
}

Array<int> ViewModeMidi::getCreationPitch(int base_pitch)
{
	Array<int> pitch;
	if (creation_mode == CREATION_MODE_NOTE){
		pitch.add(base_pitch);
	}else if (creation_mode == CREATION_MODE_INTERVAL){
		pitch.add(base_pitch);
		if (midi_interval != 0)
			pitch.add(base_pitch + midi_interval);
	}else if (creation_mode == CREATION_MODE_CHORD){
		pitch = chord_notes(chord_type, chord_inversion, base_pitch);
	}
	return pitch;
}

MidiData ViewModeMidi::getCreationNotes(Selection *sel, int pos0)
{
	int start = min(pos0, sel->pos);
	int end = max(pos0, sel->pos);
	Range r = Range(start, end - start);

	// align to beats
	if (song->bars.num > 0)
		align_to_beats(song, r, beat_partition);

	Array<int> pitch = getCreationPitch(sel->pitch);

	// collision?
	Range allowed = get_allowed_midi_range(view->cur_track, pitch, pos0);

	// create notes
	MidiData notes;
	if (allowed.empty())
		return notes;
	for (int p: pitch)
		notes.add(new MidiNote(r and allowed, p, 1));
	notes[0]->clef_position = sel->clef_position;
	notes[0]->modifier = sel->modifier;
	return notes;
}

void ViewModeMidi::setBeatPartition(int partition)
{
	beat_partition = partition;
	view->forceRedraw();
}

void ViewModeMidi::drawTrackBackground(Painter *c, AudioViewTrack *t)
{
	t->drawBlankBackground(c);

	color cc = t->getBackgroundColor();
	view->drawGridTime(c, t->area, cc, false);
	t->drawGridBars(c, cc, (t->track->type == Track::TYPE_TIME), beat_partition);

	if (t->track->type == Track::TYPE_MIDI){
		int mode = which_midi_mode(t->track);
		if (t->track == view->cur_track){
			if (mode == AudioView::MIDI_MODE_LINEAR)
				drawTrackPitchGrid(c, t);
		}

		if (mode == AudioView::MIDI_MODE_CLASSICAL){
			const Clef& clef = t->track->instrument.get_clef();
			t->drawMidiClefClassical(c, clef, view->midi_scale);
		}else if (mode == AudioView::MIDI_MODE_TAB){
			t->drawMidiClefTab(c);
		}
	}


}

void ViewModeMidi::drawTrackPitchGrid(Painter *c, AudioViewTrack *t)
{
	cur_track = t;

	// pitch grid
	c->setColor(color(0.25f, 0, 0, 0));
	for (int i=t->pitch_min; i<t->pitch_max; i++){
		float y0 = t->pitch2y_linear(i + 1);
		float y1 = t->pitch2y_linear(i);
		if (!view->midi_scale.contains(i)){
			c->setColor(color(0.2f, 0, 0, 0));
			c->drawRect(t->area.x1, y0, t->area.width(), y1 - y0);
		}
	}


	// pitch names
	color cc = view->colors.text;
	cc.a = 0.4f;
	Array<SampleRef*> *p = NULL;
	if ((t->track->synth) and (t->track->synth->name == "Sample")){
		PluginData *c = t->track->synth->get_config();
		p = (Array<SampleRef*> *)&c[1];
	}
	bool is_drum = (t->track->instrument.type == Instrument::TYPE_DRUMS);
	for (int i=cur_track->pitch_min; i<cur_track->pitch_max; i++){
		c->setColor(cc);
		if (((hover->type == Selection::TYPE_MIDI_PITCH) or (hover->type == Selection::TYPE_MIDI_NOTE)) and (i == hover->pitch))
			c->setColor(view->colors.text);

		string name = pitch_name(i);
		if (is_drum){
			name = drum_pitch_name(i);
		}else if (p){
			if (i < p->num)
				if ((*p)[i])
					name = (*p)[i]->origin->name;
		}
		c->drawStr(20, t->area.y1 + t->area.height() * (cur_track->pitch_max - i - 1) / PITCH_SHOW_COUNT, name);
	}
}

inline bool hover_note_classical_or_tab(const MidiNote &n, Selection &s, ViewModeMidi *vmm)
{
	if (n.clef_position != s.clef_position)
		return false;
	return n.range.is_inside(s.pos);
}

inline bool hover_note_linear(const MidiNote &n, Selection &s, ViewModeMidi *vmm)
{
	if (n.pitch != s.pitch)
		return false;
	return n.range.is_inside(s.pos);
}

Selection ViewModeMidi::getHover()
{
	Selection s;
	int mx = view->mx;
	int my = view->my;

	// track?
	foreachi(AudioViewTrack *t, view->vtrack, i){
		if (view->mouseOverTrack(t)){
			s.vtrack = t;
			s.index = i;
			s.track = t->track;
			s.type = Selection::TYPE_TRACK;
			if (view->mx < t->area.x1 + view->TRACK_HANDLE_WIDTH)
				s.show_track_controls = t->track;
		}
	}

	// selection boundaries?
	view->selectionUpdatePos(s);
	if (view->mouse_over_time(view->sel_raw.end())){
		s.type = Selection::TYPE_SELECTION_END;
		return s;
	}
	if (view->mouse_over_time(view->sel_raw.start())){
		s.type = Selection::TYPE_SELECTION_START;
		return s;
	}
	if (view->stream->isPlaying()){
		if (view->mouse_over_time(view->stream->getPos())){
			s.type = Selection::TYPE_PLAYBACK;
			return s;
		}
	}

	// mute button?
	if (s.track){
		AudioViewTrack *t = s.vtrack;
		if ((mx >= t->area.x1 + 5) and (mx < t->area.x1 + 17) and (my >= t->area.y1 + 22) and (my < t->area.y1 + 34)){
			s.type = Selection::TYPE_MUTE;
			return s;
		}
		if ((song->tracks.num > 1) and (mx >= t->area.x1 + 22) and (mx < t->area.x1 + 34) and (my >= t->area.y1 + 22) and (my < t->area.y1 + 34)){
			s.type = Selection::TYPE_SOLO;
			return s;
		}
	}

	// sub?
	if (s.track){

		// markers
		for (int i=0; i<min(s.track->markers.num, view->vtrack[s.index]->marker_areas.num); i++){
			if (view->vtrack[s.index]->marker_areas[i].inside(mx, my)){
				s.type = Selection::TYPE_MARKER;
				s.index = i;
				return s;
			}
		}

		// TODO: prefer selected subs
		for (SampleRef *ss: s.track->samples){
			int offset = view->mouseOverSample(ss);
			if (offset >= 0){
				s.sample = ss;
				s.type = Selection::TYPE_SAMPLE;
				s.sample_offset = offset;
				return s;
			}
		}
	}

	// midi
	if ((s.track) and (s.track->type == Track::TYPE_MIDI) and (s.track == view->cur_track)){

		// scroll bar
		if (scroll_bar.inside(view->mx, view->my)){
			s.type = Selection::TYPE_SCROLL;
			return s;
		}

		if (creation_mode != CREATION_MODE_SELECT){
			int mode = which_midi_mode(s.track);
			if ((mode == AudioView::MIDI_MODE_CLASSICAL)){// or (mode == AudioView::MIDI_MODE_TAB)){
				s.pitch = cur_track->y2pitch_classical(my, modifier);
				s.clef_position = cur_track->screen_to_clef_pos(my);
				s.modifier = modifier;
				s.type = Selection::TYPE_MIDI_PITCH;
				s.index = randi(100000); // quick'n'dirty fix to force view update every time the mouse moves

				foreachi(MidiNote *n, s.track->midi, i)
					if (hover_note_classical_or_tab(*n, s, this)){
						s.note = n;
						s.index = i;
						s.type = Selection::TYPE_MIDI_NOTE;
						return s;
					}
			}else if (mode == AudioView::MIDI_MODE_LINEAR){
				s.pitch = cur_track->y2pitch_linear(my);
				s.clef_position = cur_track->y2clef_linear(my, s.modifier);
				//s.modifier = modifier;
				s.type = Selection::TYPE_MIDI_PITCH;
				s.index = randi(100000); // quick'n'dirty fix to force view update every time the mouse moves

				foreachi(MidiNote *n, s.track->midi, i)
					if (hover_note_linear(*n, s, this)){
						s.note = n;
						s.index = i;
						s.type = Selection::TYPE_MIDI_NOTE;
						return s;
					}
			}
		}
	}

	// time scale
	if (my < view->TIME_SCALE_HEIGHT){
		s.type = Selection::TYPE_TIME;
		return s;
	}

	// track handle
	if ((s.track) and (mx < view->area.x1 + view->TRACK_HANDLE_WIDTH)){
		s.type = Selection::TYPE_TRACK_HANDLE;
		return s;
	}

	return s;
}

void ViewModeMidi::drawTrackData(Painter *c, AudioViewTrack *t)
{
	// midi
	if ((view->cur_track == t->track) and (t->track->type == Track::TYPE_MIDI)){
		// we're editing this track...
		cur_track = t;

		for (int n: t->reference_tracks)
			if ((n >= 0) and (n < song->tracks.num) and (n != t->track->get_index()))
				drawMidi(c, t, song->tracks[n]->midi, true, 0);

		drawMidi(c, t, t->track->midi, false, 0);

		int mode = which_midi_mode(t->track);

		// current creation
		if ((HuiGetEvent()->lbut) and (selection->type == Selection::TYPE_MIDI_PITCH)){
			MidiData notes = getCreationNotes(selection, mouse_possibly_selecting_start);
			drawMidi(c, t, notes, false, 0);
			//c->setFontSize(view->FONT_SIZE);
		}


		// creation preview
		if ((!HuiGetEvent()->lbut) and (hover->type == Selection::TYPE_MIDI_PITCH)){
			MidiData notes = getCreationNotes(hover, hover->pos);
			drawMidi(c, t, notes, false, 0);
		}


		if (mode == view->MIDI_MODE_CLASSICAL){

		}else if (mode == view->MIDI_MODE_LINEAR){

			// scrollbar
			if (hover->type == Selection::TYPE_SCROLL)
				c->setColor(view->colors.text);
			else
				c->setColor(view->colors.text_soft1);
			scroll_bar = rect(t->area.x2 - 40, t->area.x2 - 20, t->area.y2 - t->area.height() * cur_track->pitch_max / (MAX_PITCH - 1.0f), t->area.y2 - t->area.height() * cur_track->pitch_min / (MAX_PITCH - 1.0f));
			c->drawRect(scroll_bar);
		}
	}else{

		// not editing -> just draw
		if (t->track->type == Track::TYPE_MIDI)
			drawMidi(c, t, t->track->midi, false, 0);
	}

	// audio buffer
	t->drawTrackBuffers(c, view->cam.pos);

	// samples
	for (SampleRef *s: t->track->samples)
		t->drawSample(c, s);

	// marker
	t->marker_areas.resize(t->track->markers.num);
	foreachi(TrackMarker *m, t->track->markers, i)
		t->drawMarker(c, m, i, (view->hover.type == Selection::TYPE_MARKER) and (view->hover.track == t->track) and (view->hover.index == i));
}

int ViewModeMidi::which_midi_mode(Track *t)
{
	if ((view->cur_track == t) and (t->type == Track::TYPE_MIDI)){
		if (mode_wanted == view->MIDI_MODE_TAB){
			if (t->instrument.string_pitch.num > 0)
				return view->MIDI_MODE_TAB;
			return view->MIDI_MODE_CLASSICAL;
		}
		return mode_wanted;
	}
	return ViewModeDefault::which_midi_mode(t);
}
