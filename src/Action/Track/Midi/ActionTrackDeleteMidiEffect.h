/*
 * ActionTrackDeleteMidiEffect.h
 *
 *  Created on: 23.09.2014
 *      Author: michi
 */

#pragma once

#include "../../../Module/Midi/MidiEffect.h"
#include "../../Action.h"
class Track;

class ActionTrackDeleteMidiEffect: public Action {
public:
	ActionTrackDeleteMidiEffect(Track *t, int index);

	string name() const override { return ":##:delete midi fx"; }

	void *execute(Data *d) override;
	void undo(Data *d) override;

private:
	shared<MidiEffect> effect;
	Track *track;
	int index;
};
