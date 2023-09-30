/*
 * PluginConsole.cpp
 *
 *  Created on: 23.08.2018
 *      Author: michi
 */

#include "PluginConsole.h"
#include "../module/ModulePanel.h"
#include "../dialog/ModuleSelectorDialog.h"
#include "../../Session.h"
#include "../../plugins/TsunamiPlugin.h"
#include "../../plugins/PluginManager.h"
#include "../../lib/base/algo.h"
#include "../../lib/hui/Controls/Control.h"


PluginConsole::PluginConsole(Session *s, BottomBar *bar) :
	BottomBar::Console(_("Plugins"), "plugin-console", s, bar)
{
	from_resource("plugin-console");
	next_x = 0;

	event("add", [this] { on_add_button(); });

	session->out_add_plugin >> create_sink([this] { on_add_plugin(); });
	session->out_remove_plugin >> create_sink([this] { on_remove_plugin(); });
}

PluginConsole::~PluginConsole() {
	session->unsubscribe(this);

	// necessary?
	for (auto *p: weak(panels))
		unembed(p);
}

void PluginConsole::on_enter() {
	update_favotites();
}

void PluginConsole::on_add_button() {
	ModuleSelectorDialog::choose(this, session, ModuleCategory::TSUNAMI_PLUGIN).on([this] (const string &name) {
		session->execute_tsunami_plugin(name);
		
		// TODO: have PluginManager send notifications...?
		update_favotites();
	});
}

void PluginConsole::on_add_plugin() {
	auto *plugin = session->last_plugin;
	auto *p = new ModulePanel(plugin, this, ConfigPanelMode::FIXED_WIDTH | ConfigPanelMode::DELETE | ConfigPanelMode::PROFILES);
	p->set_options(p->root_control->id, "class=card");
	p->set_func_delete([this, plugin] {
		plugin->stop_request();
	});
	embed(p, "panel-grid", next_x ++, 0);
	panels.add(p);
	hide_control("no-plugins-label", true);
	hide_control("scroller", false);
	blink();
}

void PluginConsole::on_remove_plugin() {
	int index = base::find_index_if(weak(panels), [this] (ModulePanel* p) {
		return p->socket.module == session->last_plugin;
	});
	if (index >= 0) {
		unembed(weak(panels)[index]);
		panels.erase(index);
	}
	hide_control("no-plugins-label", panels.num > 0);
	hide_control("scroller", panels.num == 0);
}

void PluginConsole::update_favotites() {
	// remove old ones
	for (auto&& [id, h]: favorite_buttons) {
		remove_event_handler(h);
		remove_control(id);
	}
	favorite_buttons.clear();

	// add new
	auto list = session->plugin_manager->find_module_sub_types(ModuleCategory::TSUNAMI_PLUGIN);
	set_target("favorite-grid");
	int index = 0;
	for (auto &p: list)
		if (session->plugin_manager->is_favorite(session, ModuleCategory::TSUNAMI_PLUGIN, p)) {
			string name = p;
			string id = format("favorite-button-%d", index);
			add_button("!flat,height=40,big\\" + p.head(1), 0, index+2, id);
			set_tooltip(id, format("Favorite: %s", name));
			int h = event(id, [this,name] { session->execute_tsunami_plugin(name); });
			favorite_buttons.add({id, h});
			index ++;
		}
	hide_control("favorite-grid", favorite_buttons.num == 0);
}
