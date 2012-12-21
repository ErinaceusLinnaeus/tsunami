/*
 * TrackDialog.cpp
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#include "TrackDialog.h"
#include "../../Data/Track.h"
#include "../Helper/Slider.h"
#include "../Helper/FxList.h"
#include "../Helper/BarList.h"
#include "../../Tsunami.h"

TrackDialog::TrackDialog(CHuiWindow *win):
	EmbeddedDialog(win)
{
	track = NULL;
	win->SetTarget("track_dialog_table", 0);
	win->SetBorderWidth(8);
	win->EmbedDialog("track_time_dialog", 0, 0);
	win->SetDecimals(1);
	volume_slider = new Slider(win, "volume_slider", "volume", 0, 2, 100, (void(HuiEventHandler::*)())&TrackDialog::OnVolume, 0, this);
	fx_list = new FxList(win, "fx_list", "add_effect", "configure_effect", "delete_effect");
	bar_list = new BarList(win, "bar_list", "add_bar", "add_bar_pause", "delete_bar");

	LoadData();
	Subscribe(tsunami->audio);

	win->EventM("name", this, (void(HuiEventHandler::*)())&TrackDialog::OnName);
	win->EventM("mute", this, (void(HuiEventHandler::*)())&TrackDialog::OnMute);
	win->EventM("close", this, (void(HuiEventHandler::*)())&TrackDialog::OnClose);
}

TrackDialog::~TrackDialog()
{
	Unsubscribe(tsunami->audio);
	delete(volume_slider);
	delete(fx_list);
	if (bar_list)
		delete(bar_list);
}

void TrackDialog::LoadData()
{
	Enable("name", track);
	Enable("mute", track);
	fx_list->SetTrack(track);
	bar_list->SetTrack(track);
	if (track){
		SetString("name", track->name);
		Check("mute", track->muted);
		volume_slider->Set(track->volume);
		volume_slider->Enable(!track->muted);
	}else{
		volume_slider->Enable(false);
	}
}

void TrackDialog::SetTrack(Track *t)
{
	track = t;
	LoadData();
}

void TrackDialog::OnName()
{
	track->SetName(GetString(""));
}

void TrackDialog::OnVolume()
{
	track->SetVolume(volume_slider->Get());
}

void TrackDialog::OnMute()
{
	track->SetMuted(IsChecked(""));
}

void TrackDialog::OnClose()
{
	win->HideControl("track_dialog_table", true);
}

void TrackDialog::OnUpdate(Observable *o)
{
	if (!track)
		return;
	bool ok = false;
	foreach(Track *t, tsunami->audio->track)
		if (track == t)
			ok = true;
	if (ok)
		LoadData();
	else
		SetTrack(NULL);
}
