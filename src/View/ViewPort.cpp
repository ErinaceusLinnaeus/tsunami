/*
 * ViewPort.cpp
 *
 *  Created on: 21.06.2015
 *      Author: michi
 */

#include "ViewPort.h"
#include "AudioView.h"
#include "../lib/math/math.h"

const float ViewPort::BORDER_FACTOR = 1.0f / 25.0f;
const float ViewPort::BORDER_FACTOR_RIGHT = ViewPort::BORDER_FACTOR * 8;

ViewPort::ViewPort(AudioView *v)
{
	view = v;
	scale = 1.0f;
	pos = 0;
	pos_target = 0;
	pos_pre_animation = 0;
	animation_time = -1;
	animation_non_linearity = 0;
}


double ViewPort::screen2sample(double _x)
{
	return (_x - view->song_area.x1) / scale + pos;
}

double ViewPort::sample2screen(double s)
{
	return view->song_area.x1 + (s - pos) * scale;
}

double ViewPort::dsample2screen(double ds)
{
	return ds * scale;
}

double ViewPort::dscreen2sample(double dx)
{
	return dx / scale;
}

void ViewPort::range2screen(const Range &r, float &x1, float &x2)
{
	x1 = sample2screen((double)r.start());
	x2 = sample2screen((double)r.end());
}

void ViewPort::range2screen_clip(const Range &r, const rect &area, float &x1, float &x2)
{
	range2screen(r, x1, x2);
	x1 = clampf(x1, area.x1, area.x2);
	x2 = clampf(x2, area.x1, area.x2);
}

void ViewPort::zoom(float f)
{
	// max zoom: 20 pixel per sample
	double scale_new = clampf(scale * f, 0.000001, 20.0);

	pos += (view->mx - view->song_area.x1) * (1.0/scale - 1.0/scale_new);
	pos_target = pos_pre_animation = pos;
	scale = scale_new;
	view->notify(view->MESSAGE_VIEW_CHANGE);
	view->force_redraw();
}

void ViewPort::move(float dpos)
{
	/*pos += dpos;
	view->notify(view->MESSAGE_VIEW_CHANGE);
	view->forceRedraw();*/
	set_target(pos_target + dpos, 0.0f);
}

// nonlin=0   feels "faster", more responsive, good for continuous human controls
// nonlin=0.7 feels smoother, good for automatic single jumps
void ViewPort::set_target(float target, float nonlin)
{
	pos_pre_animation = pos;
	pos_target = target;
	animation_time = 0;
	animation_non_linearity = nonlin;
	view->notify(view->MESSAGE_VIEW_CHANGE);
	view->force_redraw();
}

void ViewPort::update(float dt)
{
	if (animation_time < 0)
		return;
	animation_time += dt;
	double t = animation_time;
	if (t >= 1){
		pos = pos_target;
		animation_time = -1;
	}else{
		float s = animation_non_linearity;
		t = s*(-2*t*t*t + 3*t*t) + (1-s) * t;
		pos = t * pos_target + (1-t) * pos_pre_animation;
	}
	view->notify(view->MESSAGE_VIEW_CHANGE);
	view->force_redraw();
}

bool ViewPort::needs_update()
{
	return (animation_time >= 0);
}

Range ViewPort::range()
{
	return Range(pos, view->song_area.width() / scale);
}

void ViewPort::make_sample_visible(int sample, int samples_ahead)
{
	double x = sample2screen(sample);
	float border = 0;
	if (view->is_playback_active())
		border = dsample2screen(samples_ahead);
	float dx = view->song_area.width() * BORDER_FACTOR;
	float dxr = min(view->song_area.width() * BORDER_FACTOR_RIGHT, dx + border);

	if (samples_ahead > 0){
		// playback: always jump until the cursor is on the left border
		if ((x > view->song_area.x2 - dxr) or (x < view->song_area.x1 + dx))
			set_target(sample - dscreen2sample(dx), 0.7f);
	}else{
		// no playback: minimal jump until the cursor is between the borders
		if (x > view->song_area.x2 - dxr)
			set_target(sample - dscreen2sample(view->song_area.width() - dxr), 0.7f);
		else if (x < view->song_area.x1 + dx)
			set_target(sample - dscreen2sample(dx), 0.7f);
	}
}

rect ViewPort::nice_mapping_area()
{
	// mapping target area
	float x0 = view->song_area.x1;
	float x1 = view->song_area.x2;
	if (x1 - x0 > 800)
		x0 += view->TRACK_HANDLE_WIDTH;
	float w = x1 - x0;
	x0 += w * BORDER_FACTOR;
	x1 -= w * BORDER_FACTOR;
	return rect(x0, x1, view->song_area.y1, view->song_area.y2);
}

void ViewPort::show(Range &r)
{
	rect mr = nice_mapping_area();

	// map r into (x0,x1)
	scale = mr.width() / (double)r.length;
	pos = (double)r.start() - (mr.x1 - view->song_area.x1) / scale;
	pos_pre_animation = pos;
	pos_target = pos;
	view->notify(view->MESSAGE_VIEW_CHANGE);
	view->force_redraw();
}

