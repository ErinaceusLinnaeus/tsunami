/*
 * SongConsole.cpp
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#include "../../Tsunami.h"
#include "../../TsunamiWindow.h"
#include "../../Stuff/Observer.h"
#include "../Helper/BarList.h"
#include "../../Stuff/Log.h"
#include "../../View/AudioView.h"
#include "../BottomBar/BottomBar.h"
#include "../../Data/Song.h"
#include "SongConsole.h"

const int NUM_POSSIBLE_FORMATS = 4;
const SampleFormat POSSIBLE_FORMATS[NUM_POSSIBLE_FORMATS] = {
	SAMPLE_FORMAT_16,
	SAMPLE_FORMAT_24,
	SAMPLE_FORMAT_32,
	SAMPLE_FORMAT_32_FLOAT
};

SongConsole::SongConsole(Song *a) :
	SideBarConsole(_("Datei Eigenschaften")),
	Observer("AudioFileConsole")
{
	song = a;

	// dialog
	setBorderWidth(5);
	embedDialog("song_dialog", 0, 0);
	setDecimals(1);
	bar_list = new BarList(this, "audio_bar_list", "audio_add_bar", "audio_add_bar_pause", "audio_delete_bar", "----", NULL);
	hideControl("ad_t_bars", true);

	expand("ad_t_tags", 0, true);

	addString("samplerate", "22050");
	addString("samplerate", i2s(DEFAULT_SAMPLE_RATE));
	addString("samplerate", "48000");
	addString("samplerate", "96000");

	setTooltip("tags", _("Vorschlag:\n* title\n* artist\n* album\n* tracknumber\n* year/date\n* genre"));
	setTooltip("format", _("beim Speichern"));
	setTooltip("compress", _("beim Speichern"));

	for (int i=0; i<NUM_POSSIBLE_FORMATS; i++)
		addString("format", format_name(POSSIBLE_FORMATS[i]));

	loadData();

	event("samplerate", this, &SongConsole::onSamplerate);
	event("format", this, &SongConsole::onFormat);
	event("compress", this, &SongConsole::onCompression);
	eventX("tags", "hui:select", this, &SongConsole::onTagsSelect);
	eventX("tags", "hui:change", this, &SongConsole::onTagsEdit);
	event("add_tag", this, &SongConsole::onAddTag);
	event("delete_tag", this, &SongConsole::onDeleteTag);

	event("edit_levels", this, &SongConsole::onEditLevels);
	event("edit_samples", this, &SongConsole::onEditSamples);
	event("edit_fx", this, &SongConsole::onEditFx);

	subscribe(song);
}

SongConsole::~SongConsole()
{
	unsubscribe(song);
	delete(bar_list);
}

void SongConsole::loadData()
{
	// tags
	reset("tags");
	foreach(Tag &t, song->tags)
		addString("tags", t.key + "\\" + t.value);
	enable("delete_tag", false);

	// data
	reset("data_list");
	int samples = song->getRange().length();
	addString("data_list", _("Anfang\\") + song->get_time_str_long(song->getRange().start()));
	addString("data_list", _("Ende\\") + song->get_time_str_long(song->getRange().end()));
	addString("data_list", _("Dauer\\") + song->get_time_str_long(samples));
	addString("data_list", _("Samples\\") + i2s(samples));
	//addString("data_list", _("Abtastrate\\") + i2s(audio->sample_rate) + " Hz");

	setString("samplerate", i2s(song->sample_rate));
	for (int i=0; i<NUM_POSSIBLE_FORMATS; i++)
		if (song->default_format == POSSIBLE_FORMATS[i])
			setInt("format", i);
	check("compress", song->compression > 0);
}

void SongConsole::onSamplerate()
{
	song->setSampleRate(getString("")._int());
}

void SongConsole::onFormat()
{
	int i = getInt("");
	if (i >= 0)
		song->setDefaultFormat(POSSIBLE_FORMATS[i]);
}

void SongConsole::onCompression()
{
	song->setCompression(isChecked("") ? 1 : 0);
}

void SongConsole::onTagsSelect()
{
	int s = getInt("tags");
	enable("delete_tag", s >= 0);
}

void SongConsole::onTagsEdit()
{
	int r = HuiGetEvent()->row;
	if (r < 0)
		return;
	Tag t = song->tags[r];
	if (HuiGetEvent()->column == 0)
		t.key = getCell("tags", r, 0);
	else
		t.value = getCell("tags", r, 1);
	song->editTag(r, t.key, t.value);
}

void SongConsole::onAddTag()
{
	song->addTag("key", "value");
}

void SongConsole::onDeleteTag()
{
	int s = getInt("tags");
	if (s >= 0)
		song->deleteTag(s);
}

void SongConsole::onEditLevels()
{
	((SideBar*)parent)->open(SideBar::LEVEL_CONSOLE);
}

void SongConsole::onEditSamples()
{
	((SideBar*)parent)->open(SideBar::SAMPLE_CONSOLE);
}

void SongConsole::onEditFx()
{
	tsunami->win->bottom_bar->open(BottomBar::FX_CONSOLE);
}

void SongConsole::onUpdate(Observable *o, const string &message)
{
	loadData();
}
