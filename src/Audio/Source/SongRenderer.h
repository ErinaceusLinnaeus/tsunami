/*
 * SongRenderer.h
 *
 *  Created on: 17.08.2015
 *      Author: michi
 */

#ifndef SRC_AUDIO_SOURCE_SONGRENDERER_H_
#define SRC_AUDIO_SOURCE_SONGRENDERER_H_

#include "../Source/AudioSource.h"

class MidiDataStreamer;

class SongRenderer : public AudioSource
{
public:
	SongRenderer(Song *s);
	virtual ~SongRenderer();

	void _cdecl __init__(Song *s);
	virtual void _cdecl __delete__();

	// from AudioSource
	virtual int _cdecl read(AudioBuffer &buf);
	virtual void _cdecl reset();
	virtual int _cdecl getSampleRate();

	void _cdecl render(const Range &range, AudioBuffer &buf);
	void _cdecl prepare(const Range &range, bool alllow_loop);
	void _cdecl allowTracks(const Set<Track*> &allowed_tracks);

	void _cdecl seek(int pos);

	void _cdecl setRange(const Range &r){ _range = r; }
	Range _cdecl range(){ return _range; }
	int _cdecl getPos(){ return pos; }

	int _cdecl getNumSamples();

private:
	void read_basic(AudioBuffer &buf, int pos, int size);
	void bb_render_audio_track_no_fx(AudioBuffer &buf, Track *t);
	void bb_render_time_track_no_fx(AudioBuffer &buf, Track *t);
	void bb_render_midi_track_no_fx(AudioBuffer &buf, Track *t, int ti);
	void bb_render_track_no_fx(AudioBuffer &buf, Track *t, int ti);
	void bb_apply_fx(AudioBuffer &buf, Track *t, Array<Effect*> &fx_list);
	void bb_render_track_fx(AudioBuffer &buf, Track *t, int ti);
	void bb_render_song_no_fx(AudioBuffer &buf);

	Song *song;
	Range _range;
	Range range_cur;
	int pos;
	Array<MidiData> midi;
	Set<Track*> allowed_tracks;

	MidiDataStreamer *midi_streamer;

public:
	Effect *preview_effect;
	bool allow_loop;
	bool loop_if_allowed;
};

#endif /* SRC_AUDIO_SOURCE_SONGRENDERER_H_ */
