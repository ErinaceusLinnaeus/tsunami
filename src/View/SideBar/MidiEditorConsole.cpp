/*
 * MidiEditorConsole.cpp
 *
 *  Created on: 09.09.2014
 *      Author: michi
 */

#include "../../Data/base.h"
#include "../../Data/Song.h"
#include "../../Data/Track.h"
#include "../../Data/TrackLayer.h"
#include "../../Data/Rhythm/BarCollection.h"
#include "../../Data/Rhythm/Beat.h"
#include "../../Data/Midi/MidiData.h"
#include "../../Module/Synth/Synthesizer.h"
#include "../../Module/ConfigPanel.h"
#include "../AudioView.h"
#include "../Node/AudioViewLayer.h"
#include "../Mode/ViewModeMidi.h"
#include "../../Session.h"
#include "../../Device/DeviceManager.h"
#include "../../Device/Device.h"
#include "MidiEditorConsole.h"
#include "../Dialog/MarkerDialog.h"
#include "../Dialog/QuestionDialog.h"

//int get_track_index_save(Song *song, Track *t);



enum class NoteBaseLength {
	WHOLE,
	HALF,
	QUARTER,
	EIGTH,
	SIXTEENTH
};

enum class NoteLengthModifier {
	NONE,
	DOTTED,
	TRIPLET
};


MidiEditorConsole::MidiEditorConsole(Session *session) :
	SideBarConsole(_("Midi"), session)
{
	from_resource("midi_editor");

	id_inner = "midi_fx_inner_table";

	set_int("interval", view->mode_midi->midi_interval);

	for (int i=0; i<(int)ChordType::NUM; i++)
		add_string("chord_type", chord_type_name((ChordType)i));
	set_int("chord_type", 0);
	add_string("chord_inversion", _("Basic form"));
	add_string("chord_inversion", _("1st inversion"));
	add_string("chord_inversion", _("2nd inversion"));
	set_int("chord_inversion", 0);


	layer = nullptr;
	//Enable("add", false);
	enable("track_name", false);

	//event("beat_partition", [=]{ on_beat_partition(); });
	//event("note_length", [=]{ on_note_length(); });
	//event("midi_edit_mode", [=]{ on_creation_mode(); });
	event("length-whole", [=]{ on_base_length(NoteBaseLength::WHOLE); });
	event("length-half", [=]{ on_base_length(NoteBaseLength::HALF); });
	event("length-quarter", [=]{ on_base_length(NoteBaseLength::QUARTER); });
	event("length-eighth", [=]{ on_base_length(NoteBaseLength::EIGTH); });
	event("length-sixteenth", [=]{ on_base_length(NoteBaseLength::SIXTEENTH); });
	event("length-dotted", [=]{ on_length_dotted(); });
	event("length-triplet", [=]{ on_length_triplet(); });
	event("length-custom", [=]{ on_length_custom(); });

	event("mode-select", [=]{ view->mode_midi->set_creation_mode(ViewModeMidi::CreationMode::SELECT); });
	event("mode-note", [=]{ view->mode_midi->set_creation_mode(ViewModeMidi::CreationMode::NOTE); });
	event("mode-interval", [=]{ view->mode_midi->set_creation_mode(ViewModeMidi::CreationMode::INTERVAL); });
	event("mode-chord", [=]{ view->mode_midi->set_creation_mode(ViewModeMidi::CreationMode::CHORD); });


	event("interval", [=]{ on_interval(); });
	event("chord_type", [=]{ on_chord_type(); });
	event("chord_inversion", [=]{ on_chord_inversion(); });
	event_x("reference_tracks", "hui:select", [=]{ on_reference_tracks(); });
	event("modifier-none", [=]{ on_modifier(NoteModifier::NONE); });
	event("modifier-sharp", [=]{ on_modifier(NoteModifier::SHARP); });
	event("modifier-flat", [=]{ on_modifier(NoteModifier::FLAT); });
	event("modifier-natural", [=]{ on_modifier(NoteModifier::NATURAL); });
	event("input_active", [=]{ on_input_active(); });
	event("input_capture", [=]{ on_input_capture(); });
	event("input", [=]{ on_input_source(); });
	event("input_volume:key", [=]{ on_input_volume(0); });
	event("input_volume:max", [=]{ on_input_volume(1); });
	event("quantize", [=]{ on_quantize(); });
	event("apply_string", [=]{ on_apply_string(); });
	event("apply_hand_position", [=]{ on_apply_hand_position(); });
	event("apply_pitch_shift", [=]{ on_apply_pitch_shift(); });
	event("flag-none", [=]{ on_apply_flags(0); });
	event("flag-trill", [=]{ on_apply_flags(NOTE_FLAG_TRILL); });
	event("flag-staccato", [=]{ on_apply_flags(NOTE_FLAG_STACCATO); });
	event("flag-tenuto", [=]{ on_apply_flags(NOTE_FLAG_TENUTO); });
	event("add_key_change", [=]{ on_add_key_change(); });
	event("edit_track", [=]{ on_edit_track(); });
	event("edit_midi_fx", [=]{ on_edit_midi_fx(); });
	event("edit_song", [=]{ on_edit_song(); });
}

MidiEditorConsole::~MidiEditorConsole() {
	clear();
}

//NoteBaseLength analyse_note_length(MidiEditorConsole *c, NoteLengthModifier &mod) {
//}

bool is_dotted(MidiEditorConsole *c) {
	if (c->view->mode_midi->sub_beat_partition*6 == c->view->mode_midi->note_length) // whole
		return true;
	if (c->view->mode_midi->sub_beat_partition*3 == c->view->mode_midi->note_length) // half
		return true;
	if (c->view->mode_midi->sub_beat_partition*3 == c->view->mode_midi->note_length*2) // quarter
		return true;
	if (c->view->mode_midi->sub_beat_partition*3 == c->view->mode_midi->note_length*4) // eighth
		return true;
	if (c->view->mode_midi->sub_beat_partition*3 == c->view->mode_midi->note_length*8) // sixteenth
		return true;
	return false;
}

bool is_triplet(MidiEditorConsole *c) {
	if (c->view->mode_midi->sub_beat_partition*4 == c->view->mode_midi->note_length*3) // half
		return true;
	if (c->view->mode_midi->sub_beat_partition*2 == c->view->mode_midi->note_length*3) // quarter
		return true;
	if (c->view->mode_midi->sub_beat_partition == c->view->mode_midi->note_length*3) // eighth
		return true;
	if (c->view->mode_midi->sub_beat_partition == c->view->mode_midi->note_length*6) // sixteenth
		return true;
	return false;
}

bool base_is_whole(MidiEditorConsole *c) {
	if (is_dotted(c))
		return c->view->mode_midi->sub_beat_partition*6 == c->view->mode_midi->note_length;
	if (is_triplet(c))
		return c->view->mode_midi->sub_beat_partition*8 == c->view->mode_midi->note_length*3;
	return c->view->mode_midi->sub_beat_partition*4 == c->view->mode_midi->note_length;
}

bool base_is_half(MidiEditorConsole *c) {
	if (is_dotted(c))
		return c->view->mode_midi->sub_beat_partition*3 == c->view->mode_midi->note_length;
	if (is_triplet(c))
		return c->view->mode_midi->sub_beat_partition*4 == c->view->mode_midi->note_length*3;
	return c->view->mode_midi->sub_beat_partition*2 == c->view->mode_midi->note_length;
}

bool base_is_quarter(MidiEditorConsole *c) {
	if (is_dotted(c))
		return c->view->mode_midi->sub_beat_partition*3 == c->view->mode_midi->note_length*2;
	if (is_triplet(c))
		return c->view->mode_midi->sub_beat_partition*2 == c->view->mode_midi->note_length*3;
	return c->view->mode_midi->sub_beat_partition == c->view->mode_midi->note_length;
}

bool base_is_eighth(MidiEditorConsole *c) {
	if (is_dotted(c))
		return c->view->mode_midi->sub_beat_partition*3 == c->view->mode_midi->note_length*4;
	if (is_triplet(c))
		return c->view->mode_midi->sub_beat_partition == c->view->mode_midi->note_length*3;
	return c->view->mode_midi->sub_beat_partition == c->view->mode_midi->note_length*2;
}

bool base_is_sixteenth(MidiEditorConsole *c) {
	if (is_dotted(c))
		return c->view->mode_midi->sub_beat_partition*3 == c->view->mode_midi->note_length*8;
	if (is_triplet(c))
		return c->view->mode_midi->sub_beat_partition == c->view->mode_midi->note_length*6;
	return c->view->mode_midi->sub_beat_partition == c->view->mode_midi->note_length*4;
}

void MidiEditorConsole::update() {
	bool allow = false;
	if (layer)
		//if (get_track_index_save(view->song, view->cur_track) >= 0)
			allow = (layer->type == SignalType::MIDI);
	hide_control("me_grid_yes", !allow);
	hide_control("me_grid_no", allow);
	hide_control(id_inner, !allow);

	if (!layer)
		return;

	check("mode-select", view->mode_midi->creation_mode == view->mode_midi->CreationMode::SELECT);
	check("mode-note", view->mode_midi->creation_mode == view->mode_midi->CreationMode::NOTE);
	check("mode-interval", view->mode_midi->creation_mode == view->mode_midi->CreationMode::INTERVAL);
	check("mode-chord", view->mode_midi->creation_mode == view->mode_midi->CreationMode::CHORD);

	hide_control("grid-interval", view->mode_midi->creation_mode != view->mode_midi->CreationMode::INTERVAL);
	hide_control("grid-chord", view->mode_midi->creation_mode != view->mode_midi->CreationMode::CHORD);

	check("modifier-none", view->mode_midi->modifier == NoteModifier::NONE);
	check("modifier-sharp", view->mode_midi->modifier == NoteModifier::SHARP);
	check("modifier-flat", view->mode_midi->modifier == NoteModifier::FLAT);
	check("modifier-natural", view->mode_midi->modifier == NoteModifier::NATURAL);

	MidiMode mode = view->get_layer(layer)->midi_mode();

	enable("modifier-none", mode == MidiMode::CLASSICAL);
	enable("modifier-sharp", mode == MidiMode::CLASSICAL);
	enable("modifier-flat", mode == MidiMode::CLASSICAL);
	enable("modifier-natural", mode == MidiMode::CLASSICAL);

	set_int("midi_edit_mode", (int)view->mode_midi->creation_mode);

	set_int("beat_partition", view->mode_midi->sub_beat_partition);
	set_int("note_length", view->mode_midi->note_length);
	string length = format(u8"(%d 𝅘𝅥 / %d)", view->mode_midi->note_length, view->mode_midi->sub_beat_partition);
	set_string("length-result", length);

	check("length-whole", base_is_whole(this));
	check("length-half", base_is_half(this));
	check("length-quarter", base_is_quarter(this));
	check("length-eighth", base_is_eighth(this));
	check("length-sixteenth", base_is_sixteenth(this));
	check("length-dotted", is_dotted(this));
	check("length-triplet", is_triplet(this));

	check("input_active", view->mode_midi->is_input_active());
	enable("input_capture", view->mode_midi->is_input_active());
	check("input_capture", view->mode_midi->input_capture);
	enable("input", view->mode_midi->is_input_active());
	update_input_device_list();

	if (view->mode_midi->maximize_input_volume)
		check("input_volume:max", true);
	else
		check("input_volume:key", true);
	enable("input_volume:key", view->mode_midi->is_input_active());
	enable("input_volume:max", view->mode_midi->is_input_active());



	if (layer->track->instrument.type == Instrument::Type::DRUMS) {
		// select a nicer pitch range in linear mode for drums
//		view->get_layer(layer->track)->setPitchMinMax(34, 34 + 30);//PITCH_SHOW_COUNT);
		// TODO
	}
}

void MidiEditorConsole::update_input_device_list() {
	input_sources = session->device_manager->good_device_list(DeviceType::MIDI_INPUT);

	reset("input");
	for (auto *d: input_sources)
		set_string("input", d->get_name());

	foreachi(auto *d, input_sources, i)
		if (d == view->mode_midi->input_device())
			set_int("input", i);
}

void simplify_fraction(int &a, int &b) {
	for (int i=2; i<20; i++)
		if ((a % i) == 0 and (b % i) == 0) {
			a /= i;
			b /= i;
		}
}

void MidiEditorConsole::on_base_length(NoteBaseLength l) {
	int partition = 1, length = 1;
	if (l == NoteBaseLength::WHOLE)
		length = 4;
	if (l == NoteBaseLength::HALF)
		length = 2;
	if (l == NoteBaseLength::QUARTER)
		length = 1;
	if (l == NoteBaseLength::EIGTH)
		partition = 2;
	if (l == NoteBaseLength::SIXTEENTH)
		partition = 4;

	if (is_checked("length-dotted")) {
		partition *= 2;
		length *= 3;
	}
	if (is_checked("length-triplet")) {
		partition *= 3;
		length *= 2;
	}

	simplify_fraction(length, partition);
	view->mode_midi->set_note_length_and_partition(length, partition);
}

NoteBaseLength MidiEditorConsole::get_base_length() {
	if (is_checked("length-whole"))
		return NoteBaseLength::WHOLE;
	if (is_checked("length-half"))
		return NoteBaseLength::HALF;
	if (is_checked("length-quarter"))
		return NoteBaseLength::QUARTER;
	if (is_checked("length-eighth"))
		return NoteBaseLength::EIGTH;
	if (is_checked("length-sixteenth"))
		return NoteBaseLength::SIXTEENTH;
	return NoteBaseLength::QUARTER; // ...
}

void MidiEditorConsole::on_length_dotted() {
	auto l = get_base_length();
	on_base_length(l);
}

void MidiEditorConsole::on_length_triplet() {
	auto l = get_base_length();
	on_base_length(l);
}

void MidiEditorConsole::on_length_custom() {
	auto r = QuestionDialogIntInt::ask(win, _("Custom note length and beat sub-partitions (of quarter notes):"), {_("Length"), _("Partition")}, {"range=1:20", "range=1:20"});
	view->mode_midi->set_note_length_and_partition(r.first, r.second);
}

void MidiEditorConsole::on_layer_delete() {
	set_layer(nullptr);
}

void MidiEditorConsole::on_view_cur_layer_change() {
	set_layer(view->cur_layer());
}

void MidiEditorConsole::on_view_vtrack_change() {
	/*update();

	reset("reference_tracks");
	if (song) {
		for (Track *t: song->tracks)
			addString("reference_tracks", t->getNiceName());
	}

	if (layer) {
		//setSelection("reference_tracks", view->get_layer(layer)->reference_tracks);
	}*/
}

void MidiEditorConsole::on_settings_change() {
	update();
}

void MidiEditorConsole::on_creation_mode() {
	int n = get_int("midi_edit_mode");
	if (n == 0) {
		view->mode_midi->set_creation_mode(ViewModeMidi::CreationMode::SELECT);
	} else if (n == 1) {
		view->mode_midi->set_creation_mode(ViewModeMidi::CreationMode::NOTE);
	} else if (n == 2) {
		view->mode_midi->set_creation_mode(ViewModeMidi::CreationMode::INTERVAL);
	} else if (n == 3) {
		view->mode_midi->set_creation_mode(ViewModeMidi::CreationMode::CHORD);
	}
}

void MidiEditorConsole::on_interval() {
	view->mode_midi->midi_interval = get_int("");
}

void MidiEditorConsole::on_chord_type() {
	view->mode_midi->chord_type = (ChordType)get_int("");
}

void MidiEditorConsole::on_chord_inversion() {
	view->mode_midi->chord_inversion = get_int("");
}

void MidiEditorConsole::on_reference_tracks() {
	/*int tn = track->get_index();
	view->vtrack[tn]->reference_tracks = getSelection("");
	view->forceRedraw();*/
}

void MidiEditorConsole::on_edit_track() {
	session->set_mode("default/track");
}

void MidiEditorConsole::on_edit_midi_fx() {
	session->set_mode("default/midi-fx");
}

void MidiEditorConsole::on_edit_song() {
	session->set_mode("default/song");
}

void MidiEditorConsole::on_modifier(NoteModifier m) {
	view->mode_midi->set_modifier(m);
}

void MidiEditorConsole::on_input_active() {
	bool a = is_checked("");
	view->mode_midi->activate_input(a);
	enable("input", a);
	enable("input_volume:key", a);
	enable("input_volume:max", a);
	enable("input_capture", a);
}

void MidiEditorConsole::on_input_capture() {
	bool a = is_checked("");
	view->mode_midi->set_input_capture(a);
}

void MidiEditorConsole::on_input_source() {
	int n = get_int("");
	if (n >= 0 and n < input_sources.num)
		view->mode_midi->set_input_device(input_sources[n]);
}

void MidiEditorConsole::on_input_volume(int mode) {
	view->mode_midi->maximize_input_volume = (mode == 1);
}

void MidiEditorConsole::clear() {
	if (layer)
		layer->unsubscribe(this);
	layer = nullptr;
	set_selection("reference_tracks", {});
}

void MidiEditorConsole::on_enter() {
	session->device_manager->subscribe(this, [=]{ update_input_device_list(); });
	view->subscribe(this, [=]{ on_view_cur_layer_change(); }, view->MESSAGE_CUR_LAYER_CHANGE);
	view->subscribe(this, [=]{ on_view_vtrack_change(); }, view->MESSAGE_VTRACK_CHANGE);
	view->mode_midi->subscribe(this, [=]{ on_settings_change(); });
	set_layer(view->cur_layer());
}

void MidiEditorConsole::on_leave() {
	clear();
	session->device_manager->unsubscribe(this);
	view->mode_midi->unsubscribe(this);
	view->unsubscribe(this);
}

void MidiEditorConsole::set_layer(TrackLayer *l) {
	clear();

	layer = l;
	if (layer) {

		layer->subscribe(this, [=]{ on_layer_delete(); }, layer->MESSAGE_DELETE);

		/*auto v = view->get_layer(layer);
		if (v)
			setSelection("reference_tracks", v->reference_tracks);*/

		int strings = layer->track->instrument.string_pitch.num;

		enable("apply_string", strings > 0);
		enable("string_no", strings > 0);
		set_options("string_no", format("range=1:%d", strings));
		enable("apply_hand_position", strings > 0);
		enable("fret_no", strings > 0);

		update();
	}

}

int align_to_beats(int pos, Array<Beat> &beats) {
	int best = pos;
	int best_diff = 100000000;
	for (auto &b: beats) {
		int d = abs(b.range.offset - pos);
		if (d < best_diff) {
			best_diff = d;
			best = b.range.offset;
		}
	}
	return best;
}

void MidiEditorConsole::on_quantize() {
	auto beats = song->bars.get_beats(Range::ALL, true, true, view->mode_midi->sub_beat_partition);

	song->begin_action_group();
	MidiNoteBufferRef ref = layer->midi.get_notes_by_selection(view->sel);
	for (auto *n: ref) {
		view->sel.set(n, false);
		MidiNote *nn = n->copy();
		nn->range.set_start(align_to_beats(nn->range.start(), beats));
		nn->range.set_end(align_to_beats(nn->range.end(), beats));
		layer->delete_midi_note(n);
		layer->add_midi_note(nn);
		view->sel.add(nn);
	}
	song->end_action_group();
}

void MidiEditorConsole::on_apply_string() {
	int string_no = get_int("string_no") - 1;

	song->begin_action_group();
	MidiNoteBufferRef ref = layer->midi.get_notes_by_selection(view->sel);
	for (auto *n: ref)
		layer->midi_note_set_string(n, string_no);
	song->end_action_group();
}

void MidiEditorConsole::on_apply_hand_position() {
	int hand_position = get_int("fret_no");
	auto &string_pitch = layer->track->instrument.string_pitch;

	song->begin_action_group();
	MidiNoteBufferRef ref = layer->midi.get_notes_by_selection(view->sel);
	for (auto *n: ref) {
 		int stringno = 0;
 		for (int i=0; i<string_pitch.num; i++)
			if (n->pitch >= string_pitch[i] + hand_position) {
 				stringno = i;
 			}
		layer->midi_note_set_string(n, stringno);
	}
	song->end_action_group();
}

void MidiEditorConsole::on_apply_pitch_shift() {
	int delta = get_int("pitch_delta");

	song->begin_action_group();
	MidiNoteBufferRef ref = layer->midi.get_notes_by_selection(view->sel);
	for (auto *n: ref)
		layer->edit_midi_note(n, n->range, n->pitch + delta, n->volume);
	song->end_action_group();
}

void MidiEditorConsole::on_apply_flags(int mask) {
	song->begin_action_group();
	MidiNoteBufferRef ref = layer->midi.get_notes_by_selection(view->sel);
	if (mask == 0) {
		for (auto *n: ref)
			layer->midi_note_set_flags(n, 0);
	} else {
		for (auto *n: ref)
			layer->midi_note_set_flags(n, n->flags | mask);

	}
	song->end_action_group();
}

void MidiEditorConsole::on_add_key_change() {
	auto *dlg = new MarkerDialog(win, layer, Range(view->cursor_pos(), 0), "::key=c-major::");
	dlg->run();
	delete dlg;
}
