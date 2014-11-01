/*
 * HuiPanel.cpp
 *
 *  Created on: 18.03.2014
 *      Author: michi
 */

#include "hui.h"
#include "hui_internal.h"
#include "Controls/HuiControl.h"

// for unique window identifiers
static int current_uid = 0;

HuiPanel::HuiPanel()
{
	win = NULL;
	parent = NULL;
	border_width = 5;
	expander_indent = 20;
	id = "";
	num_float_decimals = 3;
	tab_creation_page = -1;
	root_control = NULL;
	is_resizable = true;
	plugable = NULL;

	unique_id = current_uid ++;

	setTarget("", 0);
}

HuiPanel::~HuiPanel()
{
	_ClearPanel_();
}

void HuiPanel::__init__()
{
	new(this) HuiPanel;
}

void HuiPanel::__delete__()
{
	_ClearPanel_();
}

// might be executed repeatedly
void HuiPanel::_ClearPanel_()
{
	HuiClosedPanel c;
	c.unique_id = unique_id;
	c.panel = this;
	c.last_id = cur_id;
	HuiClosedPanels.add(c);

	if (parent){
		// disconnect
		for (int i=0; i<parent->children.num; i++)
			if (parent->children[i] == this)
				parent->children.erase(i);
		parent = NULL;
	}
	while (children.num > 0){
		HuiPanel *p = children[0];
		children.erase(0);
		delete(p);
	}

	while (control.num > 0){
		HuiControl *c = control[0];
		control.erase(0);
		delete(c);
	}
	id.clear();
	cur_id.clear();
	events.clear();
}

void HuiPanel::setBorderWidth(int width)
{
	border_width = width;
}

void HuiPanel::setIndent(int indent)
{
	expander_indent = indent;
}

void HuiPanel::setDecimals(int decimals)
{
	num_float_decimals = decimals;
}

string HuiPanel::_get_cur_id_()
{
	return cur_id;
}

void HuiPanel::_set_cur_id_(const string &id)
{
	if (win)
		win->cur_id = id;
	cur_id = id;
}

void HuiPanel::eventS(const string &id, hui_callback *function)
{
	events.add(HuiEventListener(id, "*", function));

}

void HuiPanel::eventSX(const string &id, const string &msg, hui_callback *function)
{
	events.add(HuiEventListener(id, msg, function));
}

void HuiPanel::_event(const string &id, HuiEventHandler *handler, void (HuiEventHandler::*function)())
{
	events.add(HuiEventListener(id, ":def:", HuiCallback(handler, function)));
}

void HuiPanel::_eventX(const string &id, const string &msg, HuiEventHandler *handler, void (HuiEventHandler::*function)())
{
	events.add(HuiEventListener(id, msg, HuiCallback(handler, function)));
}

void HuiPanel::_eventK(const string &id, hui_kaba_callback *function)
{
	events.add(HuiEventListener(id, ":def:", HuiCallback(this, function)));
}

void HuiPanel::_eventKO(const string &id, HuiEventHandler* handler, hui_kaba_callback *function)
{
	events.add(HuiEventListener(id, ":def:", HuiCallback(handler, function)));
}

void HuiPanel::_eventKX(const string &id, const string &msg, hui_kaba_callback *function)
{
	events.add(HuiEventListener(id, msg, HuiCallback(this, function)));
}

void HuiPanel::_eventKOX(const string &id, const string &msg, HuiEventHandler* handler, hui_kaba_callback *function)
{
	events.add(HuiEventListener(id, msg, HuiCallback(handler, function)));
}

void HuiPanel::removeEventHandlers(HuiEventHandler *handler)
{
	for (int i=events.num-1;i>=0;i--)
		if (events[i].function.has_handler(handler))
			events.erase(i);
}

bool HuiPanel::_send_event_(HuiEvent *e)
{
	if (!win)
		return false;
	if (!win->allow_input)
		return false;
	msg_db_f("SendEvent", 2);
	//msg_write(e->id);
	//msg_write(e->message);
	HuiCurWindow = win;
	e->win = win;
	e->mx = win->input.x;
	e->my = win->input.y;
	e->dx = win->input.dx;
	e->dy = win->input.dy;
	e->dz = win->input.dz;
	e->lbut = win->input.lb;
	e->mbut = win->input.mb;
	e->rbut = win->input.rb;
	e->key_code = win->input.key_code;
	e->key = (e->key_code % 256);
	e->text = HuiGetKeyChar(e->key_code);
	e->row = win->input.row;
	e->column = win->input.column;
	_HuiEvent_ = *e;
	if (e->id.num > 0)
		_set_cur_id_(e->id);
	else
		_set_cur_id_(e->message);

	bool sent = false;
	foreach(HuiEventListener &ee, events){
		if (!_HuiEventMatch_(e, ee.id, ee.message))
			continue;

		// send the event
		if (ee.function.is_set()){
			ee.function.call();
			sent = true;
		}

		// window closed by callback?
		foreach(HuiClosedPanel &cp, HuiClosedPanels)
			if (cp.panel == this)
				return sent;
		_foreach_it_.update();
	}

	// reset
	win->input.dx = 0;
	win->input.dy = 0;
	win->input.dz = 0;

	return sent;
}

int HuiPanel::_get_unique_id_()
{
	return unique_id;
}

void HuiPanel::show()
{
	if (this == win)
		win->show();
	else if (root_control)
		root_control->Hide(false);
	onShow();
}

void HuiPanel::hide()
{
	if (this == win)
		win->hide();
	else if (root_control)
		root_control->Hide(true);
	onHide();
}

//----------------------------------------------------------------------------------
// easy window creation functions


void HuiPanel::addControl(const string &type, const string &title, int x, int y, int width, int height, const string &id)
{
	//msg_db_m(format("HuiPanelAddControl %s  %s  %d  %d  %d  %d  %d", type.c_str(), title.c_str(), x, y, width, height, id.c_str()).c_str(),2);
	if (type == "Button")
		addButton(title, x, y, width, height, id);
	else if (type == "ColorButton")
		addColorButton(title, x, y, width, height, id);
	else if (type == "DefButton")
		addDefButton(title, x, y, width, height, id);
	else if (type == "Text")
		addText(title, x, y, width, height, id);
	else if (type == "Edit")
		addEdit(title, x, y, width, height, id);
	else if (type == "MultilineEdit")
		addMultilineEdit(title, x, y, width, height, id);
	else if (type == "Group")
		addGroup(title, x, y, width, height, id);
	else if (type == "CheckBox")
		addCheckBox(title, x, y, width, height, id);
	else if (type == "ComboBox")
		addComboBox(title, x, y, width, height, id);
	else if (type == "TabControl")
		addTabControl(title, x, y, width, height, id);
	else if (type == "ListView")
		addListView(title, x, y, width, height, id);
	else if (type == "TreeView")
		addTreeView(title, x, y, width, height, id);
	else if (type == "IconView")
		addIconView(title, x, y, width, height, id);
	else if (type == "ProgressBar")
		addProgressBar(title, x, y, width, height, id);
	else if (type == "Slider")
		addSlider(title, x, y, width, height, id);
	else if (type == "Image")
		addImage(title, x, y, width, height, id);
	else if (type == "DrawingArea")
		addDrawingArea(title, x, y, width, height, id);
	else if ((type == "ControlTable") || (type == "Grid"))
		addControlTable(title, x, y, width, height, id);
	else if (type == "SpinButton")
		addSpinButton(title, x, y, width, height, id);
	else if (type == "RadioButton")
		addRadioButton(title, x, y, width, height, id);
	else if (type == "ToggleButton")
		addToggleButton(title, x, y, width, height, id);
	else if (type == "Expander")
		addExpander(title, x, y, width, height, id);
	else if (type == "Scroller")
		addScroller(title, x, y, width, height, id);
	else if (type == "Paned")
		addPaned(title, x, y, width, height, id);
	else if (type == "Separator")
		addSeparator(title, x, y, width, height, id);
}

void HuiPanel::fromResource(const string &id)
{
	msg_db_f("Window.FromResource",1);
	HuiResource *res = HuiGetResource(id);
	if (!res)
		return;

	// title
	if (win)
		win->setTitle(HuiGetLanguage(res->id));

	// size
	if (win)
		win->setSize(res->i_param[0], res->i_param[1]);


	// dialog
	/*CHuiPanel *dlg
	if (res->type == "SizableDialog")
		dlg = HuiCreateSizableDialog(HuiGetLanguage(res->id), res->i_param[0], res->i_param[1], root, res->b_param[0]);
	else
		dlg = HuiCreateDialog(HuiGetLanguage(res->id), res->i_param[0], res->i_param[1], root, res->b_param[0]);*/

	// menu?
	if ((win) && (res->s_param[0].num > 0))
		win->setMenu(HuiCreateResourceMenu(res->s_param[0]));

	// toolbar?
	if ((win) && (res->s_param[1].num > 0))
		win->toolbar[HuiToolbarTop]->setByID(res->s_param[1]);

	// controls
	foreach(HuiResource &cmd, res->children){
		//msg_db_m(format("%d:  %d / %d",j,(cmd->type & 1023),(cmd->type >> 10)).c_str(),4);
		if (res->type == "Dialog"){
			setTarget(cmd.s_param[0], cmd.i_param[4]);
			addControl( cmd.type, HuiGetLanguage(cmd.id),
						cmd.i_param[0], cmd.i_param[1],
						cmd.i_param[2], cmd.i_param[3],
						cmd.id);
		}else if (res->type == "SizableDialog"){
			//msg_write("insert " + cmd.id + " (" + cmd.type + ") into " + cmd.s_param[0]);
			setTarget(cmd.s_param[0], cmd.i_param[4]);
			addControl( cmd.type, HuiGetLanguage(cmd.id),
						cmd.i_param[0], cmd.i_param[1],
						cmd.i_param[2], cmd.i_param[3],
						cmd.id);
		}
		enable(cmd.id, cmd.enabled);
		if (cmd.image.num > 0)
			setImage(cmd.id, cmd.image);
	}
	msg_db_m("  \\(^_^)/",1);
}

void HuiPanel::fromSource(const string &buffer)
{
	msg_db_f("FromSource",1);
	HuiResourceNew res;
	res.load(buffer);
	if (res.type == "Dialog"){
		if (win)
			win->setSize(res.w, res.h);

		if (res.children.num > 0)
			embedResource(res.children[0], "", 0, 0);
	}else{
		embedResource(res, "", 0, 0);
	}

}


void HuiPanel::embedResource(HuiResourceNew &c, const string &parent_id, int x, int y)
{
	setTarget(parent_id, x);
	string title = c.title;
	if (c.options.num > 0)
		title = "!" + implode(c.options, ",") + "\\" + title;
	addControl(c.type, title, x, y, c.w, c.h, c.id);

	enable(c.id, c.enabled);
	if (c.image.num > 0)
		setImage(c.id, c.image);

	foreach(HuiResourceNew &child, c.children)
		embedResource(child, c.id, child.x, child.y);
}

void HuiPanel::embedSource(const string &buffer, const string &parent_id, int x, int y)
{
	HuiResourceNew res;
	res.load(buffer);
	embedResource(res, parent_id, x, y);
}

void HuiPanel::embed(HuiPanel *panel, const string &parent_id, int x, int y)
{
	if (!panel->root_control){
		msg_error("trying to embed an empty panel");
		return;
	}
	panel->parent = this;
	panel->set_win(win);
	children.add(panel);

	setTarget(parent_id, x);
	_insert_control_(panel->root_control, x, y, 0, 0);
	control.pop(); // dont' really add to us
	panel->root_control->panel = panel;
}

void HuiPanel::set_win(HuiWindow *_win)
{
	win = _win;
	foreach(HuiPanel *p, children)
		p->set_win(win);
}


//----------------------------------------------------------------------------------
// data exchanging functions for control items


#define test_controls(_id, c)	\
	string tid = (_id.num == 0) ? cur_id : _id; \
	foreach(HuiControl *c, control) \
		if (c->id == tid)

// replace all the text
//    for all
void HuiPanel::setString(const string &_id, const string &str)
{
	if (win && (id == _id))
		win->setTitle(str);
	test_controls(_id, c)
		c->SetString(str);
}

// replace all the text with a numerical value (int)
//    for all
// select an item
//    for ComboBox, TabControl, ListView?
void HuiPanel::setInt(const string &_id, int n)
{
	test_controls(_id, c)
		c->SetInt(n);
}

// replace all the text with a float
//    for all
void HuiPanel::setFloat(const string &_id, float f)
{
	test_controls(_id, c)
		c->SetFloat(f);
}

void HuiPanel::setImage(const string &_id, const string &image)
{
	test_controls(_id, c)
		c->SetImage(image);
}

void HuiPanel::setTooltip(const string &_id, const string &tip)
{
	test_controls(_id, c)
		c->SetTooltip(tip);
}


// add a single line/string
//    for ComboBox, ListView, ListViewTree, ListViewIcons
void HuiPanel::addString(const string &_id, const string &str)
{
	test_controls(_id, c)
		c->AddString(str);
}

// add a single line as a child in the tree of a ListViewTree
//    for ListViewTree
void HuiPanel::addChildString(const string &_id, int parent_row, const string &str)
{
	test_controls(_id, c)
		c->AddChildString(parent_row, str);
}

// change a single line in the tree of a ListViewTree
//    for ListViewTree
void HuiPanel::changeString(const string &_id,int row,const string &str)
{
	test_controls(_id, c)
		c->ChangeString(row, str);
}

// listview / treeview
string HuiPanel::getCell(const string &_id, int row, int column)
{
	test_controls(_id, c)
		return c->GetCell(row, column);
	return "";
}

// listview / treeview
void HuiPanel::setCell(const string &_id, int row, int column, const string &str)
{
	test_controls(_id, c)
		c->SetCell(row, column, str);
}

void HuiPanel::setColor(const string &_id, const color &col)
{
	test_controls(_id, c)
		c->SetColor(col);
}

// retrieve the text
//    for edit
string HuiPanel::getString(const string &_id)
{
	test_controls(_id, c)
		return c->GetString();
	return "";
}

// retrieve the text as a numerical value (int)
//    for edit
// which item/line is selected?
//    for ComboBox, TabControl, ListView
int HuiPanel::getInt(const string &_id)
{
	test_controls(_id, c)
		return c->GetInt();
	return 0;
}

// retrieve the text as a numerical value (float)
//    for edit
float HuiPanel::getFloat(const string &_id)
{
	test_controls(_id, c)
		return c->GetFloat();
	return 0;
}

color HuiPanel::getColor(const string &_id)
{
	test_controls(_id, c)
		return c->GetColor();
	return Black;
}

// switch control to usable/unusable
//    for all
void HuiPanel::enable(const string &_id,bool enabled)
{
	test_controls(_id, c)
		c->Enable(enabled);
}

// show/hide control
//    for all
void HuiPanel::hideControl(const string &_id,bool hide)
{
	test_controls(_id, c)
		c->Hide(hide);
}

// mark as "checked"
//    for CheckBox, ToolBarItemCheckable
void HuiPanel::check(const string &_id,bool checked)
{
	test_controls(_id, c)
		c->Check(checked);
}

// is marked as "checked"?
//    for CheckBox
bool HuiPanel::isChecked(const string &_id)
{
	test_controls(_id, c)
		return c->IsChecked();
	return false;
}

// which lines are selected?
//    for ListView
Array<int> HuiPanel::getMultiSelection(const string &_id)
{
	test_controls(_id, c)
		return c->GetMultiSelection();
	Array<int> sel;
	return sel;
}

void HuiPanel::setMultiSelection(const string &_id, Array<int> &sel)
{
	test_controls(_id, c)
		c->SetMultiSelection(sel);
}

// delete all the content
//    for ComboBox, ListView
void HuiPanel::reset(const string &_id)
{
	test_controls(_id, c)
		c->Reset();
}

// expand a single row
//    for TreeView
void HuiPanel::expand(const string &_id, int row, bool expand)
{
	test_controls(_id, c)
		c->Expand(row, expand);
}

// expand all rows
//    for TreeView
void HuiPanel::expandAll(const string &_id, bool expand)
{
	test_controls(_id, c)
		c->ExpandAll(expand);
}

// is column in tree expanded?
//    for TreeView
bool HuiPanel::isExpanded(const string &_id, int row)
{
	test_controls(_id, c)
		return false;
	return false;
}

void HuiPanel::deleteControl(const string &_id)
{
	for(int i=control.num-1;i>=0;i--)
		if (control[i]->id == _id)
			delete(control[i]);
}

void HuiPanel::setOptions(const string &_id, const string &options)
{
	test_controls(_id, c)
		c->SetOptions(options);
}

