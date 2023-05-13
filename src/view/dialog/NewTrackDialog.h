//
// Created by michi on 13.05.23.
//
#ifndef SRC_VIEW_DIALOG_NEWTRACKDIALOG_H
#define SRC_VIEW_DIALOG_NEWTRACKDIALOG_H


#include "../../lib/hui/hui.h"
#include "../../data/base.h"
#include "../../data/midi/Instrument.h"

class Song;
class Session;
enum class SignalType;

class NewTrackDialog : public hui::Dialog {
public:
	NewTrackDialog(hui::Window *parent, Session *session);

	SignalType type;
	Session *session;
	Instrument instrument;
	Array<Instrument> instrument_list;

	void load_data();
	void apply_data();
	void update_strings();

	void on_type(SignalType t);
	void on_instrument();
	void on_beats();
	void on_complex();
	void on_pattern();
	void on_divisor();
	void on_ok();
	void on_metronome();
	void on_edit_tuning();
};

#endif //SRC_VIEW_DIALOG_NEWTRACKDIALOG_H
