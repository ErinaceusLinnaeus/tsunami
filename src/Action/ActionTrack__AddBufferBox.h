/*
 * ActionTrack__AddBufferBox.h
 *
 *  Created on: 24.03.2012
 *      Author: michi
 */

#ifndef ACTIONTRACK__ADDBUFFERBOX_H_
#define ACTIONTRACK__ADDBUFFERBOX_H_

#include "Action.h"

class ActionTrack__AddBufferBox : public Action
{
public:
	ActionTrack__AddBufferBox(int _track_no, int index, int _pos, int _length);
	virtual ~ActionTrack__AddBufferBox();

	void *execute(Data *d);
	void undo(Data *d);
	void redo(Data *d);

private:
	int track_no;
	int pos, length, index;
};

#endif /* ACTIONTRACK__ADDBUFFERBOX_H_ */
