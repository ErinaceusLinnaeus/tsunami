/*
 * ViewModeCurve.cpp
 *
 *  Created on: 14.11.2015
 *      Author: michi
 */

#include "ViewModeCurve.h"
#include "../AudioView.h"
#include "../../Data/Curve.h"
#include "../../Data/Track.h"
#include "../../Device/Stream/AudioOutput.h"
#include "../../TsunamiWindow.h"
#include "../../Module/Audio/SongRenderer.h"
#include "../Graph/AudioViewLayer.h"
#include "../Graph/AudioViewTrack.h"
#include "../SideBar/SideBar.h"

ViewModeCurve::ViewModeCurve(AudioView* view) :
	ViewModeDefault(view)
{
	_curve = nullptr;
}


void ViewModeCurve::on_start() {
	set_side_bar(SideBar::CURVE_CONSOLE);
}

AudioViewTrack *ViewModeCurve::cur_vtrack() {
	return view->cur_vtrack();
}

Track *ViewModeCurve::cur_track() {
	return view->cur_track();
}

void ViewModeCurve::left_click_handle_void(AudioViewLayer *vlayer) {
	if (target == "")
		return;

	if (!_curve) {
		CurveTarget t;
		t.from_id(target, cur_track());
		_curve = cur_track()->add_curve("", t);
	}

	if (hover().type == HoverData::Type::CURVE_POINT_NONE) {
		int pos = view->get_mouse_pos();
		float value = screen2value(view->my);
		cur_track()->curve_add_point(_curve, pos, value);
	} else if (hover().type == HoverData::Type::CURVE_POINT) {
		view->mdp_prepare([=] {
			int pos = view->get_mouse_pos();
			float value = screen2value(view->my);
			cur_track()->curve_edit_point(_curve, view->cur_selection.index, pos, value);
		});
	}
}

void ViewModeCurve::on_key_down(int k) {
	ViewModeDefault::on_key_down(k);

	if (_curve and (view->cur_selection.type == HoverData::Type::CURVE_POINT))
		if (k == hui::KEY_DELETE){
			cur_track()->curve_delete_point(_curve, view->cur_selection.index);
			view->cur_selection.clear();
		}
}

void ViewModeCurve::draw_track_data(Painter* c, AudioViewTrack* t) {
	ViewModeDefault::draw_track_data(c, t);

	if (t != cur_vtrack())
		return;

	rect r = t->area;
	if (_curve) {

		// lines
		c->set_line_width(1.0f);
		c->set_color(view->colors.text);
		Array<complex> pp;
		for (int x=r.x1; x<r.x2; x+=3)
			pp.add(complex(x, value2screen(_curve->get(cam->screen2sample(x)))));
		c->draw_lines(pp);

		// points
		foreachi(auto &p, _curve->points, i) {
			float r = 3;
			if ((hover().type == HoverData::Type::CURVE_POINT) and (i == hover().index)) {
				c->set_color(view->colors.selection_boundary_hover);
				r = 5;
			} else if ((view->cur_selection.type == HoverData::Type::CURVE_POINT) and (i == view->cur_selection.index)) {
				// TODO.... selected...
				c->set_color(view->colors.selection_boundary);
			} else {
				c->set_color(view->colors.text);
			}
			c->draw_circle(cam->sample2screen(p.pos), value2screen(p.value), r);
		}
	}
}

void ViewModeCurve::draw_post(Painter* c) {
	auto *t = cur_vtrack();
	if (!t)
		return;
	color col = view->colors.text;
	col.a = 0.1f;
	float d = 12;
	c->set_color(col);
	c->draw_rect(view->song_area().x1, t->area.y1-d, view->song_area().width(), d);
	c->draw_rect(view->song_area().x1, t->area.y2, view->song_area().width(), d);
	d = 2;
	col.a = 0.7f;
	c->set_color(col);
	c->draw_rect(view->song_area().x1, t->area.y1-d, view->song_area().width(), d);
	c->draw_rect(view->song_area().x1, t->area.y2, view->song_area().width(), d);
}

HoverData ViewModeCurve::get_hover_data(AudioViewLayer *vlayer, float mx, float my) {
	auto s = vlayer->Node::get_hover_data(mx, my);//ViewModeDefault::get_hover_data(vlayer, mx, my);
	s.type = HoverData::Type::CURVE_POINT_NONE;

	// curve points
	if (_curve) {
		foreachi(auto &p, _curve->points, i) {
			float x = cam->sample2screen(p.pos);
			float y = value2screen(p.value);
			if ((fabs(mx - x) < 10) and (fabs(my - y) < 10)) {
				s.type = HoverData::Type::CURVE_POINT;
				s.index = i;
				return s;
			}
		}
	}

	return s;
}

void ViewModeCurve::set_curve_target(const string &id) {
	target = id;
	_curve = nullptr;
	if (cur_track())
		for (auto *c: weak(cur_track()->curves))
			if (c->target.id == target)
				_curve = c;
	view->force_redraw();
}

float ViewModeCurve::value2screen(float value) {
	if (!cur_track())
		return 0;
	if (!_curve)
		return cur_vtrack()->area.y2 - cur_vtrack()->area.height() * value;
	return cur_vtrack()->area.y2 - cur_vtrack()->area.height() * (value - _curve->min) / (_curve->max - _curve->min);
}

float ViewModeCurve::screen2value(float y) {
	if (!cur_vtrack())
		return 0;
	if (!_curve)
		return (cur_vtrack()->area.y2 - y) / cur_vtrack()->area.height();
	return _curve->min + (cur_vtrack()->area.y2 - y) / cur_vtrack()->area.height() * (_curve->max - _curve->min);
}

string ViewModeCurve::get_tip() {
	return "click to add points onto the track,  delete...";
}
