/*
 * MidiPainter.h
 *
 *  Created on: 13.11.2018
 *      Author: michi
 */

#ifndef SRC_VIEW_PAINTER_MIDIPAINTER_H_
#define SRC_VIEW_PAINTER_MIDIPAINTER_H_

#include <functional>
#include "midi/MidiPainterModeClassical.h"
#include "../../lib/base/base.h"
#include "../../lib/math/rect.h"
#include "../../data/Range.h"
#include "../../data/midi/Scale.h"

class AudioView;
class ViewPort;
class SongSelection;
class HoverData;
class Painter;
class MidiNote;
class MidiNoteBuffer;
class Clef;
class Scale;
class Song;
class Range;
class color;
class Instrument;
class Synthesizer;
class ColorScheme;
enum class NoteModifier;
enum class MidiMode;
struct QuantizedNote;
class QuantizedNoteGroup;
class PluginManager;

class MidiKeyChange {
public:
	MidiKeyChange();
	MidiKeyChange(double pos, const Scale &key);
	double pos;
	Scale key;
};


enum class MidiNoteState {
	DEFAULT = 0,
	HOVER = 1,
	SELECTED = 2,
	REFERENCE = 4,
};
inline bool operator&(MidiNoteState a, MidiNoteState b) {
	return (int)a & (int)b;
}
inline MidiNoteState operator|(MidiNoteState a, MidiNoteState b) {
	return (MidiNoteState)((int)a | (int)b);
}


class MidiPainter {
	friend class PluginManager;
	friend class MidiPainterModeClassical;
public:
	MidiPainter(Song *song, ViewPort *cam, SongSelection *sel, HoverData *hover, ColorScheme &colors);
	void __init__(Song *song, ViewPort *cam, SongSelection *sel, HoverData *hover, ColorScheme &colors);


	static const int PITCH_MIN_DEFAULT = 25;
	static const int PITCH_MAX_DEFAULT = 105;

	static color pitch_color(int pitch);

	void set_synthesizer(Synthesizer *synth);
	Synthesizer *synth = nullptr;

private:
	void draw_pitch_grid(Painter *c);
	void draw_clef_tab(Painter *c);

	void draw_rhythm(Painter *c, const MidiNoteBuffer &midi, const Range &range, std::function<float(MidiNote*)> y_func);

	void draw_simple_note(Painter *c, float x1, float x2, float y, float rx, const color &col, const color &col_shadow, bool force_circle);

	void draw_note_flags(Painter *c, const MidiNote *n, MidiNoteState state, float x1, float x2, float y);

	void draw_note_linear(Painter *c, const MidiNote &n, MidiNoteState state);
	void draw_linear(Painter *c, const MidiNoteBuffer &midi);
	void draw_note_tab(Painter *c, const MidiNote *n, MidiNoteState state);
	void draw_tab(Painter *c, const MidiNoteBuffer &midi);

	void draw_low_detail_dummy(Painter *c, const MidiNoteBuffer &midi);
	void draw_low_detail_dummy_part(Painter *c, const Range &r, const MidiNoteBuffer &midi);

public:
	void _draw_notes(Painter *p, const MidiNoteBuffer &notes);
	void draw(Painter *c, const MidiNoteBuffer &midi);
	void draw_background(Painter *c, bool force = false);

	// some is exposed to rhythm...
	ViewPort *cam;
	Scale midi_scale;
	SongSelection *sel;
	HoverData *hover;
	Song *song;
	ColorScheme &local_theme;
	rect area;
	bool is_playable;
	int shift;
	bool as_reference;
	bool allow_shadows;

public:
	void set_context(const rect &area, const Instrument &i, bool playable, MidiMode mode);
	void set_line_weight(float s);
	void set_shift(int shift);
	void set_linear_range(float pitch_min, float pitch_max);
	void set_quality(float quality, bool antialiasing);
	void set_force_shadows(bool force);
	void set_key_changes(const Array<MidiKeyChange> &changes);



private:
	const Instrument *instrument;
	const Clef *clef;
	float pitch_min, pitch_max;
	bool force_shadows;
	Array<MidiKeyChange> key_changes;

private:
	struct {
		// configuration
		bool antialiasing;
		float rhythm_zoom_min;
		float notes_zoom_min;
		float shadow_threshold;
		float note_circle_threshold;
		float tab_text_threshold;
		int note_count_threshold;
		float factor;

		// current state
		bool _highest_details;
	} quality;

	float clef_dy;
	float clef_y0;

	float string_dy;
	float string_y0;
	float clef_line_width;
	MidiMode mode;
	MidiPainterModeClassical mode_classical;
	MidiPainterMode mode_dummy;
	MidiPainterMode *mmode = nullptr;
	float rr;
	float modifier_font_size;
	Range cur_range;


public:
	float clef_pos_to_screen(int pos);
	int screen_to_clef_pos(float y);
	float string_to_screen(int string);
	int screen_to_string(float y);

	float pitch2y_classical(int pitch);
	float pitch2y_linear(float pitch);
	int y2pitch_classical(float y, NoteModifier modifier);
	int y2pitch_linear(float y);
	int y2clef_classical(float y, NoteModifier &mod);
	int y2clef_linear(float y, NoteModifier &mod);

	float note_r() const;
	float get_clef_dy() const;

private:
	float scale;
	float neck_length_single;
	float neck_length_group;
	float neck_width;
	float bar_distance;
	float bar_width;
	float flag_dx, flag_dy;
	void draw_single_ndata(Painter *c, QuantizedNote &d, bool neck_offset);
	void draw_group_ndata(Painter *c, const QuantizedNoteGroup &d, bool neck_offset);
};

#endif /* SRC_VIEW_PAINTER_MIDIPAINTER_H_ */
