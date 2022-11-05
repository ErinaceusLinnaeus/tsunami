/*
 * FormatPdf.cpp
 *
 *  Created on: 22.04.2018
 *      Author: michi
 */

#include "FormatPdf.h"
#include "../dialog/PdfConfigDialog.h"
#include "../../data/base.h"
#include "../../data/Song.h"
#include "../../data/Track.h"
#include "../../data/SongSelection.h"
#include "../../lib/doc/pdf.h"
#include "../../lib/math/rect.h"
#include "../../view/ColorScheme.h"
#include "../../view/helper/SymbolRenderer.h"
#include "../../view/HoverData.h"
#include "../../view/painter/MultiLinePainter.h"
#include <math.h>

static const color NOTE_COLOR = color(1, 0.3f, 0.3f, 0.3f);
static const color NOTE_COLOR_TAB = color(1, 0.8f, 0.8f, 0.8f);
static const float PAGE_WIDTH_A4 = 595.276f; // pts
static const float PAGE_HEIGHT_A4 = 841.89f;

FormatDescriptorPdf::FormatDescriptorPdf() :
	FormatDescriptor(_("Pdf sheet"), "pdf", Flag::MIDI | Flag::MULTITRACK | Flag::WRITE) {}


bool FormatPdf::get_parameters(StorageOperationData *od, bool save) {
	// optional defaults
	if (!od->parameters.has("horizontal-scale"))
		od->parameters.map_set("horizontal-scale", 1.0f);

	if (od->parameters.has("tracks"))
		return true;

	// mandatory defaults
	if (!od->parameters.has("tracks"))
		od->parameters.map_set("tracks", {});
	
	bool ok = false;
	auto dlg = new PdfConfigDialog(od, od->win);
	hui::run(dlg, [dlg, &ok] {
		ok = dlg->ok;
	});
	return ok;
}

ColorScheme create_pdf_color_scheme() {
	ColorScheme bright;
	bright.background = White;
	bright.text = Black;//color(1, 0.3f, 0.3f, 0.1f);
	bright.selection = color(1, 0.2f, 0.2f, 0.7f);
	bright.hover = White;
	bright.gamma = 1.0f;
	bright.name = "pdf";
	bright.auto_generate();
	return bright;
}

MultiLinePainter *prepare_pdf_multi_line_view(Song *song, const ColorScheme &_colors, const Any &params) {

	// A4
	float page_width = PAGE_WIDTH_A4;
	float page_height = PAGE_HEIGHT_A4;

	float border = 25;

	float horizontal_scale = params["horizontal-scale"]._float();
	float avg_scale = 65.0f / song->sample_rate * horizontal_scale;
	float avg_samples_per_line = (page_width - 2*border) / avg_scale;

	auto mlp = new MultiLinePainter(song, _colors);
	mlp->set_context(params["tracks"], page_width, avg_samples_per_line);
	Any conf;
	conf["border"] = border;
	conf["line-height"] = 20;
	mlp->set(conf);

	SymbolRenderer::enable(false);

	int samples = song->range().end();
	mlp->line_space = 25;
	return mlp;
}

float draw_pdf_header(Painter *p, Song *song, float page_width, const ColorScheme &_colors) {
	p->set_color(_colors.text);
	p->set_font("Times", 18, false, false);
	//p->set_font("Helvetica", 25, false, false);
	p->draw_str(vec2(100, 25), song->get_tag("title"));
	if (song->get_tag("artist").num > 0) {
		p->set_font("Courier", 12, false, false);
		p->set_font_size(12);
		p->set_color(_colors.text_soft2);
		p->draw_str(vec2(page_width - 150, 25), "by " + song->get_tag("artist"));
	}

	// y0 offset
	return 70;
}

void FormatPdf::save_song(StorageOperationData* _od) {
	od = _od;
	song = od->song;

	// A4
	float page_width = PAGE_WIDTH_A4;
	float page_height = PAGE_HEIGHT_A4;

	ColorScheme _colors = create_pdf_color_scheme();

	auto mlp = prepare_pdf_multi_line_view(song, _colors, od->parameters);

	pdf::Parser parser;
	parser.set_page_size(page_width, page_height);

	SymbolRenderer::enable(false);

	int samples = song->range().end();

	auto p = parser.add_page();

	float y0 = draw_pdf_header(p, song, page_width, _colors);
	p->set_font("Helvetica", 8, false, false);

	int offset = 0;
	while (offset < samples) {
		float y_prev = y0;
		y0 = mlp->draw_next_line(p, offset, {0, y0});

		// new page?
		float dy = y0 - y_prev;
		if (y0 + dy > page_height and offset < samples) {
			p = parser.add_page();
			y0 = 50;
		}
	}

	delete mlp;

	parser.save(od->filename);
}
