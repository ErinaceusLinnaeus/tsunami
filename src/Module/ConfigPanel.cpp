/*
 * ConfigPanel.cpp
 *
 *  Created on: 23.09.2014
 *      Author: michi
 */

#include "ConfigPanel.h"
#include "Module.h"
#include "../View/Helper/Progress.h"
#include "../Plugins/PluginManager.h"
#include "../Session.h"

extern const int CONFIG_PANEL_WIDTH;
extern const int CONFIG_PANEL_HEIGHT;


ConfigPanel::ConfigPanel(Module *_c) {
	ignore_change = false;
	c = _c;
	if (c) {
		c->subscribe(this, [=]{ if (!ignore_change) update(); }, c->MESSAGE_CHANGE);
		c->subscribe(this, [=]{ if (!ignore_change) update(); }, c->MESSAGE_CHANGE_BY_ACTION);
	}
}

ConfigPanel::~ConfigPanel() {
	if (c)
		c->unsubscribe(this);
}


void ConfigPanel::__init__(Module *_c) {
	new(this) ConfigPanel(_c);
}

void ConfigPanel::__delete__() {
	this->ConfigPanel::~ConfigPanel();
}

void ConfigPanel::changed() {
	ignore_change = true;
	c->changed();
	ignore_change = false;
}




class ConfigurationDialog : public hui::Window {
public:
	ConfigurationDialog(Module *c, ModuleConfiguration *pd, ConfigPanel *p, hui::Window *parent) :
		hui::Window("configurable_dialog", parent)
	{
		config = c;
		panel = p;
		progress = nullptr;
		ok = false;

		set_title(config->module_subtype);
		embed(panel, "content", 0, 0);
		panel->set_options("content", format("width=%d,height=%d", int(CONFIG_PANEL_WIDTH*1.3f), CONFIG_PANEL_HEIGHT)); // doesn't seem to work
		set_size(CONFIG_PANEL_WIDTH*1.1f, CONFIG_PANEL_HEIGHT*1.2f);

		if (c->module_type != ModuleType::AUDIO_EFFECT)
			hide_control("preview", true);

		event("load_favorite", [=]{ on_load(); });
		event("save_favorite", [=]{ on_save(); });
		event("ok", [=]{ on_ok(); });
		event("preview", [=]{ on_preview(); });
		event("cancel", [=]{ on_close(); });
		event("hui:close", [=]{ on_close(); });
	}
	void on_ok() {
		ok = true;
		destroy();
	}
	void on_close() {
		destroy();
	}
	void on_preview() {
		preview_start();
	}
	void on_load() {
		string name = config->session->plugin_manager->select_favorite_name(this, config, false);
		if (name.num == 0)
			return;
		config->session->plugin_manager->apply_favorite(config, name);
	}
	void on_save() {
		string name = config->session->plugin_manager->select_favorite_name(this, config, true);
		if (name.num == 0)
			return;
		config->session->plugin_manager->save_favorite(config, name);
	}

	void on_config_change() {
		panel->update();
	}

	void preview_start() {
		/*if (progress)
			previewEnd();
		config->configToString();
		tsunami->win->view->renderer->preview_effect = (Effect*)config;


		progress = new ProgressCancelable(_("Preview"), win);
		progress->subscribe(this, [=]{ onProgressCancel(); }, progress->MESSAGE_CANCEL);

		tsunami->win->view->stream->subscribe(this, [=]{ onUpdateStream(); });
		tsunami->win->view->renderer->prepare(tsunami->win->view->sel.range, false);
		tsunami->win->view->stream->play();*/
	}

	void on_progress_cancel() {
		preview_end();
	}

	void on_update_stream() {
		/*if (progress){
			int pos = tsunami->win->view->stream->getPos(0); // TODO
			Range r = tsunami->win->view->sel.range;
			progress->set(_("Preview"), (float)(pos - r.offset) / r.length);
			if (!tsunami->win->view->stream->isPlaying())
				previewEnd();
		}*/
	}

	void preview_end() {
		/*if (!progress)
			return;
		tsunami->win->view->stream->unsubscribe(this);
		progress->unsubscribe(this);
		tsunami->win->view->stream->stop();
		delete(progress);
		progress = NULL;


		tsunami->win->view->renderer->preview_effect = NULL;*/
	}

	Module *config;
	ConfigPanel *panel;
	Progress *progress;
	bool ok;
};



bool configure_module(hui::Window *win, Module *m) {
	auto *config = m->get_config();
	if (!config)
		return true;

	//_auto_panel_ = NULL;
	auto *panel = m->create_panel();
	if (!panel)
		return true;
	panel->set_large(true);
	panel->update();
	auto *dlg = new ConfigurationDialog(m, config, panel, win);
	dlg->run();
	bool ok = dlg->ok;
	delete dlg;
	return ok;
}

