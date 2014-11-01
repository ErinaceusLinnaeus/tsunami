/*
 * BottomBar.cpp
 *
 *  Created on: 23.03.2014
 *      Author: michi
 */

#include "BottomBar.h"
#include "MiniConsole.h"
#include "MixingConsole.h"
#include "CurveConsole.h"
#include "FxConsole.h"
#include "SynthConsole.h"
#include "LevelConsole.h"
#include "LogDialog.h"
#include "SampleManager.h"
#include "MidiEditor.h"
#include "../../lib/hui/Controls/HuiControl.h"
#include "../AudioView.h"

BottomBar::BottomBar(AudioView *view, AudioFile *audio, AudioOutput *output, Log *log) :
	Observable("BottomBar")
{
	ready = false;
	console_when_ready = MIXING_CONSOLE;

	addControlTable("!noexpandy,height=300,expandx", 0, 0, 1, 2, "root_grid0");
	setTarget("root_grid0", 0);
	addSeparator("!horizontal,expandx", 0, 0, 0, 0, "");
	addControlTable("!expandx", 0, 1, 3, 1, "root_grid");
	setTarget("root_grid", 0);
	addControlTable("!noexpandx,width=130", 0, 0, 1, 2, "button_grid");
	addSeparator("!vertical", 1, 0, 0, 0, "");
	addControlTable("", 2, 0, 1, 20, "console_grid");
	setTarget("button_grid", 0);
	addButton("!noexpandy,flat", 0, 0, 0, 0, "close");
	setImage("close", "hui:close");
	addListView("!nobar\\name", 0, 1, 0, 0, "choose");
	fx_console = new FxConsole(view, audio);
	synth_console = new SynthConsole(view, audio);
	mixing_console = new MixingConsole(audio, output, view->stream);
	level_console = new LevelConsole(view, audio);
	curve_console = new CurveConsole(view, audio);
	sample_manager = new SampleManager(audio);
	log_dialog = new LogDialog(log);
	midi_editor = new MidiEditor(view, audio);
	embed(mixing_console, "console_grid", 0, 0);
	embed(fx_console, "console_grid", 0, 1);
	embed(synth_console, "console_grid", 0, 2);
	embed(midi_editor, "console_grid", 0, 3);
	embed(level_console, "console_grid", 0, 4);
	embed(sample_manager, "console_grid", 0, 5);
	embed(curve_console, "console_grid", 0, 6);
	embed(log_dialog, "console_grid", 0, 7);

	view->subscribe(this);

	//menu = new HuiMenu;
	foreachi(HuiPanel *p, children, i){
		addString("choose", ((BottomBarConsole*)p)->title);
		//string id = "bottom_bar_choose_" + i2s(i);
		//menu->AddItemCheckable(((BottomBarConsole*)p)->title, id);
		//EventM(id, (HuiPanel*)this, (void(HuiPanel::*)())&BottomBar::OnChooseByMenu);
	}

	eventX("choose", "hui:select", (HuiPanel*)this, (void(HuiPanel::*)())&BottomBar::onChoose);
	event("close", (HuiPanel*)this, (void(HuiPanel::*)())&BottomBar::onClose);

	visible = true;
	ready = true;
	choose(console_when_ready);
}

BottomBar::~BottomBar()
{
}

void BottomBar::onClose()
{
	hide();
}

/*void BottomBar::OnOpenChooseMenu()
{
	foreachi(HuiControl *c, menu->item, i)
		c->Check(i == active_console);
	menu->OpenPopup(this, 0, 0);
}

void BottomBar::OnChooseByMenu()
{
	Choose(HuiGetEvent()->id.tail(1)._int());
}*/

void BottomBar::onChoose()
{
	int n = getInt("");
	if (n >= 0)
		choose(n);
}

void BottomBar::onShow()
{
	visible = true;
	notify();
}

void BottomBar::onHide()
{
	visible = false;
	notify();
}

void BottomBar::choose(int console)
{
	if (!ready){
		console_when_ready = console;
		return;
	}

	foreachi(HuiPanel *p, children, i){
		if (i == console){
			setString("title", "!big\\" + ((BottomBarConsole*)p)->title);
			p->show();
		}else
			p->hide();
	}
	setInt("choose", console);
	active_console = console;
	if (!visible)
		show();
	notify();
}

bool BottomBar::isActive(int console)
{
	return (active_console == console) && visible;
}

