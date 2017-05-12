/*
 * Tsunami.h
 *
 *  Created on: 13.08.2014
 *      Author: michi
 */

#ifndef TSUNAMI_H_
#define TSUNAMI_H_

#include "lib/hui/hui.h"

extern const string AppName;
extern const string AppVersion;


class Song;
class PluginManager;
class Slider;
class Log;
class AudioInput;
class DeviceManager;
class AudioView;
class Storage;
class Clipboard;
class TsunamiWindow;

class Tsunami : public hui::Application
{
public:
	Tsunami();
	virtual ~Tsunami();

	virtual bool onStartup(const Array<string> &arg);

	bool handleCLIArguments(const Array<string> &arg);
	void loadKeyCodes();

	void createWindow();
	bool allowTermination();

	TsunamiWindow *win;
	hui::Window *_win;
	AudioView *_view;


	Song *song;

	Storage *storage;

	Log *log;

	DeviceManager *device_manager;

	PluginManager *plugin_manager;
	Clipboard *clipboard;
};


extern Tsunami *tsunami;

#endif /* TSUNAMI_H_ */
