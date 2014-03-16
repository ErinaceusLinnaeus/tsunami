/*
 * MixingConsole.cpp
 *
 *  Created on: 16.03.2014
 *      Author: michi
 */

#include "MixingConsole.h"
#include "../../Data/AudioFile.h"
#include <math.h>

const float TrackMixer::DB_MIN = -100;
const float TrackMixer::DB_MAX = 10;
const float TrackMixer::TAN_SCALE = 10.0f;

TrackMixer::TrackMixer(int _index, HuiWindow *win) :
	EmbeddedDialog(win)
{
	index = _index;
	win->SetTarget("mixing_table", 0);
	id_grid = "mixing_track_table_" + i2s(index);
	win->AddControlTable("", index, 0, 1, 4, id_grid);
	win->SetTarget("mixing_track_table_" + i2s(index), 0);
	id_name = "mc_track_name_" + i2s(index);
	win->AddText("Track " + i2s(index+1), 0, 0, 0, 0, id_name);
	vol_slider_id = "mc_volume_" + i2s(index);
	pan_slider_id = "mc_panning_" + i2s(index);
	mute_id = "mc_mute_" + i2s(index);
	win->AddSlider("!width=80,noorigin", 0, 1, 0, 0, pan_slider_id);
	win->AddString(pan_slider_id, "0\\L");
	win->AddString(pan_slider_id, "0.5\\");
	win->AddString(pan_slider_id, "1\\R");
	win->SetTooltip(pan_slider_id, "Balance");
	win->AddSlider("!vertical,expandy", 0, 2, 0, 0, vol_slider_id);
	win->AddString(vol_slider_id, format("%f\\%d", db2slider(DB_MAX), (int)DB_MAX));
	win->AddString(vol_slider_id, format("%f\\%d", db2slider(5), (int)5));
	win->AddString(vol_slider_id, format("%f\\%d", db2slider(0), 0));
	win->AddString(vol_slider_id, format("%f\\%d", db2slider(-5), (int)-5));
	win->AddString(vol_slider_id, format("%f\\%d", db2slider(-10), (int)-10));
	win->AddString(vol_slider_id, format("%f\\%d", db2slider(DB_MIN), (int)DB_MIN));
	win->SetTooltip(vol_slider_id, _("Lautst&arke in dB"));
	win->AddCheckBox("Stumm", 0, 3, 0, 0, mute_id);

	win->EventM(vol_slider_id, this, &TrackMixer::OnVolume);
	win->EventM(pan_slider_id, this, &TrackMixer::OnPanning);
	win->EventM(mute_id, this, &TrackMixer::OnMute);
}

TrackMixer::~TrackMixer()
{
	win->RemoveControl(id_grid);
}

float TrackMixer::slider2db(float val)
{
	return tan(atan(DB_MIN / TAN_SCALE) + val * (atan(DB_MAX / TAN_SCALE)- atan(DB_MIN / TAN_SCALE))) * TAN_SCALE;
}

float TrackMixer::db2slider(float db)
{
	return (atan(db / TAN_SCALE) - atan(DB_MIN / TAN_SCALE)) / (atan(DB_MAX / TAN_SCALE) - atan(DB_MIN / TAN_SCALE));
}

float TrackMixer::slider2vol(float val)
{
	return db2amplitude(slider2db(val));
}

float TrackMixer::vol2slider(float vol)
{
	return db2slider(amplitude2db(vol));
}

void TrackMixer::OnVolume()
{
	track->SetVolume(slider2vol(win->GetFloat("")));
}

void TrackMixer::OnMute()
{
	track->SetMuted(IsChecked(""));
}

void TrackMixer::OnPanning()
{
	track->SetPanning(win->GetFloat("")*2 - 1);
}

void TrackMixer::SetTrack(Track* t)
{
	track = t;
	Update();
}

void TrackMixer::Update()
{
	win->SetFloat(vol_slider_id, vol2slider(track->volume));
	win->SetFloat(pan_slider_id, track->panning * 0.5f + 0.5f);
	win->Check(mute_id, track->muted);
	win->SetString(id_name, track->name);
}


MixingConsole::MixingConsole(AudioFile *_audio, HuiWindow* win) :
	EmbeddedDialog(win),
	Observable("MixingConsole")
{
	enabled = false;
	audio = _audio;

	Subscribe(audio);
}

MixingConsole::~MixingConsole()
{
	Unsubscribe(audio);
	foreach(TrackMixer *m, mixer)
		delete(m);
}

void MixingConsole::Show(bool show)
{
	enabled = show;
	win->HideControl("mixing_table", !enabled);
	Notify("Show");
}

void MixingConsole::LoadData()
{
	for (int i=mixer.num; i<audio->track.num; i++)
		mixer.add(new TrackMixer(i, win));
	for (int i=audio->track.num; i<mixer.num; i++)
		delete(mixer[i]);
	mixer.resize(audio->track.num);

	foreachi(Track *t, audio->track, i)
		mixer[i]->SetTrack(t);
}

void MixingConsole::OnUpdate(Observable* o)
{
	LoadData();
}
