/*
 * AudioViewLayer.h
 *
 *  Created on: 25.10.2014
 *      Author: michi
 */

#ifndef SRC_VIEW_AUDIOVIEWLAYER_H_
#define SRC_VIEW_AUDIOVIEWLAYER_H_

#include "../lib/math/math.h"
#include "../Stuff/Observable.h"

class Track;
class TrackLayer;
class Painter;
class AudioView;
class AudioBuffer;
class SampleRef;
class MidiNoteBuffer;
class MidiNote;
class MidiEvent;
class TrackMarker;
class Clef;
class Scale;
class Range;
class GridColors;
enum class NoteModifier;
enum class MidiMode;


class AudioViewLayer : public Observable<VirtualBase>
{
public:
	AudioViewLayer(AudioView *v, TrackLayer *l);
	virtual ~AudioViewLayer(){}


	color background_color();
	color background_selection_color();
	void draw_blank_background(Painter *c);

	void draw_track_buffers(Painter *c);
	void draw_buffer(Painter *c, AudioBuffer &b, double view_pos_rel, const color &col, float x0, float x1);
	void draw_buffer_selection(Painter *c, AudioBuffer &b, double view_pos_rel, const color &col, const Range &r);

	void draw_sample_frame(Painter *c, SampleRef *s, const color &col, int delay);
	void draw_sample(Painter *c, SampleRef *s);

	void draw_marker(Painter *c, const TrackMarker *marker, int index, bool hover);

	void draw_version_header(Painter *c);
	virtual void draw(Painter *c);

	bool on_screen();

	GridColors grid_colors();

	TrackLayer *layer;
	rect area;
	rect area_last, area_target;
	int height_wish, height_min;
	Array<rect> marker_areas;
	Array<rect> marker_label_areas;
	AudioView *view;
	void set_midi_mode(MidiMode wanted);
	MidiMode midi_mode;


	static color pitch_color(int pitch);
	static color marker_color(const TrackMarker *m);

	void set_edit_pitch_min_max(int pitch_min, int pitch_max);
	int edit_pitch_min, edit_pitch_max;

	virtual bool is_playable();


	void set_solo(bool solo);
	bool solo;


	bool mouse_over();

	bool hidden;
	bool represents_imploded;
};

#endif /* SRC_VIEW_AUDIOVIEWLAYER_H_ */
