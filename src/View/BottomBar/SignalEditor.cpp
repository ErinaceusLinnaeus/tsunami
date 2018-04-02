/*
 * SignalEditor.cpp
 *
 *  Created on: 30.03.2018
 *      Author: michi
 */

#include "SignalEditor.h"
#include "../AudioView.h"
#include "../../Session.h"
#include "../../Stuff/SignalChain.h"
#include "../../Plugins/PluginManager.h"
#include "../../Plugins/Configurable.h"
#include "../Dialog/ConfigurableSelectorDialog.h"
#include "../../Midi/MidiSource.h"


SignalEditor::Selection::Selection()
{
	type = -1;
	module = NULL;
	port = port_type = -1;
	target_module = NULL;
	target_port = -1;
	dx = dy = 0;
}

SignalEditor::SignalEditor(Session *session) :
	BottomBar::Console(_("Signal Chain"), session)
{
	addDrawingArea("!expandx,expandy,grabfocus", 0, 0, "area");

	menu_chain = hui::CreateResourceMenu("popup_signal_chain_menu");
	menu_module = hui::CreateResourceMenu("popup_signal_module_menu");

	eventXP("area", "hui:draw", std::bind(&SignalEditor::onDraw, this, std::placeholders::_1));
	eventX("area", "hui:mouse-move", std::bind(&SignalEditor::onMouseMove, this));
	eventX("area", "hui:left-button-down", std::bind(&SignalEditor::onLeftButtonDown, this));
	eventX("area", "hui:left-button-up", std::bind(&SignalEditor::onLeftButtonUp, this));
	eventX("area", "hui:right-button-down", std::bind(&SignalEditor::onRightButtonDown, this));
	eventX("area", "hui:key-down", std::bind(&SignalEditor::onKeyDown, this));
	event("signal_chain_add_audio_source", std::bind(&SignalEditor::onAddAudioSource, this));
	event("signal_chain_add_audio_effect", std::bind(&SignalEditor::onAddAudioEffect, this));
	event("signal_chain_add_audio_input", std::bind(&SignalEditor::onAddAudioInputStream, this));
	event("signal_chain_add_audio_joiner", std::bind(&SignalEditor::onAddAudioJoiner, this));
	event("signal_chain_add_midi_source", std::bind(&SignalEditor::onAddMidiSource, this));
	event("signal_chain_add_midi_effect", std::bind(&SignalEditor::onAddMidiEffect, this));
	event("signal_chain_add_midi_input", std::bind(&SignalEditor::onAddMidiInputStream, this));
	event("signal_chain_add_synthesizer", std::bind(&SignalEditor::onAddSynthesizer, this));
	event("signal_chain_add_beat_source", std::bind(&SignalEditor::onAddBeatSource, this));
	event("signal_chain_add_beat_midifier", std::bind(&SignalEditor::onAddBeatMidifier, this));
	event("signal_chain_reset", std::bind(&SignalEditor::onReset, this));
	event("signal_chain_load", std::bind(&SignalEditor::onLoad, this));
	event("signal_chain_save", std::bind(&SignalEditor::onSave, this));
	event("signal_module_delete", std::bind(&SignalEditor::onModuleDelete, this));
	event("signal_module_configure", std::bind(&SignalEditor::onModuleConfigure, this));

	chain = session->signal_chain;
	chain->subscribe(this, std::bind(&SignalEditor::onChainUpdate, this));
}

SignalEditor::~SignalEditor()
{
	chain->unsubscribe(this);
	delete menu_chain;
	delete menu_module;
}

void SignalEditor::onLeftButtonDown()
{
	hover = getHover(hui::GetEvent()->mx, hui::GetEvent()->my);
	sel = hover;
	if (sel.type == sel.TYPE_PORT_IN){
		chain->disconnect_target((SignalChain::Module*)sel.module, sel.port);
	}else if (sel.type == sel.TYPE_PORT_OUT){
		chain->disconnect_source((SignalChain::Module*)sel.module, sel.port);
	}
	redraw("area");
}

void SignalEditor::onLeftButtonUp()
{
	if (sel.type == sel.TYPE_PORT_IN or sel.type == sel.TYPE_PORT_OUT){
		if (hover.target_module){
			if (sel.type == sel.TYPE_PORT_IN){
				chain->disconnect_source((SignalChain::Module*)hover.target_module, hover.target_port);
				chain->connect((SignalChain::Module*)hover.target_module, hover.target_port, (SignalChain::Module*)sel.module, sel.port);
			}else if (sel.type == sel.TYPE_PORT_OUT){
				chain->disconnect_target((SignalChain::Module*)hover.target_module, hover.target_port);
				chain->connect((SignalChain::Module*)sel.module, sel.port, (SignalChain::Module*)hover.target_module, hover.target_port);
			}
		}
		sel = Selection();
	}
	redraw("area");
}

void SignalEditor::onMouseMove()
{
	float mx = hui::GetEvent()->mx;
	float my = hui::GetEvent()->my;
	if (hui::GetEvent()->lbut){
		if (sel.type == sel.TYPE_MODULE){
			auto *m = (SignalChain::Module*)sel.module;
			m->x = mx + sel.dx;
			m->y = my + sel.dy;
		}else if (sel.type == sel.TYPE_PORT_IN or sel.type == sel.TYPE_PORT_OUT){
			hover.target_module = NULL;
			auto h = getHover(mx, my);
			if (h.module != sel.module and h.port_type == sel.port_type){
				if (h.type == sel.TYPE_PORT_IN and sel.type == sel.TYPE_PORT_OUT){
					hover.target_module = h.module;
					hover.target_port = h.port;
				}
				if (h.type == sel.TYPE_PORT_OUT and sel.type == sel.TYPE_PORT_IN){
					hover.target_module = h.module;
					hover.target_port = h.port;
				}
			}
		}
	}else{
		hover = getHover(mx, my);
	}
	redraw("area");
}

const float MODULE_WIDTH = 160;
const float MODULE_HEIGHT = 25;

static rect module_rect(SignalChain::Module *m)
{
	return rect(m->x, m->x + MODULE_WIDTH, m->y, m->y + MODULE_HEIGHT);
}

static float module_port_in_x(SignalChain::Module *m)
{
	return m->x - 5;
}

static float module_port_in_y(SignalChain::Module *m, int index)
{
	return m->y + MODULE_HEIGHT/2 + (index - (float)(m->port_in.num-1)/2)*20;
}

static float module_port_out_x(SignalChain::Module *m)
{
	return m->x + MODULE_WIDTH + 5;
}

static float module_port_out_y(SignalChain::Module *m, int index)
{
	return m->y + MODULE_HEIGHT/2 + (index - (float)(m->port_out.num-1)/2)*20;
}

static color signal_color(int type)
{
	if (type == Track::Type::AUDIO)
		return Red;
	if (type == Track::Type::MIDI)
		return Green;
	if (type == Track::Type::TIME)
		return Blue;
	return White;
}

void SignalEditor::onRightButtonDown()
{
	int mx = hui::GetEvent()->mx;
	int my = hui::GetEvent()->my;
	hover = getHover(mx, my);
	sel = hover;

	if (hover.type == hover.TYPE_MODULE){
		menu_module->openPopup(this, mx, my);
	}else if (hover.type < 0){
		menu_chain->openPopup(this, mx, my);
	}
}

void SignalEditor::onKeyDown()
{
	int key = hui::GetEvent()->key_code;
	if (key == hui::KEY_DELETE){
		if (sel.type == sel.TYPE_MODULE){
			chain->remove((SignalChain::Module*)sel.module);
			hover = sel = Selection();
		}
	}
}

void SignalEditor::onAddAudioSource()
{
	auto *dlg = new ConfigurableSelectorDialog(win, Configurable::Type::AUDIO_SOURCE, session);
	dlg->run();
	if (dlg->_return.num > 0)
		chain->addAudioSource(dlg->_return);
	delete(dlg);
}

void SignalEditor::onAddAudioEffect()
{
	auto *dlg = new ConfigurableSelectorDialog(win, Configurable::Type::AUDIO_EFFECT, session);
	dlg->run();
	if (dlg->_return.num > 0)
		chain->addAudioEffect(dlg->_return);
	delete(dlg);
}

void SignalEditor::onAddAudioJoiner()
{
	chain->addAudioJoiner();
}

void SignalEditor::onAddAudioInputStream()
{
	chain->addAudioInputStream();
}

void SignalEditor::onAddMidiSource()
{
	auto *dlg = new ConfigurableSelectorDialog(win, Configurable::Type::MIDI_SOURCE, session);
	dlg->run();
	if (dlg->_return.num > 0)
		chain->addMidiSource(dlg->_return);
	delete(dlg);
}

void SignalEditor::onAddMidiEffect()
{
	auto *dlg = new ConfigurableSelectorDialog(win, Configurable::Type::MIDI_EFFECT, session);
	dlg->run();
	if (dlg->_return.num > 0)
		chain->addMidiEffect(dlg->_return);
	delete(dlg);
}

void SignalEditor::onAddSynthesizer()
{
	auto *dlg = new ConfigurableSelectorDialog(win, Configurable::Type::SYNTHESIZER, session);
	dlg->run();
	if (dlg->_return.num > 0)
		chain->addSynthesizer(dlg->_return);
	delete(dlg);
}

void SignalEditor::onAddMidiInputStream()
{
	chain->addMidiInputStream();
}

void SignalEditor::onAddBeatSource()
{
	auto *dlg = new ConfigurableSelectorDialog(win, Configurable::Type::BEAT_SOURCE, session);
	dlg->run();
	if (dlg->_return.num > 0)
		chain->addBeatSource(dlg->_return);
	delete(dlg);
}

void SignalEditor::onAddBeatMidifier()
{
	chain->addBeatMidifier();
}

void SignalEditor::onModuleDelete()
{
	if (sel.type == sel.TYPE_MODULE){
		chain->remove((SignalChain::Module*)sel.module);
		hover = sel = Selection();
	}
}

void SignalEditor::onModuleConfigure()
{
}

void SignalEditor::onReset()
{
	chain->reset();
}

void SignalEditor::onLoad()
{
	if (hui::FileDialogOpen(win, "", "", "*", "*"))
		chain->load(hui::Filename);
}

void SignalEditor::onSave()
{
	if (hui::FileDialogSave(win, "", "", "*", "*"))
		chain->save(hui::Filename);
}

SignalEditor::Selection SignalEditor::getHover(float mx, float my)
{
	Selection s;
	for (auto *m: chain->modules){
		rect r = module_rect(m);
		if (r.inside(mx, my)){
			s.type = Selection::TYPE_MODULE;
			s.module = m;
			s.dx = m->x - mx;
			s.dy = m->y - my;
			return s;
		}
		for (int i=0; i<m->port_in.num; i++){
			float y = module_port_in_y(m, i);
			float x = module_port_in_x(m);
			if (abs(x - mx) < 10 and abs(y - my) < 10){
				s.type = Selection::TYPE_PORT_IN;
				s.module = m;
				s.port = i;
				s.port_type = m->port_in[i];
				s.dx = x;
				s.dy = y;
				return s;
			}
		}
		for (int i=0; i<m->port_out.num; i++){
			float y = module_port_out_y(m, i);
			float x = module_port_out_x(m);
			if (abs(x - mx) < 10 and abs(y - my) < 10){
				s.type = Selection::TYPE_PORT_OUT;
				s.module = m;
				s.port = i;
				s.port_type = m->port_out[i];
				s.dx = x;
				s.dy = y;
				return s;
			}
		}
	}
	return s;
}


void SignalEditor::onDraw(Painter* p)
{
	int w = p->width;
	int h = p->height;
	p->setColor(view->colors.background);
	p->drawRect(0, 0, w, h);
	p->setFontSize(12);

	for (auto *m: chain->modules){
		p->setColor(view->colors.background_track_selected);
		if (hover.type == Selection::TYPE_MODULE and hover.module == m)
			p->setColor(ColorInterpolate(view->colors.background_track_selected, view->colors.hover, 0.25f));
		p->setRoundness(view->CORNER_RADIUS);
		p->drawRect(module_rect(m));
		p->setRoundness(0);
		if (sel.type == sel.TYPE_MODULE and sel.module == m){
			p->setColor(view->colors.text);
			p->setFont("", 12, true, false);
		}else{
			p->setColor(view->colors.text_soft1);
		}
		float ww = p->getStrWidth(m->type());
		p->drawStr(m->x + MODULE_WIDTH/2 - ww/2, m->y + 4, m->type());
		p->setFont("", 12, false, false);

		foreachi(int t, m->port_in, i){
			p->setColor(signal_color(t));
			float r = 4;
			if (hover.type == Selection::TYPE_PORT_IN and hover.module == m and hover.port == i)
				r = 8;
			p->drawCircle(module_port_in_x(m), module_port_in_y(m, i), r);
		}
		foreachi(int t, m->port_out, i){
			p->setColor(signal_color(t));
			float r = 4;
			if (hover.type == Selection::TYPE_PORT_OUT and hover.module == m and hover.port == i)
				r = 8;
			p->drawCircle(module_port_out_x(m), module_port_out_y(m, i), r);
		}
	}

	for (auto *c: chain->cables){
		float x0 = module_port_out_x(c->source);
		float y0 = module_port_out_y(c->source, c->source_port);
		float x1 = module_port_in_x(c->target);
		float y1 = module_port_in_y(c->target, c->target_port);
		p->setColor(signal_color(c->type));
		p->drawLine(x0, y0, x1, y1);
	}

	if (sel.type == sel.TYPE_PORT_IN or sel.type == sel.TYPE_PORT_OUT){
		p->setColor(White);
		if (hover.target_module){
			p->setLineWidth(5);
			SignalChain::Module *t = (SignalChain::Module*)hover.target_module;
			if (hover.type == hover.TYPE_PORT_IN)
				p->drawLine(sel.dx, sel.dy, module_port_out_x(t), module_port_out_y(t, hover.target_port));
			else
				p->drawLine(sel.dx, sel.dy, module_port_in_x(t), module_port_in_y(t, hover.target_port));
			p->setLineWidth(1);
		}else
			p->drawLine(sel.dx, sel.dy, hui::GetEvent()->mx, hui::GetEvent()->my);
	}
}

void SignalEditor::onChainUpdate()
{
	redraw("area");
}
