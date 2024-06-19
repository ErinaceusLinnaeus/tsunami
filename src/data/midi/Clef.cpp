/*
 * Clef.cpp
 *
 *  Created on: 11.03.2016
 *      Author: michi
 */

#include "Clef.h"
#include "Scale.h"
#include "MidiData.h"


namespace tsunami {

const Clef Clef::_TREBLE(Clef::Type::TREBLE, u8"\U0001d11e", 5*7-5);
const Clef Clef::_TREBLE_8(Clef::Type::TREBLE_8, u8"\U0001d120", 4*7-5);
const Clef Clef::_BASS(Clef::Type::BASS, u8"\U0001d122", 3*7-3);
const Clef Clef::_BASS_8(Clef::Type::BASS_8, u8"\U0001d124", 2*7-3);
const Clef Clef::_DRUMS(Clef::Type::DRUMS, u8"\U0001d125", 0);

struct DrumClefPosition
{
	int pitch, clef_pos;
	NoteModifier modifier;
};
const int NUM_DRUM_CLEF_POSITIONS = 19;
const DrumClefPosition DrumClefPositions[NUM_DRUM_CLEF_POSITIONS] =
{
	// drums
	{DrumPitch::BASS, 1, NoteModifier::NONE}, // "F"
	{DrumPitch::BASS_ACCOUSTIC, 1, NoteModifier::NONE},
	{DrumPitch::TOM_FLOOR_LOW, 3, NoteModifier::NONE}, // "A"
	{DrumPitch::TOM_FLOOR_HI, 3, NoteModifier::NONE},
	{DrumPitch::TOM_LOW, 6, NoteModifier::NONE}, // "D"
	{DrumPitch::TOM_LOW_MID, 6, NoteModifier::NONE},
	{DrumPitch::TOM_HI_MID, 7, NoteModifier::NONE}, // "E"
	{DrumPitch::TOM_HI, 7, NoteModifier::NONE},
	{DrumPitch::SNARE, 5, NoteModifier::NONE}, // "C"
	{DrumPitch::SNARE_ELECTRONIC, 5, NoteModifier::NONE},

	// cymbals
	{DrumPitch::HIHAT_PEDAL, -1, NoteModifier::NONE}, // "D"
	{DrumPitch::HIHAT_OPEN, 9, NoteModifier::SHARP}, // "G"
	{DrumPitch::HIHAT_CLOSED, 9, NoteModifier::FLAT},
	{DrumPitch::RIDE_1, 8, NoteModifier::NONE}, // "F"
	{DrumPitch::RIDE_2, 8, NoteModifier::NONE},
	{DrumPitch::BELL_RIDE, 8, NoteModifier::FLAT},
	{DrumPitch::CRASH_1, 10, NoteModifier::NONE}, // "A"
	{DrumPitch::CRASH_2, 11, NoteModifier::NONE}, // "B"
	{DrumPitch::CHINESE, 13, NoteModifier::NONE}, // "D"
};

/*
		DrumPitch::SIDE_STICK = 37,
		DrumPitch::CLAP = 39,
		DrumPitch::TAMBOURINE = 54,
		DrumPitch::SPLASH = 55,
		DrumPitch::COWBELL = 56,
		DrumPitch::VIBRASLASH = 58,
		DrumPitch::BONGO_HI = 60,
		DrumPitch::BONGO_LOW = 61,
*/


Clef::Clef(int _type, const string &_symbol, int _offset)
{
	type = _type;
	symbol = _symbol;
	offset = _offset;
}

// TODO account for scale!
int Clef::pitch_to_position(int pitch, const Scale &s, NoteModifier &modifier) const
{
	if (type == Type::DRUMS){
		for (int i=0; i<NUM_DRUM_CLEF_POSITIONS; i++)
			if (pitch == DrumClefPositions[i].pitch){
				modifier = DrumClefPositions[i].modifier;
				return DrumClefPositions[i].clef_pos;
			}

		modifier = NoteModifier::SHARP;
		return 13;
	}
	int octave = pitch_get_octave(pitch);
	int rel = pitch_to_rel(pitch);

	const int pp[12] = {0,0,1,2,2,3,3,4,4,5,6,6};
	const NoteModifier ss[12] = {NoteModifier::NONE,NoteModifier::SHARP,NoteModifier::NONE,NoteModifier::FLAT,NoteModifier::NONE,NoteModifier::NONE,NoteModifier::SHARP,NoteModifier::NONE,NoteModifier::SHARP,NoteModifier::NONE,NoteModifier::FLAT,NoteModifier::NONE};

	modifier = ss[rel];
	return pp[rel] + 7 * octave - offset;
}

int Clef::position_to_pitch(int pos, const Scale &s, NoteModifier mod) const
{
	if (type == Type::DRUMS){
		for (int i=0; i<NUM_DRUM_CLEF_POSITIONS; i++)
			if ((pos == DrumClefPositions[i].clef_pos) and (mod == DrumClefPositions[i].modifier))
				return DrumClefPositions[i].pitch;
		return 70;//DrumPitch::BASS;
	}
	return s.transform_out(position_to_uniclef(pos), mod);
}

int Clef::position_to_uniclef(int pos) const
{
	return pos + offset;
}

}
