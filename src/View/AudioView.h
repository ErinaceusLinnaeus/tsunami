/*
 * AudioView.h
 *
 *  Created on: 24.03.2012
 *      Author: michi
 */

#ifndef AUDIOVIEW_H_
#define AUDIOVIEW_H_

#include "../Data/SongSelection.h"
#include "../Data/Midi/Scale.h"
#include "../Stuff/Observable.h"
#include "TrackHeightManager.h"
#include "ViewPort.h"
#include "Selection.h"
#include "ColorScheme.h"
#include <atomic>

namespace hui{
	class Menu;
}

class Song;
class Track;
class TrackLayer;
class Sample;
class SampleRef;
class AudioBuffer;
class DeviceManager;
class OutputStream;
class SongRenderer;
class PeakMeter;
class TsunamiWindow;
class AudioViewTrack;
class AudioViewLayer;
class PeakThread;
class ViewMode;
class ViewModeDefault;
class ViewModeMidi;
class ViewModeScaleBars;
class ViewModeCurve;
class ViewModeCapture;
class ScrollBar;
class Session;


enum class MidiMode{
	LINEAR,
	TAB,
	CLASSICAL,
	DRUM
};


class AudioView : public Observable<VirtualBase>
{
public:
	AudioView(Session *session, const string &id);
	virtual ~AudioView();

	void check_consistency();
	void force_redraw();
	void force_redraw_part(const rect &r);

	void on_draw(Painter *p);
	void on_mouse_move();
	void on_left_button_down();
	void on_left_button_up();
	void on_middle_button_down();
	void on_middle_button_up();
	void on_right_button_down();
	void on_right_button_up();
	void on_left_double_click();
	void on_mouse_leave();
	void on_mouse_wheel();
	void on_key_down();
	void on_key_up();
	void on_command(const string &id);

	void on_song_update();
	void on_stream_update();
	void on_stream_state_change();
	void on_stream_end_of_stream();
	void on_update();
	static const string MESSAGE_CUR_TRACK_CHANGE;
	static const string MESSAGE_CUR_SAMPLE_CHANGE;
	static const string MESSAGE_CUR_LAYER_CHANGE;
	static const string MESSAGE_SELECTION_CHANGE;
	static const string MESSAGE_SETTINGS_CHANGE;
	static const string MESSAGE_VIEW_CHANGE;
	static const string MESSAGE_VTRACK_CHANGE;
	static const string MESSAGE_INPUT_CHANGE;
	static const string MESSAGE_OUTPUT_STATE_CHANGE;
	static const string MESSAGE_SOLO_CHANGE;

	void update_peaks();
	void zoom_in();
	void zoom_out();

	void draw_time_scale(Painter *c);
	void draw_grid_time(Painter *c, const rect &r, const color &col, const color &col_sel, const color &bg, const color &bg_sel, bool show_text);
	void draw_grid_bars(Painter *c, const rect &r, const color &col, const color &col_sel, const color &bg, const color &bg_sel, int beat_partition);
	void draw_bar_numbers(Painter *c, const rect &r, const color &col, const color &col_sel, const color &bg, const color &bg_sel);
	void draw_time_line(Painter *c, int pos, int type, const color &col, bool show_time = false);
	void draw_selection(Painter *c);
	void draw_background(Painter *c);
	void draw_song(Painter *c);

	static rect get_boxed_str_rect(Painter *c, float x, float y, const string &str);
	static void draw_boxed_str(Painter *c, float x, float y, const string &str, const color &col_text, const color &col_bg, int align=1);

	static void draw_cursor_hover(Painter *c, const string &msg, float mx, float my, const rect &area);
	void draw_cursor_hover(Painter *c, const string &msg);

	void optimize_view();
	void update_menu();

	string id;

	Array<ColorSchemeBasic> basic_schemes;
	static ColorSchemeBasic basic_colors;
	static ColorScheme colors;
	void set_color_scheme(const string &name);

	static const int SAMPLE_FRAME_HEIGHT;
	static const int TIME_SCALE_HEIGHT;
	static const float LINE_WIDTH;
	static const float CORNER_RADIUS;
	static const int FONT_SIZE;
	static const int MAX_TRACK_CHANNEL_HEIGHT;
	static const int TRACK_HANDLE_WIDTH;
	static const int LAYER_HANDLE_WIDTH;
	static const int TRACK_HANDLE_HEIGHT;
	static const int TRACK_HANDLE_HEIGHT_SMALL;
	static const int SCROLLBAR_WIDTH;
	static const int SNAPPING_DIST;

	Selection hover;
	Selection hover_before_leave;
	SongSelection sel;
	SongSelection sel_temp;

	enum class SelectionMode{
		NONE,
		TIME,
		RECT,
		TRACK_RECT,
		FAKE,
	};
	SelectionMode selection_mode;
	bool hide_selection;


	void snap_to_grid(int &pos);

	void _cdecl unselect_all_samples();

	bool enabled;
	void enable(bool enabled);

	float ScrollSpeed;
	float ScrollSpeedFast;
	float ZoomSpeed;
	float mouse_wheel_speed;

	int mx, my;

	struct MouseSelectionPlanner
	{
		float dist;
		int start_pos;
		int start_y;
		int min_move_to_select;
		void start(int pos, int y);
		bool step();
		bool selecting();
		void stop();
	}msp;

	void select_none();
	void select_all();
	void select_expand();
	void update_selection();
	void set_selection(const SongSelection &s);
	Range get_playback_selection(bool for_recording);

	void set_mouse();
	int mouse_over_sample(SampleRef *s);
	void selection_update_pos(Selection &s);
	bool mouse_over_time(int pos);

	void select_sample(SampleRef *s, bool diff);

	int detail_steps;
	int preview_sleep_time;
	bool antialiasing;


	void set_midi_view_mode(MidiMode mode);
	MidiMode midi_view_mode;

	ViewMode *mode;
	void set_mode(ViewMode *m);
	ViewModeDefault *mode_default;
	ViewModeMidi *mode_midi;
	ViewModeScaleBars *mode_scale_bars;
	ViewModeCurve *mode_curve;
	ViewModeCapture *mode_capture;

	Session *session;
	TsunamiWindow *win;

	Song *song;

	OutputStream *stream;
	bool playback_active;
	SongRenderer *renderer;
	PeakMeter *peak_meter;
	void play(const Range &range, bool allow_loop);
	void stop();
	void pause(bool pause);
	bool is_playback_active();
	bool is_paused();
	int playback_pos();
	Set<Track*> get_playable_tracks();
	Set<Track*> get_selected_tracks();
	bool has_any_solo_track();
	Set<TrackLayer*> get_playable_layers();
	bool has_any_solo_layer(Track *t);

	void set_cur_sample(SampleRef *s);
	//void setCurTrack(Track *t);
	void set_cur_layer(AudioViewLayer *l);
	AudioViewLayer *cur_vlayer;
	Track *_prev_cur_track;
	Track *cur_track();
	SampleRef *cur_sample;
	TrackLayer *cur_layer();

	bool editing_track(Track *t);
	bool editing_layer(AudioViewLayer *l);


	void set_scale(const Scale &s);
	Scale midi_scale;

	rect area;
	rect song_area;
	rect clip;
	TrackHeightManager thm;

	ViewPort cam;

	ScrollBar *scroll;

	Array<AudioViewTrack*> vtrack;
	Array<AudioViewLayer*> vlayer;
	AudioViewLayer *metronome_overlay_vlayer;
	AudioViewTrack *dummy_vtrack;
	AudioViewLayer *dummy_vlayer;
	AudioViewTrack *get_track(Track *track);
	AudioViewLayer *get_layer(TrackLayer *layer);
	void update_tracks();

	void update_peaks_now(AudioBuffer &buf);

	int prefered_buffer_layer;
	double buffer_zoom_factor;
	void update_buffer_zoom();

	PeakThread *peak_thread;

	struct ImageData
	{
		Image *speaker, *x, *solo;
		Image *speaker_bg, *x_bg, *solo_bg;
		Image *track_audio, *track_time, *track_midi;
		Image *track_audio_bg, *track_time_bg, *track_midi_bg;
	};
	ImageData images;

	hui::Menu *menu_track;
	hui::Menu *menu_time_track;
	hui::Menu *menu_layer;
	hui::Menu *menu_sample;
	hui::Menu *menu_marker;
	hui::Menu *menu_bar;
	hui::Menu *menu_song;

	int perf_channel;
};

#endif /* AUDIOVIEW_H_ */
