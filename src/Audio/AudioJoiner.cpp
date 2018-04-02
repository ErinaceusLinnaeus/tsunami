/*
 * AudioJoiner.cpp
 *
 *  Created on: 02.04.2018
 *      Author: michi
 */

#include "AudioJoiner.h"

AudioJoiner::AudioJoiner()
{
	out = new Output(this);
	a = NULL;
	b = NULL;
}

AudioJoiner::~AudioJoiner()
{
}

int AudioJoiner::Output::read(AudioBuffer& buf)
{
	if (joiner->a and joiner->b){
		int ra = joiner->a->read(buf);
		AudioBuffer buf_b;
		buf_b.resize(buf.length);
		int rb = joiner->b->read(buf_b);
		buf.add(buf_b, 0, 1, 0);
		return max(ra, rb);
	}else if (joiner->a){
		return joiner->a->read(buf);
	}else if (joiner->b){
		return joiner->b->read(buf);
	}
	return buf.length;
}

AudioJoiner::Output::Output(AudioJoiner *j)
{
	joiner = j;
}

void AudioJoiner::Output::reset()
{
	if (joiner->a)
		joiner->a->reset();
	if (joiner->b)
		joiner->b->reset();
}

int AudioJoiner::Output::get_pos(int delta)
{
	if (joiner->a)
		return joiner->a->get_pos(delta);
	if (joiner->b)
		return joiner->b->get_pos(delta);
	return 0;
}

void AudioJoiner::set_source_a(AudioPort* _a)
{
	a = _a;
}

void AudioJoiner::set_source_b(AudioPort* _b)
{
	b = _b;
}
