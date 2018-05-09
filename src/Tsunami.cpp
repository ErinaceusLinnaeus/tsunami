/*
 * Tsunami.cpp
 *
 *  Created on: 13.08.2014
 *      Author: michi
 */

#include "Tsunami.h"

#include "Device/DeviceManager.h"
#include "Device/OutputStream.h"
#include "Module/SignalChain.h"
#include "Module/Audio/SongRenderer.h"
#include "Module/Audio/PeakMeter.h"
#include "TsunamiWindow.h"
#include "Session.h"
#include "Storage/Storage.h"
#include "Stuff/Log.h"
#include "Stuff/Clipboard.h"
#include "Stuff/PerformanceMonitor.h"
#include "Stuff/BackupManager.h"
#include "Plugins/PluginManager.h"
#include "Plugins/TsunamiPlugin.h"


const string AppName = "Tsunami";
const string AppVersion = "0.7.10.0";
const string AppNickname = "absolute 2er0";

Tsunami *tsunami = NULL;

bool ugly_hack_slow = false;

Tsunami::Tsunami() :
	hui::Application("tsunami", "English", hui::FLAG_LOAD_RESOURCE)
{
	device_manager = NULL;
	log = NULL;
	clipboard = NULL;
	plugin_manager = NULL;
	perf_mon = NULL;

	setProperty("name", AppName);
	setProperty("version", AppVersion + " \"" + AppNickname + "\"");
	setProperty("comment", _("Editor for audio files"));
	setProperty("website", "http://michi.is-a-geek.org/software");
	setProperty("copyright", "© 2007-2018 by Michael Ankele");
	setProperty("author", "Michael Ankele <michi@lupina.de>");
	printf("aaaaaaaaaaaaa\n");
}

Tsunami::~Tsunami()
{
	delete(device_manager);
	delete(plugin_manager);
	delete(clipboard);
	delete(log);
	delete(perf_mon);
}

void Tsunami::onEnd()
{
	while (sessions.num > 0){
		Session *s = sessions.pop();
		delete s;
	}
}

bool Tsunami::onStartup(const Array<string> &_arg)
{
	printf("start\n");
	Array<string> arg = _arg;
	tsunami = this;

	perf_mon = new PerformanceMonitor;

	log = new Log;

	clipboard = new Clipboard;

	Session::GLOBAL = new Session(log, NULL, NULL, perf_mon);

	Session::GLOBAL->i(AppName + " " + AppVersion + " \"" + AppNickname + "\"");
	Session::GLOBAL->i(_("  ...don't worry. Everything will be fine!"));

	device_manager = new DeviceManager;
	Session::GLOBAL->device_manager = device_manager;

	// create (link) PluginManager after all other components are ready
	plugin_manager = new PluginManager;
	Session::GLOBAL->plugin_manager = plugin_manager;

	plugin_manager->LinkAppScriptData();

	printf("aaaaaaaaaaaaa2\n");

	if (handleArguments(arg))
		return false;

	printf("aaaaaaaaaaaaa3\n");

	// ok, full window mode

	BackupManager::check_old_files(Session::GLOBAL);


	// create a window and load file
	if (sessions.num == 0){
		Session *session = createSession();
		session->win->show();
	}
	printf("aaaaaaaaaaaaa4\n");

	return true;
}

bool Tsunami::handleArguments(Array<string> &args)
{
	if (args.num < 2)
		return false;
	Session *session = Session::GLOBAL;

	for (int i=1; i<args.num; i++){

	if (args[i] == "--help"){
		session->i(AppName + " " + AppVersion);
		session->i("--help");
		session->i("--info <FILE>");
		session->i("--export <FILE_IN> <FILE_OUT>");
		session->i("--execute <PLUGIN_NAME> [ARGUMENTS]");
		session->i("--chain <SIGNAL_CHAIN_FILE>");
		return true;
	}else if (args[i] == "--info"){
		Song* song = new Song(session);
		session->song = song;
		if (args.num < i+2){
			session->e(_("call: tsunami --info <File>"));
		}else if (session->storage->load_ex(song, args[i+1], true)){
			msg_write(format("sample-rate: %d", song->sample_rate));
			msg_write(format("samples: %d", song->getRange().length));
			msg_write("length: " + song->get_time_str(song->getRange().length));
			msg_write(format("tracks: %d", song->tracks.num));
			int n = 0;
			for (Track *t: song->tracks)
				n += t->samples.num;
			msg_write(format("refs: %d / %d", n, song->samples.num));
			for (Tag &t: song->tags)
				msg_write("tag: " + t.key + " = " + t.value);
		}
		delete(song);
		return true;
	}else if (args[i] == "--export"){
		Song* song = new Song(session);
		session->song = song;
		if (args.num < i + 3){
			session->e(_("call: tsunami --export <File> <Exportfile>"));
		}else if (session->storage->load(song, args[i+1])){
			session->storage->save(song, args[i+2]);
		}
		delete(song);
		return true;
	}else if (args[1] == "--execute"){
		if (args.num < i + 2){
			session->e(_("call: tsunami --execute <PLUGIN_NAME> [ARGUMENTS]"));
			return true;
		}
		if (session == Session::GLOBAL)
			session = createSession();
		session->win->hide();
		session->die_on_plugin_stop = true;
		session->executeTsunamiPlugin(args[i+1]);
		i ++;
		//return false;
	}else if (args[i] == "--chain"){
		if (args.num < i + 2){
			session->e(_("call: tsunami --chain <SIGNAL_CHAIN>"));
			return true;
		}
		if (session == Session::GLOBAL)
			session = createSession();
		session->win->show();
		session->signal_chain->load(args[i+1]);
		i ++;
	}else if (args[i] == "--slow"){
		ugly_hack_slow = true;
	}else if (args[i].head(2) == "--"){
		session->e(_("unknown command: ") + args[i]);
		return true;
	}else{
		if (session == Session::GLOBAL)
			session = createSession();
		session->win->show();
		session->storage->load(session->song, args[i]);
	}
	}
	return false;
}

Session* Tsunami::createSession()
{
	Session *session = new Session(log, device_manager, plugin_manager, perf_mon);

	session->song = new Song(session);

	session->signal_chain = SignalChain::create_default(session);
	session->song_renderer = dynamic_cast<SongRenderer*>(session->signal_chain->modules[0]);
	session->peak_meter = dynamic_cast<PeakMeter*>(session->signal_chain->modules[1]);
	session->output_stream = dynamic_cast<OutputStream*>(session->signal_chain->modules[2]);

	session->setWin(new TsunamiWindow(session));
	session->win->auto_delete = true;
	session->win->show();

	sessions.add(session);
	return session;
}

void Tsunami::loadKeyCodes()
{
}

bool Tsunami::allowTermination()
{
	for (auto *s: sessions)
		if (!s->win->allowTermination())
			return false;
	return true;
}

//HUI_EXECUTE(Tsunami);

int hui_main(const Array<string> &args)
{
	printf("xxxx\n");


        Tsunami::_args = args;
        Tsunami *app = new Tsunami;
	printf("aaaaa\n");
        int r = 0;
        if (app->onStartup(args))
                r = app->run();
        delete(app);
        return r;


	return 0;
}

