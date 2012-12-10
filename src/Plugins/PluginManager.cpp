/*
 * PluginManager.cpp
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#include "PluginManager.h"
#include "../Tsunami.h"
#include "FastFourierTransform.h"
#include "../View/Dialog/Slider.h"
#include "Plugin.h"
#include "Effect.h"

PluginManager::PluginManager()
{
}

PluginManager::~PluginManager()
{
}


BufferBox AudioFileRender(AudioFile *a, const Range &r)
{	return tsunami->renderer->RenderAudioFile(a, r);	}

void GlobalPutFavoriteBarFixed(CHuiWindow *win, int x, int y, int w)
{	tsunami->plugins->PutFavoriteBarFixed(win, x, y, w);	}

void GlobalPutFavoriteBarSizable(CHuiWindow *win, const string &root_id, int x, int y)
{	tsunami->plugins->PutFavoriteBarSizable(win, root_id, x, y);	}

void GlobalPutCommandBarFixed(CHuiWindow *win, int x, int y, int w)
{	tsunami->plugins->PutCommandBarFixed(win, x, y, w);	}

void GlobalPutCommandBarSizable(CHuiWindow *win, const string &root_id, int x, int y)
{	tsunami->plugins->PutCommandBarSizable(win, root_id, x, y);	}

Array<Slider*> global_slider;

void GlobalAddSlider(CHuiWindow *win, const string &id_slider, const string &id_edit, float v_min, float v_max, float factor, hui_callback *func, float value)
{	global_slider.add(new Slider(win, id_slider, id_edit, v_min, v_max, factor, func, value));	}

void GlobalSliderSet(CHuiWindow *win, const string &id, float value)
{
	foreach(Slider *s, global_slider)
		if (s->Match(id))
				s->Set(value);
}

float GlobalSliderGet(CHuiWindow *win, const string &id)
{
	foreach(Slider *s, global_slider)
		if (s->Match(id))
				return s->Get();
	return 0;
}

void GlobalRemoveSliders(CHuiWindow *win)
{
	foreach(Slider *s, global_slider)
		delete(s);
	global_slider.clear();
}

CHuiWindow *GlobalMainWin = NULL;

void PluginManager::LinkAppScriptData()
{
	msg_db_r("LinkAppScriptData", 2);
	ScriptDirectory = "";

	// api definition
	ScriptResetSemiExternalData();
	GlobalMainWin = dynamic_cast<CHuiWindow*>(tsunami);
	ScriptLinkSemiExternalVar("MainWin",		&GlobalMainWin);
	ScriptLinkSemiExternalVar("cur_audio",		&tsunami->cur_audio);
	ScriptLinkSemiExternalVar("audio",			&tsunami->audio);
	ScriptLinkSemiExternalVar("CaptureBuf",		&tsunami->input->CaptureBuf);
	ScriptLinkSemiExternalVar("CaptureAddData",	&tsunami->input->CaptureAddData);
	ScriptLinkSemiExternalVar("CapturePreviewBuf",&tsunami->input->CapturePreviewBuf);
	ScriptLinkSemiExternalVar("input",			&tsunami->input);
	ScriptLinkSemiExternalVar("output",			&tsunami->output);
/*	ScriptLinkSemiExternalFunc("CreateNewAudioFile",(void*)&CreateNewAudioFile);
	ScriptLinkSemiExternalFunc("AddEmptyTrack",	(void*)&AddEmptyTrack);
	ScriptLinkSemiExternalFunc("DeleteTrack",	(void*)&DeleteTrack);
	ScriptLinkSemiExternalFunc("AddEmptySubTrack",(void*)&AddEmptySubTrack);*/
	ScriptLinkSemiExternalFunc("AudioFile.GetNextBeat",	(void*)&AudioFile::GetNextBeat);
	ScriptLinkSemiExternalFunc("Track.GetBuffers",	(void*)&Track::GetBuffers);
	ScriptLinkSemiExternalFunc("Track.ReadBuffers",	(void*)&Track::ReadBuffers);
	ScriptLinkSemiExternalFunc("BufferBox.clear",(void*)&BufferBox::clear);
	ScriptLinkSemiExternalFunc("BufferBox.__assign__",(void*)&BufferBox::__assign__);
	ScriptLinkSemiExternalFunc("BufferBox.add_click",(void*)&BufferBox::add_click);
	ScriptLinkSemiExternalFunc("fft_c2c",		(void*)&FastFourierTransform::fft_c2c);
	ScriptLinkSemiExternalFunc("fft_r2c",		(void*)&FastFourierTransform::fft_r2c);
	ScriptLinkSemiExternalFunc("fft_c2r_inv",	(void*)&FastFourierTransform::fft_c2r_inv);
	ScriptLinkSemiExternalFunc("fft_i2c",		(void*)&FastFourierTransform::fft_i2c);
	ScriptLinkSemiExternalFunc("fft_c2i_inv",	(void*)&FastFourierTransform::fft_c2i_inv);
	/*ScriptLinkSemiExternalFunc("ProgressStart",	(void*)&ProgressStart);
	ScriptLinkSemiExternalFunc("ProgressEnd",	(void*)&ProgressEnd);
	ScriptLinkSemiExternalFunc("Progress",		(void*)&ProgressStatus);*/
	ScriptLinkSemiExternalFunc("PutFavoriteBarFixed",	(void*)&GlobalPutFavoriteBarFixed);
	ScriptLinkSemiExternalFunc("PutFavoriteBarSizable",	(void*)&GlobalPutFavoriteBarSizable);
	ScriptLinkSemiExternalFunc("PutCommandBarFixed",	(void*)&GlobalPutCommandBarFixed);
	ScriptLinkSemiExternalFunc("PutCommandBarSizable",	(void*)&GlobalPutCommandBarSizable);
	ScriptLinkSemiExternalFunc("AddSlider",		(void*)&GlobalAddSlider);
	ScriptLinkSemiExternalFunc("SliderSet",		(void*)&GlobalSliderSet);
	ScriptLinkSemiExternalFunc("SliderGet",		(void*)&GlobalSliderGet);
	ScriptLinkSemiExternalFunc("RemoveSliders",	(void*)&GlobalRemoveSliders);
	ScriptLinkSemiExternalFunc("AudioFileRender",		(void*)&AudioFileRender);
	ScriptLinkSemiExternalFunc("AudioOutput.Play",	(void*)&AudioOutput::Play);
	ScriptLinkSemiExternalFunc("AudioOutput.PlayGenerated",	(void*)&AudioOutput::PlayGenerated);
	ScriptLinkSemiExternalFunc("AudioOutput.Stop",	(void*)&AudioOutput::Stop);
	ScriptLinkSemiExternalFunc("AudioOutput.IsPlaying",	(void*)&AudioOutput::IsPlaying);
	ScriptLinkSemiExternalFunc("AudioOutput.GetPos",	(void*)&AudioOutput::GetPos);
	ScriptLinkSemiExternalFunc("AudioOutput.GetSampleRate",	(void*)&AudioOutput::GetSampleRate);
	ScriptLinkSemiExternalFunc("AudioOutput.GetVolume",	(void*)&AudioOutput::GetVolume);
	ScriptLinkSemiExternalFunc("AudioOutput.SetVolume",	(void*)&AudioOutput::SetVolume);
	ScriptLinkSemiExternalFunc("AudioInput.Start",	(void*)&AudioInput::Start);
	ScriptLinkSemiExternalFunc("AudioInput.Stop",	(void*)&AudioInput::Stop);
	msg_db_l(2);
}

void PluginManager::OnMenuExecutePlugin()
{
	int n = s2i(HuiGetEvent()->id.substr(strlen("execute_plugin_"), -1));

	if ((n >= 0) && (n < plugin_file.num))
		ExecutePlugin(plugin_file[n].filename);
}

void get_plugin_file_data(PluginManager::PluginFile &pf)
{
	pf.image = "";
	SilentFiles = true;
	string content = FileRead(pf.filename);
	int p = content.find("// Image = hui:");
	if (p >= 0)
		pf.image = content.substr(p + 11, content.find("\n") - p - 11);
	SilentFiles = false;
}

void find_plugins_in_dir(const string &dir, PluginManager *pm, CHuiMenu *m)
{
	Array<DirEntry> list = dir_search(HuiAppDirectoryStatic + "Plugins/" + dir, "*.kaba", false);
	foreach(DirEntry &e, list){
		PluginManager::PluginFile pf;
		pf.name = e.name.replace(".kaba", "");
		pf.filename = HuiAppDirectoryStatic + "Plugins/" + dir + e.name;
		get_plugin_file_data(pf);
		m->AddItemImage(pf.name, pf.image, format("execute_plugin_%d", pm->plugin_file.num));
		pm->plugin_file.add(pf);
	}
}

void PluginManager::AddPluginsToMenu()
{
	msg_db_r("AddPluginsToMenu", 2);
	ScriptInit();

	CHuiMenu *m = tsunami->GetMenu()->GetSubMenuByID("menu_plugins");

	// "Buffer"..
	m->AddSeparator();
	m->AddItem(_("Auf Audiopuffer"), "plugin_on_file");
	m->EnableItem("plugin_on_file", false);

	CHuiMenu *sm = new CHuiMenu();
	m->AddSubMenu(_("Kan&ale"), "", sm);
	find_plugins_in_dir("Buffer/Channels/", this, sm);
	sm = new CHuiMenu();
	m->AddSubMenu(_("Dynamik"), "", sm);
	find_plugins_in_dir("Buffer/Dynamics/", this, sm);
	sm = new CHuiMenu();
	m->AddSubMenu(_("Echo"), "", sm);
	find_plugins_in_dir("Buffer/Echo/", this, sm);
	sm = new CHuiMenu();
	m->AddSubMenu(_("Tonh&ohe"), "", sm);
	find_plugins_in_dir("Buffer/Pitch/", this, sm);
	sm = new CHuiMenu();
	m->AddSubMenu(_("Klang"), "", sm);
	find_plugins_in_dir("Buffer/Sound/", this, sm);
	sm = new CHuiMenu();
	m->AddSubMenu(_("Synthese"), "", sm);
	find_plugins_in_dir("Buffer/Synthesizer/", this, sm);

	// "All"
	m->AddSeparator();
	m->AddItem(_("Auf alles"), "plugin_on_audio");
	m->EnableItem("plugin_on_audio", false);
	find_plugins_in_dir("All/", this, m);

	// rest
	m->AddSeparator();
	m->AddItem(_("Eigenst&andig"), "plugin_other");
	m->EnableItem("plugin_other", false);
	find_plugins_in_dir("Independent/", this, m);

	// Events
	for (int i=0;i<plugin_file.num;i++)
		tsunami->EventM(format("execute_plugin_%d", i), this, (void(HuiEventHandler::*)())&PluginManager::OnMenuExecutePlugin);
	msg_db_l(2);
}

void PluginManager::InitPluginData()
{
	msg_db_r("InitPluginData", 2);
	msg_db_l(2);
}

void PluginManager::FinishPluginData()
{
	msg_db_r("FinishPluginData", 2);
	tsunami->ForceRedraw();
	msg_db_l(2);
}

void PluginManager::OnFavoriteName()
{
	bool enabled = HuiCurWindow->GetString("").num > 0;
	HuiCurWindow->Enable("favorite_save", enabled);
	HuiCurWindow->Enable("favorite_delete", enabled);
}

void PluginManager::OnFavoriteList()
{
	int n = HuiCurWindow->GetInt("");
	cur_plugin->ResetData();
	if (n == 0){
		HuiCurWindow->SetString("favorite_name", "");
		HuiCurWindow->Enable("favorite_save", false);
		HuiCurWindow->Enable("favorite_delete", false);
	}else{
		LoadPluginDataFromFile(PluginFavoriteName[n - 1]);
		HuiCurWindow->SetString("favorite_name", PluginFavoriteName[n - 1]);
		HuiCurWindow->Enable("favorite_delete", true);
	}
	cur_plugin->DataToDialog();
}

void PluginManager::OnFavoriteSave()
{
	string name = HuiCurWindow->GetString("favorite_name");
	WritePluginDataToFile(name);
	PluginFavoriteName.add(name);
	HuiCurWindow->AddString("favorite_list", name);
	HuiCurWindow->SetInt("favorite_list", PluginFavoriteName.num);
}

void PluginManager::OnFavoriteDelete()
{}

void PluginManager::InitFavorites(CHuiWindow *win)
{
	msg_db_r("InitFavorites", 1);
	PluginFavoriteName.clear();


	win->Enable("favorite_save", false);
	win->Enable("favorite_delete", false);

	string init = cur_plugin->s->pre_script->Filename.basename() + "___";

	dir_create(HuiAppDirectory + "Plugins/Favorites");
	Array<DirEntry> list = dir_search(HuiAppDirectory + "Plugins/Favorites", "*", false);
	foreach(DirEntry &e, list){
		if (e.name.find(init) < 0)
			continue;
		PluginFavoriteName.add(e.name.substr(init.num, -1));
		win->AddString("favorite_list", PluginFavoriteName.back());
	}

	win->EventM("favorite_name", this, (void(HuiEventHandler::*)())&PluginManager::OnFavoriteName);
	win->EventM("favorite_save", this, (void(HuiEventHandler::*)())&PluginManager::OnFavoriteSave);
	win->EventM("favorite_delete", this, (void(HuiEventHandler::*)())&PluginManager::OnFavoriteDelete);
	win->EventM("favorite_list", this, (void(HuiEventHandler::*)())&PluginManager::OnFavoriteList);
	msg_db_l(1);
}

void PluginManager::PutFavoriteBarFixed(CHuiWindow *win, int x, int y, int w)
{
	msg_db_r("PutFavoriteBarFixed", 1);
	w -= 10;
	win->AddComboBox("", x, y, w / 2 - 35, 25, "favorite_list");
	win->AddEdit("", x + w / 2 - 30, y, w / 2 - 30, 25, "favorite_name");
	win->AddButton("", x + w - 55, y, 25, 25, "favorite_save");
	win->SetImage("favorite_save", "hui:save");
	win->AddButton("", x + w - 25, y, 25, 25, "favorite_delete");
	win->SetImage("favorite_delete", "hui:delete");

	InitFavorites(win);
	msg_db_l(1);
}

void PluginManager::PutFavoriteBarSizable(CHuiWindow *win, const string &root_id, int x, int y)
{
	msg_db_r("PutFavoriteBarSizable", 1);
	win->SetTarget(root_id, 0);
	win->AddControlTable("!noexpandy", x, y, 4, 1, "favorite_table");
	win->SetTarget("favorite_table", 0);
	win->AddComboBox("", 0, 0, 0, 0, "favorite_list");
	win->AddEdit("", 1, 0, 0, 0, "favorite_name");
	win->AddButton("", 2, 0, 0, 0, "favorite_save");
	win->SetImage("favorite_save", "hui:save");
	win->AddButton("", 3, 0, 0, 0, "favorite_delete");
	win->SetImage("favorite_delete", "hui:delete");

	InitFavorites(win);
	msg_db_l(1);
}

void PluginManager::OnPluginFavoriteName()
{
	CHuiWindow *win = HuiGetEvent()->win;
	win->Enable("favorite_save", win->GetString("favorite_name").num > 0);
	win->Enable("favorite_delete", win->GetString("favorite_name").num > 0);
}

void PluginManager::OnPluginFavoriteList()
{
	CHuiWindow *win = HuiGetEvent()->win;
	int n = win->GetInt("");
	cur_plugin->ResetData();
	if (n == 0){
		win->SetString("favorite_name", "");
		win->Enable("favorite_save", false);
		win->Enable("favorite_delete", false);
	}else{
		LoadPluginDataFromFile(PluginFavoriteName[n - 1].c_str());
		win->SetString("favorite_name", PluginFavoriteName[n - 1].c_str());
		win->Enable("favorite_delete", true);
	}
	cur_plugin->DataToDialog();
}

void PluginManager::OnPluginFavoriteSave()
{
	CHuiWindow *win = HuiGetEvent()->win;
	WritePluginDataToFile(win->GetString("favorite_name"));
	PluginFavoriteName.add(win->GetString("favorite_name"));
	win->AddString("favorite_list", win->GetString("favorite_name"));
	win->SetInt("favorite_list", PluginFavoriteName.num);
}

void PluginManager::OnPluginOk()
{
	PluginCancelled = false;
	delete(HuiCurWindow);
}

void PluginManager::OnPluginClose()
{
	PluginCancelled = true;
	delete(HuiCurWindow);
}

void PluginManager::PutCommandBarFixed(CHuiWindow *win, int x, int y, int w)
{
	msg_db_r("PutCommandBarFixed", 1);
	w -= 10;
	int ww = (w - 30) / 3;
	if (ww > 120)
		ww = 120;

	win->AddDefButton(_("OK"),w - ww,y,ww,25,"ok");
	//win->SetImage("ok", "hui:ok");
	win->AddButton(_("Abbrechen"),w - ww*2 - 10,y,ww,25,"cancel");
	//win->SetImage("cancel", "hui:cancel");

	if (PluginAddPreview){
		if (cur_plugin->type == Plugin::TYPE_EFFECT){
			win->AddButton(_("Vorschau"),w - ww * 3 - 20,y,ww,25,"preview");
			win->SetImage("preview", "hui:media-play");
		}
	}
	win->EventM("ok", this, (void(HuiEventHandler::*)())&PluginManager::OnPluginOk);
	win->EventM("preview", this, (void(HuiEventHandler::*)())&PluginManager::OnPluginPreview);
	win->EventM("cancel", this, (void(HuiEventHandler::*)())&PluginManager::OnPluginClose);
	win->EventM("hui:close", this, (void(HuiEventHandler::*)())&PluginManager::OnPluginClose);
	msg_db_l(1);
}

void PluginManager::PutCommandBarSizable(CHuiWindow *win, const string &root_id, int x, int y)
{
	msg_db_r("PutCommandBarSizable", 1);
	win->SetTarget(root_id, 0);
	win->AddControlTable("!noexpandy", x, y, 4, 1, "command_table");
	win->SetTarget("command_table", 0);
	win->AddDefButton(_("OK"), 3, 0, 0, 0, "ok");
	win->SetImage("ok", "hui:ok");
	win->AddButton(_("Abbrechen"), 2, 0, 0, 0, "cancel");
	win->SetImage("cancel", "hui:cancel");
	win->AddText("", 1, 0, 0, 0, "");
	if (PluginAddPreview){
		if (cur_plugin->type == Plugin::TYPE_EFFECT){
			win->AddButton(_("Vorschau"), 0, 0, 0, 0, "preview");
			win->SetImage("preview", "hui:media-play");
		}
	}
	win->EventM("ok", this, (void(HuiEventHandler::*)())&PluginManager::OnPluginOk);
	win->EventM("preview", this, (void(HuiEventHandler::*)())&PluginManager::OnPluginPreview);
	win->EventM("cancel", this, (void(HuiEventHandler::*)())&PluginManager::OnPluginClose);
	win->EventM("hui:close", this, (void(HuiEventHandler::*)())&PluginManager::OnPluginClose);
	msg_db_l(1);
}

void PluginManager::OnPluginPreview()
{
	cur_plugin->Preview();
}

void PluginManager::WritePluginDataToFile(const string &name)
{
	msg_db_r("WritePluginDataToFile", 1);
	Effect fx;
	fx.plugin = cur_plugin;
	fx.ExportData();
	dir_create(HuiAppDirectory + "Plugins/");
	dir_create(HuiAppDirectory + "Plugins/Favorites/");
	CFile *f = CreateFile(HuiAppDirectory + "Plugins/Favorites/" + cur_plugin->s->pre_script->Filename.basename() + "___" + name);
	f->WriteInt(0);
	f->WriteInt(0);
	f->WriteComment("// Data");
	f->WriteInt(fx.param.num);
	foreach(EffectParam &p, fx.param){
		f->WriteStr(p.name);
		f->WriteStr(p.type);
		f->WriteStr(p.value);
	}
	fx.param.clear();
	f->WriteStr("#");
	FileClose(f);
	msg_db_l(1);
}

void PluginManager::LoadPluginDataFromFile(const string &name)
{
	msg_db_r("LoadPluginDataFromFile", 1);
	CFile *f = OpenFile(HuiAppDirectory + "Plugins/Favorites/" + cur_plugin->s->pre_script->Filename.basename() + "___" + name);
	if (!f){
		msg_db_l(1);
		return;
	}
	Effect fx;
	fx.plugin = cur_plugin;

	f->ReadInt();
	f->ReadInt();
	f->ReadComment();
	int num = f->ReadInt();
	fx.param.resize(num);
	foreach(EffectParam &p, fx.param){
		p.name = f->ReadStr();
		p.type = f->ReadStr();
		p.value = f->ReadStr();
	}
	fx.ImportData();
	fx.param.clear();

	FileClose(f);
	msg_db_l(1);
}

void PluginManager::OnUpdate(Observable *o)
{
	if (o == tsunami->progress){
		if (o->GetMessage() == "Cancel")
			tsunami->output->Stop();
	}else if (o == tsunami->output){
		int pos = tsunami->output->GetPos();
		tsunami->progress->Set(_("Vorschau"), (float)(pos - tsunami->output->GetRange().start()) / tsunami->output->GetRange().length());
	}
}

void Plugin::Preview()
{
	Effect fx;
	fx.plugin = this;
	fx.name = filename.basename();
	fx.name = fx.name.substr(0, fx.name.num - 5);
	//msg_write(fx.filename);
	fx.ExportData();
	tsunami->renderer->effect = &fx;


	tsunami->progress->StartCancelable(_("Vorschau"), 0);
	tsunami->plugins->Subscribe(tsunami->progress);
	tsunami->plugins->Subscribe(tsunami->output);
	tsunami->output->Play(tsunami->cur_audio, false);

	while(tsunami->output->IsPlaying()){
		HuiSleep(10);
		HuiDoSingleMainLoop();
	}
	tsunami->plugins->Unsubscribe(tsunami->output);
	tsunami->plugins->Unsubscribe(tsunami->progress);
	tsunami->progress->End();


	tsunami->renderer->effect = NULL;
	fx.ImportData();
	fx.param.clear();
}

// always push the script... even if an error occurred
bool PluginManager::LoadAndCompilePlugin(const string &filename)
{
	msg_db_r("LoadAndCompilePlugin", 1);

	//msg_write(filename);

	foreach(Plugin *p, plugin){
		if (filename == p->filename){
			cur_plugin = p;
			msg_db_l(1);
			return true;
		}
	}

	//InitPluginData();

	// linking would kill type information already defined in api.kaba...
	if (plugin.num == 0)
		LinkAppScriptData();

	Plugin *p = new Plugin(filename);
	p->index = plugin.num;

	plugin.add(p);
	cur_plugin = p;

	msg_db_l(1);
	return !p->s->Error;
}
typedef void main_audiofile_func(AudioFile*);
typedef void main_void_func();

void PluginManager::ExecutePlugin(const string &filename)
{
	msg_db_r("ExecutePlugin", 1);

	if (LoadAndCompilePlugin(filename)){
		CScript *s = cur_plugin->s;

		AudioFile *a = tsunami->cur_audio;

		// run
		cur_plugin->ResetData();
		if (cur_plugin->Configure(true)){
			main_audiofile_func *f_audio = (main_audiofile_func*)s->MatchFunction("main", "void", 1, "AudioFile*");
			main_void_func *f_void = (main_void_func*)s->MatchFunction("main", "void", 0);
			if (cur_plugin->type == Plugin::TYPE_EFFECT){
				if (a->used){
					cur_plugin->ResetState();
					foreach(Track &t, a->track)
						if ((t.is_selected) && (t.type == t.TYPE_AUDIO)){
							cur_plugin->ProcessTrack(&t, a->cur_level, a->selection);
						}
				}else{
					tsunami->log->Error(_("Plugin kann nicht f&ur eine leere Audiodatei ausgef&uhrt werden"));
				}
			}else if (f_audio){
				if (a->used)
					f_audio(a);
				else
					tsunami->log->Error(_("Plugin kann nicht f&ur eine leere Audiodatei ausgef&uhrt werden"));
			}else if (f_void){

				f_void();
			}
		}

		// data changed?
		FinishPluginData();
	}else{
		string fn = filename;
		if (cur_plugin->s->pre_script)
			fn = cur_plugin->s->pre_script->Filename;
		tsunami->log->Error(format(_("Fehler in  Script-Datei: \"%s\"\n%s\n%s"), fn.c_str(), cur_plugin->s->ErrorMsgExt[0].c_str(), cur_plugin->s->ErrorMsgExt[1].c_str()));
	}

	msg_db_l(1);
}


void PluginManager::FindAndExecutePlugin()
{
	msg_db_r("ExecutePlugin", 1);


	if (HuiFileDialogOpen(tsunami, _("Plugin-Script w&ahlen"), HuiAppDirectoryStatic + "Plugins/", _("Script (*.kaba)"), "*.kaba")){
		ExecutePlugin(HuiFilename);
	}
	msg_db_l(1);
}


Plugin *PluginManager::GetPlugin(const string &name)
{
	foreach(PluginFile &pf, plugin_file)
		if (name == pf.name)
			if (LoadAndCompilePlugin(pf.filename))
				return cur_plugin;
	return NULL;
}
