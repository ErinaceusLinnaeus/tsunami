/*
 * HuiPanel.h
 *
 *  Created on: 18.03.2014
 *      Author: michi
 */

#ifndef HUIPANEL_H_
#define HUIPANEL_H_

#include "hui.h"
#include "../base/pointer.h"


class Painter;
class rect;

namespace hui
{

class Menu;
class Resource;
class Painter;
class Event;
class EventListener;
class EventKeyCode;
class Control;
class ControlRadioButton;

class Panel : public Sharable<EventHandler> {
	friend class Control;
	friend class ControlRadioButton;
	friend class Menu;
public:
	Panel();
	virtual ~Panel();
	void _cdecl __init__();
	virtual void _cdecl __delete__();
	void _ClearPanel_();

	void _cdecl activate(const string &control_id);
	bool _cdecl is_active(const string &control_id);
	void _cdecl from_resource(const string &id);
	void set_from_resource(Resource *res);
	void _cdecl from_source(const string &source);

	void _cdecl show();
	void _cdecl hide();

	virtual void _cdecl on_show(){}
	virtual void _cdecl on_hide(){}

	void set_win(Window *win);

	// events
	int _cdecl event(const string &id, const Callback &function);
	int _cdecl event_x(const string &id, const string &msg, const Callback &function);
	int _cdecl event_xp(const string &id, const string &msg, const CallbackP &function);
	void _cdecl remove_event_handler(int event_handler_id);
	bool _send_event_(Event *e, bool force_if_not_allowed = false);

	// creating controls

	void _cdecl add_control(const string &type, const string &title, int x, int y, const string &id);
	void _cdecl _add_control(const string &ns, Resource &cmd, const string &parent_id);
	void _cdecl add_button(const string &title, int x, int y,const string &id);
	void _cdecl add_def_button(const string &title, int x, int y, const string &id);
	void _cdecl add_color_button(const string &title, int x, int y, const string &id);
	void _cdecl add_toggle_button(const string &title, int x, int y, const string &id);
	void _cdecl add_check_box(const string &title, int x, int y, const string &id);
	void _cdecl add_radio_button(const string &title, int x, int y, const string &id);
	void _cdecl add_label(const string &title, int x, int y, const string &id);
	void _cdecl add_edit(const string &title, int x, int y, const string &id);
	void _cdecl add_multiline_edit(const string &title, int x, int y, const string &id);
	void _cdecl add_group(const string &title, int x, int y, const string &id);
	void _cdecl add_combo_box(const string &title, int x, int y, const string &id);
	void _cdecl add_tab_control(const string &title, int x, int y, const string &id);
	void _cdecl set_target(const string &id);
	void _cdecl add_list_view(const string &title, int x, int y, const string &id);
	void _cdecl add_tree_view(const string &title, int x, int y, const string &id);
	void _cdecl add_icon_view(const string &title, int x, int y, const string &id);
	void _cdecl add_list_view__test(const string &title, int x, int y, const string &id);
	void _cdecl add_progress_bar(const string &title, int x, int y, const string &id);
	void _cdecl add_slider(const string &title, int x, int y, const string &id);
	void _cdecl add_image(const string &title, int x, int y, const string &id);
	void _cdecl add_drawing_area(const string &title, int x, int y, const string &id);
	void _cdecl add_grid(const string &title, int x, int y, const string &id);
	void _cdecl add_spin_button(const string &title, int x, int y, const string &id);
	void _cdecl add_scroller(const string &title, int x, int y, const string &id);
	void _cdecl add_expander(const string &title, int x, int y, const string &id);
	void _cdecl add_separator(const string &title, int x, int y, const string &id);
	void _cdecl add_paned(const string &title, int x, int y, const string &id);
	void _cdecl add_revealer(const string &title, int x, int y, const string &id);
	void _cdecl add_menu_button(const string &title, int x, int y, const string &id);

	void _cdecl embed_dialog(const string &id, int x, int y);
	void _cdecl embed_source(const string &source, const string &parent_id, int x, int y);
	void embed_resource(Resource &c, const string &parent_id, int x, int y);
	void _embed_resource(const string &ns, Resource &c, const string &parent_id, int x, int y);
	void _cdecl embed(hui::Panel *panel, const string &parent_id, int x, int y);

// using controls
	// string
	void _cdecl set_string(const string &id, const string &str);
	void _cdecl add_string(const string &id, const string &str);
	void _cdecl add_child_string(const string &id, int parent_row, const string &str);
	void _cdecl change_string(const string &id, int row, const string &str);
	void _cdecl remove_string(const string &id, int row);
	string _cdecl get_string(const string &id);
	string _cdecl get_cell(const string &id, int row, int column);
	void _cdecl set_cell(const string &id, int row, int column, const string &str);
	// int
	void _cdecl set_int(const string &id, int i);
	int _cdecl get_int(const string &id);
	// float
	void _cdecl set_decimals(int decimals);
	void _cdecl set_float(const string &id, float f);
	float _cdecl get_float(const string &id);
	// color
	void _cdecl set_color(const string &id, const color &col);
	color _cdecl get_color(const string &id);
	// tree
	void _cdecl expand_all(const string &id, bool expand);
	void _cdecl expand(const string &id, int row, bool expand);
	bool _cdecl is_expanded(const string &id, int row);
	// revealer
	void _cdecl reveal(const string &id, bool reveal);
	bool _cdecl is_revealed(const string &id);
	// stuff
	void _cdecl enable(const string &id, bool enabled);
	bool _cdecl is_enabled(const string &id);
	void _cdecl hide_control(const string &id, bool hide);
	void _cdecl delete_control(const string &id);
	void _cdecl check(const string &id, bool checked);
	bool _cdecl is_checked(const string &id);
	void _cdecl set_image(const string &id, const string &image);
	void _cdecl set_tooltip(const string &id, const string &tip);
	Array<int> _cdecl get_selection(const string &id);
	void _cdecl set_selection(const string &id, const Array<int> &sel);
	void _cdecl reset(const string &id);
	void _cdecl remove_control(const string &id);
	void _cdecl set_options(const string &id, const string &options);

	// drawing
	void _cdecl redraw(const string &id);
	void _cdecl redraw_rect(const string &_id, const rect &r);
	Control *_get_control_(const string &id);
#ifdef HUI_API_GTK
	Control *_get_control_by_widget_(GtkWidget *widget);
	string _get_id_by_widget_(GtkWidget *widget);
#endif
	string _get_cur_id_();
	void _set_cur_id_(const string &id);
	void _cdecl set_border_width(int width);
	void _cdecl set_spacing(int width);


	void _connect_menu_to_panel(Menu *menu);

protected:


#ifdef HUI_API_WIN
#endif
#ifdef HUI_API_GTK
public:
	GtkWidget *plugable;
	struct SizeGroup {
		GtkSizeGroup *group;
		int mode;
		string name;
	};
	Array<SizeGroup> size_groups;
protected:
	void _insert_control_(Control *c, int x, int y);
	int desired_width, desired_height;
#endif

	Control *cur_control;
	Control *root_control;
	void apply_foreach(const string &id, std::function<void(Control*)> f);
public:
	Array<EventListener> event_listeners;
	int current_event_listener_uid;
protected:

	string id;
	int unique_id;
	string cur_id;
public:
	int _get_unique_id_();
	int num_float_decimals;
	int border_width;
	int spacing;
	Window *win;
	Panel *parent;
	Array<Panel*> children;


#if GTK_CHECK_VERSION(4,0,0)
	static void _on_menu_action_(GSimpleAction *simple, GVariant *parameter, gpointer user_data);
	GSimpleActionGroup *action_group = nullptr;
	void _try_add_action_(const string &id, bool checkable);
public:
	GAction *_get_action(const string &id, bool with_scope);
#endif
};

};

#endif /* HUIPANEL_H_ */
