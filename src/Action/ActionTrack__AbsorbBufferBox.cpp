/*
 * ActionTrack__AbsorbBufferBox.cpp
 *
 *  Created on: 24.03.2012
 *      Author: michi
 */

#include "ActionTrack__AbsorbBufferBox.h"
#include "../Data/AudioFile.h"

ActionTrack__AbsorbBufferBox::ActionTrack__AbsorbBufferBox(int _track_no, int _dest, int _src)
{
	track_no = _track_no;
	dest = _dest;
	src = _src;
}

ActionTrack__AbsorbBufferBox::~ActionTrack__AbsorbBufferBox()
{
}

void *ActionTrack__AbsorbBufferBox::execute(Data *d)
{
	AudioFile *a = dynamic_cast<AudioFile*>(d);
	Track &t = a->track[track_no];


	BufferBox &b_src  = t.buffer[src];
	BufferBox &b_dest = t.buffer[dest];
	int new_size = b_src.offset + b_src.num - b_dest.offset;
	if (new_size > b_dest.num)
		b_dest.resize(new_size);

	b_dest.set(b_src, b_src.offset - b_dest.offset, 1.0f);

	t.buffer.erase(src);

	return NULL;
}



void ActionTrack__AbsorbBufferBox::redo(Data *d)
{
	execute(d);
}



void ActionTrack__AbsorbBufferBox::undo(Data *d)
{
	AudioFile *a = dynamic_cast<AudioFile*>(d);
	Track &t = a->track[track_no];

	msg_todo("absorb undo...");
}


