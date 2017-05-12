/*
 * BarEditDialog.cpp
 *
 *  Created on: 30.10.2015
 *      Author: michi
 */

#include "BarEditDialog.h"
#include "../../Data/Song.h"

BarEditDialog::BarEditDialog(hui::Window *root, Song *_song, const Range &_bars, bool _apply_to_midi):
	hui::Dialog("", 100, 100, root, false)
{
	fromResource("bar_edit_dialog");
	song = _song;
	for (int i=_bars.start(); i<_bars.end(); i++)
		sel.add(i);
	apply_to_midi = _apply_to_midi;

	BarPattern &b = song->bars[sel[0]];
	setInt("beats", b.num_beats);
	setInt("sub_beats", b.sub_beats);
	setFloat("bpm", song->sample_rate * 60.0f / (b.length / b.num_beats));

	event("ok", std::bind(&BarEditDialog::onOk, this));
	event("cancel", std::bind(&BarEditDialog::onClose, this));
	event("hui:close", std::bind(&BarEditDialog::onClose, this));
	event("beats", std::bind(&BarEditDialog::onBeats, this));
	event("sub_beats", std::bind(&BarEditDialog::onSubBeats, this));
	event("bpm", std::bind(&BarEditDialog::onBpm, this));
}

void BarEditDialog::onOk()
{
	int beats = getInt("beats");
	int sub_beats = getInt("sub_beats");
	float bpm = getFloat("bpm");
	bool edit_beats = isChecked("edit_beats");
	bool edit_sub_beats = isChecked("edit_sub_beats");
	bool edit_bpm = isChecked("edit_bpm");
	song->action_manager->beginActionGroup();
	foreachb(int i, sel){
		BarPattern b = song->bars[i];
		if (edit_beats)
			b.num_beats = beats;
		if (edit_sub_beats)
			b.sub_beats = sub_beats;
		if (edit_bpm)
			b.length = song->sample_rate * 60.0f * b.num_beats / bpm;
		song->editBar(i, b, apply_to_midi);
	}
	song->action_manager->endActionGroup();

	destroy();
}

void BarEditDialog::onClose()
{
	destroy();
}

void BarEditDialog::onBeats()
{
	check("edit_beats", true);
}

void BarEditDialog::onSubBeats()
{
	check("edit_sub_beats", true);
}

void BarEditDialog::onBpm()
{
	check("edit_bpm", true);
}
