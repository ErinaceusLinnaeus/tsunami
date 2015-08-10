/*
 * ActionTrackAddBar.h
 *
 *  Created on: 15.12.2012
 *      Author: michi
 */

#ifndef ACTIONTRACKADDBAR_H_
#define ACTIONTRACKADDBAR_H_

#include "../../Action.h"
#include "../../../Data/Track.h"

class ActionTrackAddBar: public Action
{
public:
	ActionTrackAddBar(Track *t, int index, BarPattern &Bar, bool affect_midi);
	virtual ~ActionTrackAddBar();

	virtual void *execute(Data *d);
	virtual void undo(Data *d);

private:
	int index;
	BarPattern bar;
	int track_no;
	bool affect_midi;
};

#endif /* ACTIONTRACKADDBAR_H_ */
