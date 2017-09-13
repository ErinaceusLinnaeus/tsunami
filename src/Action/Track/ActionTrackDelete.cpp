/*
 * ActionTrackDelete.cpp
 *
 *  Created on: 09.04.2012
 *      Author: michi
 */

#include "ActionTrackDelete.h"
#include "Sample/ActionTrackDeleteSample.h"
#include "ActionTrack__DeleteEmpty.h"
#include <assert.h>
#include "Buffer/ActionTrack__DeleteBuffer.h"

ActionTrackDelete::ActionTrackDelete(Track *_track)
{
	track = _track;
}

void ActionTrackDelete::build(Data *d)
{
	//Song *s = dynamic_cast<Song*>(d);

	// delete buffers
	foreachi(TrackLayer &l, track->layers, li)
		for (int i=l.buffers.num-1; i>=0; i--)
			addSubAction(new ActionTrack__DeleteBuffer(track, li, i), d);

	// delete samples
	for (int i=track->samples.num-1; i>=0; i--)
		addSubAction(new ActionTrackDeleteSample(track->samples[i]), d);

	// delete the track itself
	addSubAction(new ActionTrack__DeleteEmpty(track), d);
}
