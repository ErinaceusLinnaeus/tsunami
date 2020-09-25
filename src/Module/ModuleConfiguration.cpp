/*
 * ModuleConfiguration.cpp
 *
 *  Created on: 06.03.2019
 *      Author: michi
 */


//#include "../lib/kaba/kaba.h"
#include "ModuleConfiguration.h"
#include "Module.h"
#include "../lib/kaba/syntax/Class.h"
#include "../lib/kaba/syntax/SyntaxTree.h"
#include "../Data/SampleRef.h"
#include "../Data/Sample.h"
#include "../Data/Song.h"
#include "../Session.h"


void ModuleConfiguration::__init__() {
	new(this) ModuleConfiguration;
}

void ModuleConfiguration::__delete__() {
	this->ModuleConfiguration::~ModuleConfiguration();
}


Array<Kaba::ClassElement> get_unique_elements(const Kaba::Class *c) {
	Array<Kaba::ClassElement> r;
	for (auto &e: c->elements)
		if (!e.hidden())
			r.add(e);
	return r;
}

string var_to_string(const Kaba::Class *c, char *v) {
	string r;
	if (c == Kaba::TypeInt) {
		r += i2s(*(int*)v);
	} else if (c == Kaba::TypeChar) {
		r += i2s(*(char*)v);
	} else if (c == Kaba::TypeFloat32) {
		r += f2s(*(float*)v, 6);
	} else if (c == Kaba::TypeBool) {
		r += (*(bool*)v) ? "true" : "false";
	} else if (c == Kaba::TypeString) {
		r += "\"" + ((string*)v)->escape() + "\"";
	} else if (c->is_array()) {
		auto tel = c->get_array_element();
		r += "[";
		for (int i=0; i<c->array_length; i++){
			if (i > 0)
				r += " ";
			r += var_to_string(tel, &v[i * tel->size]);
		}
		r += "]";
	} else if (c->is_super_array()) {
		auto a = (DynamicArray*)v;
		auto tel = c->get_array_element();
		r += "[";
		for (int i=0; i<a->num; i++){
			if (i > 0)
				r += " ";
			r += var_to_string(tel, &(((char*)a->data)[i * tel->size]));
		}
		r += "]";
	} else if (c->name == "SampleRef*") {
		auto sr = *(SampleRef**)v;
		if (sr)
			r += i2s(sr->origin->get_index());
		else
			r += "nil";
	} else {
		auto e = get_unique_elements(c);
		r += "(";
		for (int i=0; i<e.num; i++) {
			if (i > 0)
				r += " ";
			r += var_to_string(e[i].type, &v[e[i].offset]);
		}
		r += ")";
	}
	return r;
}

string get_next(const string &var_temp, int &pos) {
	int start = pos;
	bool in_string = false;
	for (int i=start;i<var_temp.num;i++) {
		if ((i == start) and (var_temp[i] == '"')) {
			in_string = true;
		} else if (in_string) {
			if (var_temp[i] == '\\') {
				i ++;
			} else if (var_temp[i] == '"') {
				pos = i + 1;
				return var_temp.substr(start + 1, i - start - 1).unescape();
			}
		} else if ((var_temp[i] == ' ') or (var_temp[i] == ']') or (var_temp[i] == ')') or (var_temp[i] == '[') or (var_temp[i] == '(')) {
			pos = i;
			return var_temp.substr(start, i - start);
		}
	}
	return var_temp.substr(start, -1);
}

void var_from_string(const Kaba::Class *type, char *v, const string &s, int &pos, Session *session) {
	if (pos >= s.num)
		return;
	if (type == Kaba::TypeInt) {
		*(int*)v = get_next(s, pos)._int();
	} else if (type == Kaba::TypeChar) {
		*(char*)v = get_next(s, pos)._int();
	} else if (type == Kaba::TypeFloat32) {
		*(float*)v = get_next(s, pos)._float();
	} else if (type == Kaba::TypeBool) {
		*(bool*)v = (get_next(s, pos) == "true");
	} else if (type == Kaba::TypeString) {
		*(string*)v = get_next(s, pos);
	} else if (type->is_array()) {
		auto tel = type->get_array_element();
		pos ++; // '['
		for (int i=0;i<type->array_length;i++) {
			if (i > 0)
				pos ++; // ' '
			var_from_string(tel, &v[i * tel->size], s, pos, session);
		}
		pos ++; // ']'
	} else if (type->is_super_array()) {
		pos ++; // '['
		auto *a = (DynamicArray*)v;
		auto tel = type->get_array_element();
		a->simple_clear(); // todo...
		while (true) {
			if ((s[pos] == ']') or (pos >= s.num))
				break;
			if (a->num > 0)
				pos ++; // ' '
			a->simple_resize(a->num + 1);
			var_from_string(tel, &(((char*)a->data)[(a->num - 1) * tel->size]), s, pos, session);
		}
		pos ++; // ']'
	} else if (type->name == "SampleRef*") {
		string ss = get_next(s, pos);
		*(SampleRef**)v = nullptr;
		if ((ss != "nil") and session->song) {
			int n = ss._int();
			if ((n >= 0) and (n < session->song->samples.num)) {
				*(SampleRef**)v = new SampleRef(session->song->samples[n]);
			}
		}
	} else {
		auto e = get_unique_elements(type);
		pos ++; // '('
		for (int i=0; i<e.num; i++) {
			if (i > 0)
				pos ++; // ' '
			var_from_string(e[i].type, &v[e[i].offset], s, pos, session);
		}
		pos ++; // ')'
	}
}

string ModuleConfiguration::to_string() const {
	return to_any().str();
	//return var_to_string(_class, (char*)this);
}

Any ModuleConfiguration::to_any() const {
	throw Exception("TODO");
	return Any();
}

void ModuleConfiguration::from_string(const string &s, Session *session) {
	from_any(Any::parse(s), session);
}

void ModuleConfiguration::from_string_legacy(const string &s, Session *session) {
	reset();
	int pos = 0;
	var_from_string(_class, (char*)this, s, pos, session);
}

void ModuleConfiguration::from_any(const Any &a, Session *session) {
	reset();
	throw Exception("TODO");
}

bool ac_name_match(const string &const_name, const string &var_name) {
	return (("AUTO_CONFIG_" + var_name).upper().replace("_", "") == const_name.upper().replace("_", ""));
}

string ModuleConfiguration::auto_conf(const string &name) const {
	if (!_class)
		return "";
	auto *ps = _class->owner;
	if (!ps)
		return "";
	for (auto c: ps->base_class->constants) {
		if (c->type == Kaba::TypeString)
			if (ac_name_match(c->name, name))
				return c->as_string();
	}
	return "";
}

void ModuleConfiguration::changed() {
	if (_module)
		_module->changed();
}
