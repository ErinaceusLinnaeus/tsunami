/*
 * BufferPainter.h
 *
 *  Created on: 13.11.2018
 *      Author: michi
 */

#pragma once

#include "../../lib/base/base.h"
#include "../../lib/math/rect.h"
#include "../../lib/image/color.h"

class AudioView;
class Painter;
class AudioBuffer;
class color;
class Range;

enum class AudioViewMode;

class BufferPainter {
public:
	BufferPainter(AudioView *view);


	void draw_buffer(Painter *c, AudioBuffer &b, int offset);
	void draw_buffer_selection(Painter *c, AudioBuffer &b, int offset);

	void draw_peaks(Painter *c, AudioBuffer &b, int offset);
	void draw_spectrum(Painter *c, AudioBuffer &b, int offset);

	void set_context(const rect &area, AudioViewMode mode);
	void set_color(const color &fg, const color &bg);
	void set_clip(const Range &r);

	AudioView *view;
	rect area;
	color col1, col2, col2sel;
	float x0, x1;
	AudioViewMode mode;
};
