/*
 * FormatSoundFont2.cpp
 *
 *  Created on: 19.05.2015
 *      Author: michi
 */

#include "FormatSoundFont2.h"
#include "../../lib/file/file.h"
#include "../../Data/base.h"
#include "../../Data/Song.h"
#include "../../Data/Sample.h"
#include "../../Data/Audio/AudioBuffer.h"

FormatDescriptorSoundFont2::FormatDescriptorSoundFont2() :
	FormatDescriptor("SoundFont2", "sf2", Flag::AUDIO | Flag::TAGS | Flag::SAMPLES | Flag::READ)
{
}

FormatSoundFont2::FormatSoundFont2() {
	sample_offset = 0;
	sample_count = 0;
	od = nullptr;
	song = nullptr;
}

void FormatSoundFont2::load_song(StorageOperationData *_od) {
	od = _od;
	song = od->song;

	File *f = nullptr;

	sample_offset = -1;

	try {
		f = FileOpen(od->filename);
		read_chunk(f);
		if (sample_offset > 0)
			read_samples(f);
	} catch(Exception &e) {
		od->error(e.message());
	}


	delete(f);
}

void FormatSoundFont2::sfSample::print() {
	msg_write("----sample");
	msg_write(name);
	msg_write(start);
	msg_write(end);
	msg_write(start_loop);
	msg_write(end_loop);
	msg_write(sample_rate);
	msg_write(sample_type);
}

string read_str(File *f, int l) {
	string s;
	s.resize(l);
	f->read_buffer(s);
	int p0 = s.find(string("\0", 1), 0);
	if (p0 >= 0)
		return s.head(p0);
	return s;
}

void FormatSoundFont2::read_chunk(File *f) {
	string name = read_str(f, 4).upper();
	int l = f->read_int();
	int after_pos = f->get_pos() + l;

	msg_write(format("chunk: %s (%d)", name, l));
	msg_right();


	if (name == "RIFF") {
		string aaa = read_str(f, 4);
		if (aaa != "sfbk")
			throw Exception("'sfbk' expected in 'RIFF' chunk");

		read_chunk(f);
		read_chunk(f);
		read_chunk(f);
	} else if (name == "LIST") {
		string aaa = read_str(f, 4);
		msg_write(format("list type: %s", aaa));
		while (f->get_pos() < after_pos - 3) {
			read_chunk(f);
		}
	} else if (name == "SMPL") {
		sample_offset = f->get_pos();
		sample_count = l / 2;
	} else if (name == "SHDR") {
		while (f->get_pos() < after_pos - 3) {
			sfSample s;
			read_sample_header(f, s);
			if (s.name == "EOS")
				break;
			samples.add(s);
		}
	} else if (sa_contains({"INAM", "IENG", "IPRD", "ICOP", "ICRD", "ICMT", "ISFT", "ISNG"}, name)) {
		string t = read_str(f, l);
		if (name == "INAM")
			song->add_tag("title", t);
		else if (name == "IENG")
			song->add_tag("engineer", t);
		else if (name == "IPRD")
			song->add_tag("product", t);
		else if (name == "ICOP")
			song->add_tag("copyright", t);
		else if (name == "ICRD")
			song->add_tag("date", t);
		else if (name == "ICMT")
			song->add_tag("comment", t);
		else if (name == "ISNG")
			song->add_tag("engine", t);
		else if (name == "ISFT")
			song->add_tag("software", t);
	} else if (sa_contains({"IFIL"}, name)) {
		// ignore
	} else if (name == "PHDR") {
		while (f->get_pos() < after_pos - 3) {
			sfPresetHeader p;
			p.name = read_str(f, 20);
			p.preset = f->read_word();
			p.bank = f->read_word();
			p.bag_index = f->read_word();
			p.library = f->read_int();
			p.genre = f->read_int();
			p.morphology = f->read_int();
			if (p.name == "EOP")
				break;
			presets.add(p);
			od->info(format("preset: %s  %d %d bag=%d", p.name, p.preset, p.bank, p.bag_index));
		}
	} else if (name == "INST") {
		while (f->get_pos() < after_pos - 3) {
			sfInstrument i;
			i.name = read_str(f, 20);
			i.bag_index = f->read_word();
			if (i.name == "EOI")
				break;
			instruments.add(i);
			od->info(format("instrument: %s   bag=%d", i.name, i.bag_index));
		}
	} else {
		string t;
		t.resize(l);
		f->read_buffer(t);
		msg_write(t.hex());
	}

	f->set_pos(after_pos);


	msg_left();
}

void FormatSoundFont2::read_sample_header(File *f, FormatSoundFont2::sfSample &s) {
	char temp[21];
	f->read_buffer(temp, 20);
	s.name = temp;
	s.start = f->read_int();
	s.end = f->read_int();
	s.start_loop = f->read_int();
	s.end_loop = f->read_int();
	s.sample_rate = f->read_int();
	s.original_key = f->read_byte();
	s.correction = f->read_char();
	s.sample_link = f->read_word();
	s.sample_type = f->read_word();
}

void FormatSoundFont2::read_samples(File *f) {
	int samples_all = 0;
	int samples_read = 0;
	for (auto &s : samples)
		samples_all += s.end - s.start;
	for (auto &s : samples) {
		//s.print();
		if ((s.sample_type & 0x8000) != 0) {
			msg_write("rom");
			continue;
		}
		if ((s.start < 0) or (s.start >= sample_count))
			throw Exception(format("invalid sample start: %d   [0, %d)", s.start, sample_count));
		if ((s.end < 0) or (s.end >= sample_count))
			throw Exception(format("invalid sample end: %d   [0, %d)", s.end, sample_count));

		f->set_pos(sample_offset + s.start*2);
		AudioBuffer buf;
		int num_samples = s.end - s.start;
		if (num_samples < 0)
			throw Exception(format("negative sample size: %d - %d", s.start, s.end));
		char *data = new char[num_samples*2];
		f->read_buffer(data, num_samples*2);
		buf.resize(num_samples);
		buf.import(data, 1, SampleFormat::SAMPLE_FORMAT_16, num_samples);// / 2);
		Sample *sample = song->create_sample_audio(s.name, buf);
		delete data;

		sample->tags.add(Tag("pitch", i2s(s.original_key)));
		sample->tags.add(Tag("start-loop", i2s(s.start_loop)));
		sample->tags.add(Tag("end-loop", i2s(s.end_loop)));
		sample->tags.add(Tag("correction", i2s(s.correction)));

		samples_read += num_samples;
		od->set(float(samples_read) / (float)samples_all);
	}
}
