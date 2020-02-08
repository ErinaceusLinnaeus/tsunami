/*
 * TrackRenderer.h
 *
 *  Created on: 02.09.2018
 *      Author: michi
 */

#ifndef SRC_MODULE_AUDIO_TRACKRENDERER_H_
#define SRC_MODULE_AUDIO_TRACKRENDERER_H_

#include "AudioSource.h"

class Track;
class TrackLayer;
class AudioEffect;
class Synthesizer;
class MidiEventStreamer;
class SongRenderer;
class AudioBuffer;

class TrackRenderer : public AudioSource {
	friend SongRenderer;
public:
	TrackRenderer(Track *t, SongRenderer *sr);
	virtual ~TrackRenderer();

	Track *track;
	Array<TrackLayer*> layers;
	Array<AudioEffect*> fx;
	Synthesizer *synth;
	MidiEventStreamer* midi_streamer;
	SongRenderer *song_renderer;
	bool direct_mode;
	int offset;

	float peak;

	void set_pos(int pos);

	void render_audio_versioned(AudioBuffer &buf);
	void render_audio_layered(AudioBuffer &buf);
	void render_audio(AudioBuffer &buf);
	void render_time(AudioBuffer &buf);
	void render_midi(AudioBuffer &buf);
	void render_group(AudioBuffer &buf);
	void render_no_fx(AudioBuffer &buf);
	int read(AudioBuffer &buf) override;

	void fill_midi_streamer();
	int get_first_usable_layer();

	static void apply_fx(AudioBuffer &buf, Array<AudioEffect*> &fx_list);

	void reset_state();

	void update_layers();

	void on_track_replace_synth();
	void on_track_add_or_delete_fx();
	void on_track_change_data();
	//void on_layer_change_data();
};

#endif /* SRC_MODULE_AUDIO_TRACKRENDERER_H_ */
