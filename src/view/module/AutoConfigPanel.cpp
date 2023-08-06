/*
 * AutoConfigPanel.cpp
 *
 *  Created on: 08.10.2017
 *      Author: michi
 */


#include "AutoConfigPanel.h"
#include "../helper/Slider.h"
#include "../helper/VolumeControl.h"
#include "../dialog/SampleSelectionDialog.h"
#include "../sidebar/SampleManagerConsole.h"
#include "../../module/Module.h"
#include "../../module/ModuleConfiguration.h"
#include "../../data/base.h"
#include "../../data/Song.h"
#include "../../data/Sample.h"
#include "../../data/SampleRef.h"
#include "../../data/midi/MidiData.h"
#include "../../lib/kaba/kaba.h"
#include "../../device/Device.h"
#include "../../device/DeviceManager.h"
#include "../../Session.h"


bool is_number(char c) {
	return (c >= '0' and c <= '9');
}

string snake_case_to_label(const string &s) {
	string r;
	bool next_upper = true;
	for (int i=0; i<s.num; i++) {
		if (s[i] == '_') {
			r += " ";
			next_upper = true;
			continue;
		} else {
			if (next_upper)
				r += s.sub(i, i+1).upper();
			else
				r += s.sub(i, i+1);
			next_upper = false;
		}
	}
	return r;
}

string shorten(const string &s, int max_length);

class AutoConfigData : public VirtualBase {
public:
	string name, label, unit;
	AutoConfigData(const string &_name) {
		name = _name;
		label = snake_case_to_label(_name);
	}
	virtual ~AutoConfigData() {}

	bool name_match(const string &const_name) {
		return (("AUTO_CONFIG_" + name).upper().replace("_", "") == const_name.upper().replace("_", ""));
	}
	virtual void parse(const string &s) = 0; // before a gui exists!
	virtual void add_gui(ConfigPanel *p, int i, const hui::Callback &callback) = 0;
	virtual void value_from_gui() = 0;
	virtual void value_to_gui() = 0;
};

class AutoConfigDataFloat : public obs::Node<AutoConfigData> {
public:
	float min, max, step, factor;
	float min_slider, max_slider;
	float *value;
	owned<Slider> slider;
	Slider::Mode mode = Slider::Mode::LINEAR;
	AutoConfigDataFloat(const string &_name) : obs::Node<AutoConfigData>(_name) {
		min = -100;
		max = 100;
		step = 0.1f;
		min_slider = min;
		max_slider = max;
		factor = 1;
		value = nullptr;
	}
	void parse(const string &s) override {
		if (is_number(s[0])) {
			// legacy
			auto p = s.explode(":");
			if (p.num == 5) {
				min = p[0]._float();
				max = p[1]._float();
				step = p[2]._float();
				factor = p[3]._float();
				step /= factor;
				unit = p[4];
			} else {
				msg_write("required format: min:max:step:factor:unit");
			}
		} else {
			for (const auto& p: s.explode(",")) {
				if (p.head(6) == "range=") {
					auto x = p.sub_ref(6).explode(":");
					min_slider = min = x[0]._float();
					if (x.num >= 2)
						max_slider = max = x[1]._float();
					if (x.num >= 3)
						step = x[2]._float();
				} else if (p.head(12) == "sliderrange=") {
					auto x = p.sub_ref(12).explode(":");
					min_slider = x[0]._float();
					if (x.num >= 2)
						max_slider = x[1]._float();
				} else if (p.head(5) == "step=") {
					step = p.sub_ref(5)._float();
				} else if (p.head(6) == "scale=") {
					factor = p.sub_ref(6)._float();
				} else if (p.head(5) == "unit=") {
					unit = p.sub(5);
				} else if (p == "exponential") {
					mode = Slider::Mode::EXPONENTIAL;
				} else if (p == "square") {
					mode = Slider::Mode::SQUARE;
				}
			}
		}
	}
	void add_gui(ConfigPanel *p, int i, const hui::Callback &callback) override {
		p->add_grid("", 1, i, "grid-" + i2s(i));
		p->set_target("grid-" + i2s(i));
		p->add_slider("!expandx", 0, 0, "slider-" + i2s(i));
		p->add_spin_button("!width=50\\" + f2s(*value, 6), 1, 0, "spin-" + i2s(i));
		//p->addLabel(unit, 2, 0, "");
		slider = new Slider(p, "slider-" + i2s(i), "spin-" + i2s(i));
		slider->out_value >> create_data_sink<float>([callback] (float f) { callback(); });
		slider->set_scale(factor);
		slider->set_range(min, max, step);
		slider->set_slider_range(min_slider, max_slider);
		slider->set(*value);
		slider->set_mode(mode);
	}
	void value_from_gui() override {
		*value = slider->get();
	}
	void value_to_gui() override {
		slider->set(*value);
	}
};

class AutoConfigDataBool : public AutoConfigData {
public:
	bool *value;
	ConfigPanel *panel;
	string id;
	AutoConfigDataBool(const string &_name) : AutoConfigData(_name) {
		value = nullptr;
		panel = nullptr;
	}
	~AutoConfigDataBool() override {};
	void parse(const string &s) override {};
	void add_gui(ConfigPanel *p, int i, const hui::Callback &callback) override {
		id = "check-" + i2s(i);
		panel = p;
		p->add_check_box("!width=150,expandx", 1, i, id);
		p->event(id, callback);
	}
	void value_from_gui() override {
		*value = panel->is_checked(id);
	}
	void value_to_gui() override {
		panel->check(id, *value);
	}
};

class AutoConfigDataInt : public AutoConfigData {
public:
	int min, max;
	int *value;
	ConfigPanel *panel;
	string id;
	AutoConfigDataInt(const string &_name) : AutoConfigData(_name) {
		min = 0;
		max = 1000000;
		value = nullptr;
		panel = nullptr;
	}
	void parse(const string &s) override {
		if (is_number(s[0])) {
			// legacy
			auto p = s.explode(":");
			if (p.num == 2) {
				min = p[0]._float();
				max = p[1]._float();
			} else {
				msg_write("required format: min:max");
			}
		} else {
			for (const auto& p: s.explode(",")) {
				if (p.head(6) == "range=") {
					auto x = p.sub_ref(6).explode(":");
					min = x[0]._float();
					if (x.num >= 2)
						max = x[1]._float();
				} else if (p.head(5) == "unit=") {
					unit = p.sub(5);
				}
			}
		}
	}
	void add_gui(ConfigPanel *p, int i, const hui::Callback &callback) override {
		id = "spin-" + i2s(i);
		panel = p;
		p->add_spin_button("!width=150,expandx\\" + i2s(*value), 1, i, id);
		p->set_options(id, format("range=%d:%d", min, max));
		p->event(id, callback);
	}
	void value_from_gui() override {
		*value = panel->get_int(id);
	}
	void value_to_gui() override {
		panel->set_int(id, *value);
	}
};

class AutoConfigDataSampleFormat : public AutoConfigData {
public:
	int *value;
	ConfigPanel *panel;
	string id;
	AutoConfigDataSampleFormat(const string &_name) : AutoConfigData(_name) {
		value = nullptr;
		panel = nullptr;
	}
	void parse(const string &s) override {
	}
	void add_gui(ConfigPanel *p, int i, const hui::Callback &callback) override {
		id = "combo-" + i2s(i);
		panel = p;
		p->add_combo_box("!width=150,expandx", 1, i, id);
		for (int i=1; i<(int)SampleFormat::NUM; i++)
			p->add_string(id, format_name((SampleFormat)i));
		p->event(id, callback);
	}
	void value_from_gui() override {
		*value = panel->get_int(id) + 1;
	}
	void value_to_gui() override {
		panel->set_int(id, *value - 1);
	}
};

class AutoConfigDataPitch : public AutoConfigData {
public:
	float *value;
	string id;
	string id_freq;
	string id_mode;
	string id_hz;
	ConfigPanel *panel;
	hui::Callback callback;
	AutoConfigDataPitch(const string &_name) : AutoConfigData(_name) {
		value = nullptr;
		panel = nullptr;
		callback = nullptr;
	}
	void parse(const string &s) override {}
	void add_gui(ConfigPanel *p, int i, const hui::Callback &cb) override {
		callback = cb;
		id = "pitch-" + i2s(i);
		id_freq = "freq-" + i2s(i);
		id_hz = "hz-" + i2s(i);
		id_mode = "mode-" + i2s(i);
		string id_grid = "grid-" + i2s(i);
		panel = p;
		p->add_grid("", 1, i, id_grid);
		p->add_toggle_button("f", 2, i, id_mode);
		p->set_tooltip(id_mode, _("mode: frequency/pitch"));
		p->set_target(id_grid);
		p->add_combo_box("!width=150,expandx", 0, 0, id);
		for (int j=0; j<MAX_PITCH; j++)
			p->add_string(id, pitch_name(j));
		p->add_spin_button("!range=0:10000:0.1,expandx", 1, 0, id_freq);
		p->add_label("Hz", 2, 0, id_hz);
		on_mode();
		p->set_int(id, (int)*value);
		p->set_float(id_freq, pitch_to_freq(*value));
		p->event(id, [this] { on_pitch(); });
		p->event(id_freq, [this] { on_freq(); });
		p->event(id_mode, [this] { on_mode(); });
	}
	void value_from_gui() override {
		bool m = panel->is_checked(id_mode);
		if (m)
			*value = freq_to_pitch(panel->get_float(id_freq));
		else
			*value = (float)panel->get_int(id);
	}
	void value_to_gui() override {
		panel->set_int(id, (int)*value);
		panel->set_float(id_freq, pitch_to_freq(*value));
	}
	void on_pitch() {
		float p = panel->get_float(id);
		panel->set_float(id_freq, pitch_to_freq(p));
		callback();
	}
	void on_freq() {
		float f = panel->get_float(id_freq);
		panel->set_int(id, (int)freq_to_pitch(f));
		callback();
	}
	void on_mode() {
		bool m = panel->is_checked(id_mode);
		panel->hide_control(id, m);
		panel->hide_control(id_freq, !m);
		panel->hide_control(id_hz, !m);
	}
};

class AutoConfigDataVolume : public obs::Node<AutoConfigData> {
public:
	float value_min, value_max;
	float *value;
	owned<VolumeControl> volume_control;
	AutoConfigDataVolume(const string &_name) : obs::Node<AutoConfigData>(_name) {
		value = nullptr;
		value_min = 0;
		value_max = 1;
	}
	void add_gui(ConfigPanel *p, int i, const hui::Callback &callback) override {
		p->remove_control(format("unit-%d", i));
		p->add_button("!small,padding=2", 2, i, format("unit-%d", i));
		p->add_grid("", 1, i, "grid-" + i2s(i));
		p->set_target("grid-" + i2s(i));
		p->add_slider("!expandx", 0, 0, "slider-" + i2s(i));
		p->add_spin_button("", 1, 0, "spin-" + i2s(i));
		volume_control = new VolumeControl(p, "slider-" + i2s(i), "spin-" + i2s(i), "unit-" + i2s(i));
		volume_control->out_volume >> create_data_sink<float>([this, callback] (float f) {
			callback();
		});
		volume_control->set_range(value_min, value_max);
	}
	void parse(const string &s) override {
		for (const auto& p: s.explode(",")) {
			if (p.head(6) == "range=") {
				auto x = p.sub_ref(6).explode(":");
				if (x.num >= 2) {
					value_min = x[0]._float();
					value_max = x[1]._float();
				}
			}
		}
	}
	void value_from_gui() override {
		*value = volume_control->get();
	}
	void value_to_gui() override {
		volume_control->set(*value);
	}
};

class AutoConfigDataString : public AutoConfigData {
public:
	string *value;
	string id;
	ConfigPanel *panel;
	Array<string> choices;
	AutoConfigDataString(const string &_name) : AutoConfigData(_name) {
		value = nullptr;
		panel = nullptr;
	}
	void parse(const string &s) override {
		for (const auto& p: s.explode(",")) {
			if (p.head(7) == "choice=") {
				choices = p.sub_ref(7).explode("|");
			}
		}
	}
	void add_gui(ConfigPanel *p, int i, const hui::Callback &callback) override {
		panel = p;
		id = "string-" + i2s(i);
		if (choices.num > 0) {
			p->add_combo_box("!expandx", 1, i, id);
			for (string &c: choices)
				p->add_string(id, c);
		} else {
			p->add_edit("!width=150,expandx\\" + *value, 1, i, id);
		}
		p->event(id, callback);
	}
	void value_from_gui() override {
		if (choices.num > 0) {
			int n = panel->get_int(id);
			if (n >= 0)
				*value = choices[n];
		} else {
			*value = panel->get_string(id);
		}
	}
	void value_to_gui() override {
		if (choices.num > 0) {
			foreachi (string &c, choices, i)
				if (c == *value)
					panel->set_int(id, i);
		} else {
			panel->set_string(id, *value);
		}
	}
};

class AutoConfigDataSampleRef : public AutoConfigData {
public:
	shared<SampleRef> *value;
	string id;
	ConfigPanel *panel;
	Session *session;
	hui::Callback callback;
	AutoConfigDataSampleRef(const string &_name, Session *_session) : AutoConfigData(_name) {
		value = nullptr;
		panel = nullptr;
		session = _session;
	}
	void parse(const string &s) override {}
	void add_gui(ConfigPanel *p, int i, const hui::Callback &_callback) override {
		id = "sample-" + i2s(i);
		panel = p;
		p->add_button("!expandx", 1, i, id);
		value_to_gui();
		p->event(id, [this] { on_button(); });
		callback = _callback;
	}
	void on_button() {
		Sample *old = nullptr;
		if (*value)
			old = (*value)->origin.get();
		SampleSelectionDialog::select(session, panel, old).on([this, old] (Sample *s) {
			if (s != old) {
				*value = nullptr;
				if (s)
					*value = s->create_ref();
				value_to_gui();
				callback();
			}
		});
	}
	void value_from_gui() override {
	}
	void value_to_gui() override {
		if (*value)
			panel->set_string(id, (*value)->origin->name);
		else
			panel->set_string(id, _(" - none -"));
	}
};

class AutoConfigDataDevice: public AutoConfigData {
public:
	Device **value;
	string id;
	ConfigPanel *panel;
	DeviceType type;
	Session *session;
	Array<Device*> list;
	AutoConfigDataDevice(const string &_name, Session *_session) : AutoConfigData(_name) {
		session = _session;
		value = nullptr;
		panel = nullptr;
		type = DeviceType::AUDIO_OUTPUT;
	}
	void parse(const string &s) override {
		type = DeviceType::AUDIO_OUTPUT;
		if (s == "audio:input")
			type = DeviceType::AUDIO_INPUT;
		if (s == "midi:input")
			type = DeviceType::MIDI_INPUT;
	}
	void update_list() {
		list = session->device_manager->good_device_list(type);
		if (panel) {
			panel->reset(id);
			for (auto *d: list)
				panel->add_string(id, shorten(d->get_name(), 42));
			panel->set_int(id, list.find(*value));
		}
	}
	void add_gui(ConfigPanel *p, int i, const hui::Callback &callback) override {
		id = "device-" + i2s(i);
		panel = p;
		p->add_combo_box("!expandx", 1, i, id);
		p->event(id, callback);
		update_list();
	}
	void value_from_gui() override {
		*value = session->device_manager->choose_device(type);
		int n = panel->get_int(id);
		if (n >= 0)
			*value = list[n];
	}
	void value_to_gui() override {
		panel->set_int(id, list.find(*value));
	}
};

Array<AutoConfigData*> get_auto_conf(ModuleConfiguration *config, Session *session) {
	//auto *ps = config->kaba_class->owner;
	Array<AutoConfigData*> r;
	for (auto &e: config->kaba_class->elements) {
		auto cc = config->auto_conf(e.name);
		if (cc == "ignore")
			continue;
		if (e.type == kaba::TypeFloat32) {
			if (e.name == "pitch" or (cc.find("pitch") >= 0)) {
				auto *a = new AutoConfigDataPitch(e.name);
				a->value = (float*)((char*)config + e.offset);
				r.add(a);
			} else if (e.name == "volume" or (cc.find("volume") >= 0)) {
				auto *a = new AutoConfigDataVolume(e.name);
				a->value = (float*)((char*)config + e.offset);
				r.add(a);
			} else {
				auto *a = new AutoConfigDataFloat(e.name);
				a->value = (float*)((char*)config + e.offset);
				r.add(a);
			}
		} else if (e.type == kaba::TypeInt) {
			if (cc == "sample-format") {
				auto *a = new AutoConfigDataSampleFormat(e.name);
				a->value = (int*)((char*)config + e.offset);
				r.add(a);
			} else {
				auto *a = new AutoConfigDataInt(e.name);
				a->value = (int*)((char*)config + e.offset);
				r.add(a);
			}
		} else if (e.type == kaba::TypeBool) {
			auto *a = new AutoConfigDataBool(e.name);
			a->value = (bool*)((char*)config + e.offset);
			r.add(a);
		} else if (e.type == kaba::TypeString) {
			auto *a = new AutoConfigDataString(e.name);
			a->value = (string*)((char*)config + e.offset);
			r.add(a);
		} else if (e.type->name == "shared[SampleRef]") {
			auto *a = new AutoConfigDataSampleRef(e.name, session);
			a->value = (shared<SampleRef>*)((char*)config + e.offset);
			r.add(a);
		} else if (e.type->name == "Device*") {
			auto *a = new AutoConfigDataDevice(e.name, session);
			a->value = (Device**)((char*)config + e.offset);
			r.add(a);
		} else {
			continue;
		}
		r.back()->parse(cc);
	}

	return r;
}

AutoConfigPanel::AutoConfigPanel(Array<AutoConfigData*> &_aa, Module *_c) :
	ConfigPanel(_c) {
	aa = _aa;
	add_grid("", 0, 0, "grid");
	set_target("grid");
	foreachi(auto *a, aa, i) {
		set_target("grid");
		add_label("!right,disabled\\" + a->label, 0, i, format("label-%d", i));
		add_label(a->unit, 2, i, format("unit-%d", i));
		a->add_gui(this, i, [a,this]{
			a->value_from_gui();
			changed();
		});
	}
}

AutoConfigPanel::~AutoConfigPanel() {
	for (auto a: aa)
		delete a;
}

void AutoConfigPanel::update() {
	for (auto a: aa)
		a->value_to_gui();
}

