/*
 * Storage.cpp
 *
 *  Created on: 24.03.2012
 *      Author: michi
 */

#include "Storage.h"
#include "Format.h"
#include "FormatWave.h"
#include "FormatRaw.h"
#include "FormatOgg.h"
#include "FormatFlac.h"
#include "FormatMidi.h"
#include "FormatNami.h"
#include "../Tsunami.h"
#include "../lib/hui/hui.h"
#include "../View/Helper/Progress.h"
#include "../Stuff/Log.h"
#include "../Audio/AudioRenderer.h"
#include "../View/AudioView.h"
#include "../Data/AudioFile.h"

Storage::Storage()
{
	format.add(new FormatNami());
	format.add(new FormatWave());
	format.add(new FormatRaw());
	format.add(new FormatOgg());
	format.add(new FormatFlac());
	format.add(new FormatMidi());

	CurrentDirectory = HuiConfig.getStr("CurrentDirectory", "");
}

Storage::~Storage()
{
	HuiConfig.setStr("CurrentDirectory", CurrentDirectory);

	foreach(Format *f, format)
		delete(f);
	format.clear();
}

bool Storage::Load(AudioFile *a, const string &filename)
{
	msg_db_f("Storage.Load", 1);
	bool ok = false;
	bool found = false;

	CurrentDirectory = filename.dirname();
	string ext = filename.extension();

	foreach(Format *f, format)
		if (f->CanHandle(ext)){
			tsunami->progress->Start(_("lade"), 0);

			a->NotifyBegin();

			a->Reset();
			a->action_manager->Enable(false);
			a->used = true;
			a->filename = filename;

			f->LoadAudio(a, filename);


			a->action_manager->Enable(true);
			tsunami->progress->Set("peaks", 1);
			a->UpdatePeaks(tsunami->view->peak_mode);

			tsunami->progress->End();
			if (a->track.num > 0)
			{}//	a->SetCurTrack(a->track[0]);
			else
				a->used = false;
			a->action_manager->Reset();
			a->NotifyEnd();
			ok = a->used;
			found = true;
			break;
		}

	if (!found)
		tsunami->log->Error(_("unbekannte Dateiendung: ") + ext);

	return ok;
}

bool Storage::LoadTrack(Track *t, const string &filename, int offset, int level)
{
	msg_db_f("Storage.LoadTrack", 1);
	bool ok = false;
	bool found = false;

	CurrentDirectory = filename.dirname();
	string ext = filename.extension();

	foreach(Format *f, format)
		if (f->CanHandle(ext)){
			tsunami->progress->Start(_("lade"), 0);

			AudioFile *a = t->root;
			a->NotifyBegin();
			a->action_manager->BeginActionGroup();

			f->LoadTrack(t, filename, offset, level);

			tsunami->progress->End();
			a->action_manager->EndActionGroup();
			a->NotifyEnd();
			found = true;
			break;
		}

	if (!found)
		tsunami->log->Error(_("unbekannte Dateiendung: ") + ext);

	return ok;
}

bool Storage::LoadBufferBox(AudioFile *a, BufferBox *buf, const string &filename)
{
	msg_db_f("Storage.LoadBufferBox", 1);
	AudioFile *aa = new AudioFile;
	aa->NewWithOneTrack(Track::TYPE_AUDIO, a->sample_rate);
	Track *t = aa->track[0];
	bool ok = LoadTrack(t, filename, 0, 0);
	buf->resize(t->level[0].buffer[0].num);
	buf->set(t->level[0].buffer[0], 0, 1);
	delete(aa);
	return ok;
}

bool Storage::Save(AudioFile *a, const string &filename)
{
	msg_db_f("Storage.Save", 1);
	bool ok = false;
	bool found = false;

	CurrentDirectory = filename.dirname();
	string ext = filename.extension();

	foreach(Format *f, format)
		if (f->CanHandle(ext)){
			if (!f->TestFormatCompatibility(a))
				tsunami->log->Warning(_("Datenverlust!"));

			tsunami->progress->Start(_("speichere"), 0);

			a->filename = filename;

			f->SaveAudio(a, filename);

			a->action_manager->MarkCurrentAsSave();
			tsunami->progress->End();
			tsunami->UpdateMenu();
			ok = true;
			found = true;
			break;
		}

	if (!found)
		tsunami->log->Error(_("unbekannte Dateiendung: ") + ext);

	return ok;
}

bool Storage::Export(AudioFile *a, const string &filename)
{
	msg_db_f("Storage.Export", 1);
	bool ok = false;
	bool found = false;

	CurrentDirectory = filename.dirname();
	string ext = filename.extension();

	foreach(Format *f, format)
		if (f->CanHandle(ext)){
			tsunami->progress->Start(_("exportiere"), 0);

			// render audio...
			tsunami->progress->Set(_("rendere Audio"), 0);
			Range r = a->GetRange();
			if (!a->selection.empty())
				r = a->selection;
			BufferBox buf;
			tsunami->renderer->RenderAudioFile(a, r, buf);

			// save
			f->SaveBuffer(a, &buf, filename);

			tsunami->progress->End();
			ok = true;
			found = true;
			break;
		}

	if (!found)
		tsunami->log->Error(_("unbekannte Dateiendung: ") + ext);

	return ok;
}

bool Storage::AskByFlags(HuiWindow *win, const string &title, bool save, int flags)
{
	string filter, filter_show;
	foreach(Format *f, format)
		if ((f->flags & flags) == flags){
			if (filter != "")
				filter += ";";
			filter += "*." + f->extension;
			if (filter_show != "")
				filter_show += ",";
			filter_show += "*." + f->extension;
		}
	if (save)
		return HuiFileDialogSave(win, title, CurrentDirectory, filter_show, filter);
	else
		return HuiFileDialogOpen(win, title, CurrentDirectory, filter_show, filter);
}

bool Storage::AskOpen(HuiWindow *win)
{
	return AskByFlags(win, _("Datei &offnen"), false, 0);
}

bool Storage::AskSave(HuiWindow *win)
{
	return AskByFlags(win, _("Datei speichern"), true, 0);
}

bool Storage::AskOpenImport(HuiWindow *win)
{
	return AskByFlags(win, _("Datei importieren"), false, Format::FLAG_SINGLE_TRACK);
}

bool Storage::AskSaveExport(HuiWindow *win)
{
	return AskByFlags(win, _("Datei exportieren"), true, Format::FLAG_SINGLE_TRACK);
}
