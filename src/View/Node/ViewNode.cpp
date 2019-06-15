/*
 * ViewNode.cpp
 *
 *  Created on: 08.06.2019
 *      Author: michi
 */

#include "ViewNode.h"
#include "SceneGraph.h"

ViewNode::ViewNode() : ViewNode(0, 0) {
}

ViewNode::ViewNode(float w, float h) {
	align.horizontal = AlignData::Mode::NONE;
	align.vertical = AlignData::Mode::NONE;
	align.dx = 0;
	align.dy = 0;
	align.w = w;
	align.h = h;
	align.dz = 1;
	area = rect(0, w, 0, h);
	parent = nullptr;
	area = rect::EMPTY;
	hidden = false;
	z = 0;
}

ViewNode::~ViewNode() {
	for (auto c: children)
		delete c;
}

bool ViewNode::hover(float mx, float my) {
	if (hidden)
		return false;
	return area.inside(mx, my);
}

void ViewNode::add_child(ViewNode* child) {
	children.add(child);
	child->parent = this;
}

void ViewNode::delete_child(ViewNode* child) {
	delete child;
	for (int i=0; i<children.num; i++)
		if (children[i] == child)
			children.erase(i);
}

ViewNode *ViewNode::root() {
	ViewNode *r = this;
	while (r->parent)
		r = r->parent;
	return r;
}

bool ViewNode::is_cur_hover() {
	for (auto *c: children)
		if (c->is_cur_hover())
			return true;
	return is_cur_hover_non_recursive();
}

bool ViewNode::is_cur_hover_non_recursive() {
	if (auto *sg = dynamic_cast<SceneGraph*>(root())) {
		return sg->hover.node == this;
	}
	return false;
}

HoverData ViewNode::get_hover_data(float mx, float my) {
	HoverData h;
	h.node = this;
	return h;
}

string ViewNode::get_tip() {
	return "";
}

void ViewNode::update_area() {
	if (parent) {
		z = parent->z + align.dz;
		if (align.horizontal == AlignData::Mode::FILL) {
			area.x1 = parent->area.x1;
			area.x2 = parent->area.x2;
		} else if (align.horizontal == AlignData::Mode::LEFT) {
			area.x1 = parent->area.x1 + align.dx;
			area.x2 = area.x1 + align.w;
		} else if (align.horizontal == AlignData::Mode::RIGHT) {
			area.x2 = parent->area.x2 + align.dx;
			area.x1 = area.x2 - align.w;
		}

		if (align.vertical == AlignData::Mode::FILL) {
			area.y1 = parent->area.y1;
			area.y2 = parent->area.y2;
		} else if (align.vertical == AlignData::Mode::TOP) {
			area.y1 = parent->area.y1 + align.dy;
			area.y2 = area.y1 + align.h;
		} else if (align.vertical == AlignData::Mode::BOTTOM) {
			area.y2 = parent->area.y2 + align.dy;
			area.y1 = area.y2 - align.h;
		}
	}
}

ViewNodeFree::ViewNodeFree() : ViewNode(0, 0) {
}

ViewNodeRel::ViewNodeRel(float dx, float dy, float w, float h) : ViewNode(w, h) {
	align.horizontal = AlignData::Mode::LEFT;
	align.dx = dx;
	align.vertical = AlignData::Mode::TOP;
	align.dy = dy;
}
