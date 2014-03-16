/*
 * BarList.cpp
 *
 *  Created on: 03.12.2012
 *      Author: michi
 */

#include "BarList.h"
#include "../../Tsunami.h"
#include "../../Data/AudioFile.h"



BarList::BarList(HuiWindow *_dlg, const string & _id, const string &_id_add, const string &_id_add_pause, const string &_id_delete)
{
	dlg = _dlg;
	id = _id;
	id_add = _id_add;
	id_add_pause = _id_add_pause;
	id_delete = _id_delete;

	track = NULL;

	FillList();
	dlg->EventM(id, this, &BarList::OnList);
	dlg->EventMX(id, "hui:select", this, &BarList::OnListSelect);
	dlg->EventMX(id, "hui:change", this, &BarList::OnListEdit);
	dlg->EventM(id_add, this, &BarList::OnAdd);
	dlg->EventM(id_add_pause, this, &BarList::OnAddPause);
	dlg->EventM(id_delete, this, &BarList::OnDelete);
}



void BarList::FillList()
{
	msg_db_f("FillBarList", 1);
	dlg->Reset(id);
	if (track){
		int sample_rate = track->root->sample_rate;
		int n = 1;
		foreach(BarPattern &b, track->bar){
			if (b.type == b.TYPE_BAR){
				if (b.count == 1)
					dlg->AddString(id, format("%d\\%d\\%.1f\\%d", n, b.num_beats, sample_rate * 60.0f / (b.length / b.num_beats), b.count));
				else
					dlg->AddString(id, format("%d-%d\\%d\\%.1f\\%d", n, n + b.count - 1, b.num_beats, sample_rate * 60.0f / (b.length / b.num_beats), b.count));
				n += b.count;
			}else if (b.type == b.TYPE_PAUSE){
				dlg->AddString(id, format(_("(Pause)\\-\\-\\%.3f"), (float)b.length / (float)sample_rate));
			}
		}
	}
	dlg->Enable(id_delete, false);
}



void BarList::OnList()
{
	int s = dlg->GetInt(id);
	if (s >= 0){
		ExecuteBarDialog(s);
		FillList();
	}
}


void BarList::OnListSelect()
{
	int s = dlg->GetInt(id);
	dlg->Enable(id_delete, s >= 0);
}


void BarList::OnListEdit()
{
	if (!track)
		return;
	int sample_rate = track->root->sample_rate;
	int index = HuiGetEvent()->row;
	BarPattern b = track->bar[index];
	string text = dlg->GetCell(id, HuiGetEvent()->row, HuiGetEvent()->column);
	if (b.type == b.TYPE_BAR){
		if (HuiGetEvent()->column == 1){
			float l = (float)b.length / (float)b.num_beats;
			b.num_beats = text._int();
			b.length = l * b.num_beats;
		}else if (HuiGetEvent()->column == 2){
			b.length = (int)((float)b.num_beats * (float)sample_rate * 60.0f / text._float());
		}else if (HuiGetEvent()->column == 3){
			b.count = text._int();
		}
	}else if (b.type == b.TYPE_PAUSE){
		if (HuiGetEvent()->column == 3){
			b.length = (int)(text._float() * (float)sample_rate);
		}
	}
	track->EditBar(index, b);
	FillList();
}


void BarList::OnAdd()
{
	AddNewBar();
}


void BarList::OnAddPause()
{
	if (!track)
		return;
	int s = dlg->GetInt(id);

	track->AddPause(s, 2.0f);
	FillList();
}


void BarList::OnDelete()
{
	if (!track)
		return;
	int s = dlg->GetInt(id);
	if (s >= 0){
		track->DeleteBar(s);
		FillList();
	}
}

BarList::~BarList()
{
}

void BarList::AddNewBar()
{
	if (!track)
		return;
	msg_db_f("AddNewBar", 1);

	int s = dlg->GetInt(id);

	track->AddBars(s, 90.0f, 4, 10);
	FillList();
}

void BarList::ExecuteBarDialog(int index)
{
	msg_db_f("ExecuteBarDialog", 1);
}

void BarList::SetTrack(Track *t)
{
	track = NULL;
	if (t)
		if (t->type == t->TYPE_TIME)
			track = t;
	FillList();

	dlg->Enable(id, track);
	dlg->Enable(id_add, track);
	dlg->Enable(id_add_pause, track);
}

