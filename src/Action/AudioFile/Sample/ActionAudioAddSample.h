/*
 * ActionAudioAddSample.h
 *
 *  Created on: 15.07.2013
 *      Author: michi
 */

#ifndef ACTIONAUDIOADDSAMPLE_H_
#define ACTIONAUDIOADDSAMPLE_H_

#include "../../Action.h"

class BufferBox;
class Sample;

class ActionAudioAddSample : public Action
{
public:
	ActionAudioAddSample(const string &name, BufferBox &buf);
	virtual ~ActionAudioAddSample();

	virtual void *execute(Data *d);
	virtual void undo(Data *d);

private:
	Sample *sample;
};

#endif /* ACTIONAUDIOADDSAMPLE_H_ */
