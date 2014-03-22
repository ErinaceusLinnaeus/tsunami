/*
 * FxPanel.cpp
 *
 *  Created on: 20.03.2014
 *      Author: michi
 */

#include "FxPanel.h"
#include "../../Data/Track.h"
#include "../../Plugins/Effect.h"

class SingleFxPanel : public HuiPanel
{
public:
	SingleFxPanel(Track *t, Effect *_fx, int _index)
	{
		track = t;
		fx = _fx;
		index = _index;
		AddControlTable("!noexpandx", 0, 0, 1, 2, "grid");
		SetTarget("grid", 0);
		AddControlTable("", 0, 0, 5, 1, "header");
		SetTarget("header", 0);
		AddText("!bold,center,expandx\\" + fx->name, 0, 0, 0, 0, "");
		AddButton("!flat", 1, 0, 0, 0, "clear");
		SetImage("clear", "hui:clear");
		SetTooltip("clear", _("auf Standard Parameter zur&ucksetzen"));
		AddButton("!flat", 2, 0, 0, 0, "load");
		SetImage("load", "hui:open");
		SetTooltip("load", _("Parameter aus Favoriten laden"));
		AddButton("!flat", 3, 0, 0, 0, "save");
		SetImage("save", "hui:save");
		SetTooltip("save", _("Parameter in Favoriten speichern"));
		AddButton("!flat", 4, 0, 0, 0, "delete");
		SetImage("delete", "hui:delete");
		SetTooltip("delete", _("Effekt l&oschen"));
		HuiPanel *p = fx->CreatePanel();
		if (p){
			Embed(p, "grid", 0, 1);
		}else{
			SetTarget("grid", 0);
			AddText(_("nicht konfigurierbar"), 0, 1, 0, 0, "");
			HideControl("clear", true);
			HideControl("load", true);
			HideControl("save", true);
		}

		EventM("clear", this, &SingleFxPanel::onClear);
	}
	void onClear()
	{
		string old_param = fx->ConfigToString();
		fx->ResetConfig();
		track->EditEffect(index, old_param);
	}
	Track *track;
	Effect *fx;
	int index;
};

FxPanel::FxPanel(AudioFile *_audio)
{
	audio = _audio;
	id_inner = "mixing_inner_table";

	AddControlTable("!height=250,noexpandy", 0, 0, 2, 1, "root_grid");
	SetTarget("root_grid", 0);
	AddControlTable("", 0, 0, 1, 3, "button_grid");
	AddControlTable("", 1, 0, 1, 20, id_inner);
	SetTarget("button_grid", 0);
	AddButton("!noexpandy,flat", 0, 0, 0, 0, "close");
	SetImage("close", "hui:close");
	AddButton("!noexpandy,flat", 0, 1, 0, 0, "add");
	SetImage("add", "hui:add");
	AddText("!big,bold,angle=90\\Effekte", 0, 2, 0, 0, "");
	SetTarget("group", 0);

	track = NULL;
	Enable("add", false);

	EventM("close", (HuiPanel*)this, (void(HuiPanel::*)())&FxPanel::OnClose);
	EventM("add", (HuiPanel*)this, (void(HuiPanel::*)())&FxPanel::OnAdd);

	Subscribe(audio);
}

FxPanel::~FxPanel()
{
	Unsubscribe(audio);
	Clear();
}

void FxPanel::OnClose()
{
	Hide();
}

void FxPanel::OnAdd()
{
	if (!HuiFileDialogOpen(win, _("einen Effekt w&ahlen"), HuiAppDirectoryStatic + "Plugins/Buffer/", "*.kaba", "*.kaba"))
		return;

	string name = HuiFilename.basename(); // remove directory
	name = name.substr(0, name.num - 5); //      and remove ".kaba"
	Effect *effect = CreateEffect(name);
	if (track)
		track->AddEffect(effect);
	/*else
		audio->AddEffect(effect);*/
}

void FxPanel::Clear()
{
	foreachi(HuiPanel *p, panels, i){
		delete(p);
		RemoveControl("separator_" + i2s(i));
	}
	panels.clear();
	track = NULL;
	Enable("add", false);
}

void FxPanel::SetTrack(Track *t)
{
	Clear();
	track = t;
	Enable("add", t);

	foreachi(Effect *fx, track->fx, i){
		panels.add(new SingleFxPanel(track, fx, i));
		Embed(panels.back(), id_inner, i*2, 0);
		AddSeparator("!vertical", i*2 + 1, 0, 0, 0, "separator_" + i2s(i));
	}
}

void FxPanel::OnUpdate(Observable* o)
{
	foreach(Track *t, audio->track)
		if (t == track){
			SetTrack(t);
			return;
		}
	SetTrack(NULL);
}

