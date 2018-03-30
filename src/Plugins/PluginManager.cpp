/*
 * PluginManager.cpp
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#include "PluginManager.h"

#include "../Audio/Source/SongRenderer.h"
#include "../Tsunami.h"
#include "../TsunamiWindow.h"
#include "../Session.h"
#include "FastFourierTransform.h"
#include "../View/Helper/Slider.h"
#include "../Device/InputStreamAudio.h"
#include "../Device/InputStreamMidi.h"
#include "../Device/OutputStream.h"
#include "../Device/DeviceManager.h"
#include "../Audio/Synth/Synthesizer.h"
#include "../Audio/Synth/DummySynthesizer.h"
#include "../Midi/MidiSource.h"
#include "../Rhythm/Bar.h"
#include "../View/Helper/Progress.h"
#include "../Storage/Storage.h"
#include "../View/AudioView.h"
#include "../View/Dialog/ConfigurableSelectorDialog.h"
#include "../View/SideBar/SampleManagerConsole.h"
#include "../View/Mode/ViewModeCapture.h"
#include "Plugin.h"
#include "Effect.h"
#include "ConfigPanel.h"
#include "ExtendedAudioBuffer.h"
#include "MidiEffect.h"
#include "SongPlugin.h"
#include "TsunamiPlugin.h"
#include "FavoriteManager.h"

#define _offsetof(CLASS, ELEMENT) (int)( (char*)&((CLASS*)1)->ELEMENT - (char*)((CLASS*)1) )

extern InputStreamAudio *export_view_input;


PluginManager::PluginManager()
{
	favorites = new FavoriteManager;

	FindPlugins();
}

PluginManager::~PluginManager()
{
	delete(favorites);
	Kaba::End();
}


bool GlobalAllowTermination()
{
	return tsunami->allowTermination();
}


void GlobalSetTempBackupFilename(const string &filename)
{
	//InputStreamAudio::setTempBackupFilename(filename);
}

Synthesizer* GlobalCreateSynthesizer(Session *session, const string &name)
{
	return session->plugin_manager->CreateSynthesizer(session, name);
}

void PluginManager::LinkAppScriptData()
{
	Kaba::config.directory = "";

	// api definition
	Kaba::LinkExternal("device_manager", &tsunami->device_manager);
	Kaba::LinkExternal("colors", &AudioView::_export_colors);
	Kaba::LinkExternal("view_input", &export_view_input);
	Kaba::LinkExternal("fft_c2c", (void*)&FastFourierTransform::fft_c2c);
	Kaba::LinkExternal("fft_r2c", (void*)&FastFourierTransform::fft_r2c);
	Kaba::LinkExternal("fft_c2r_inv", (void*)&FastFourierTransform::fft_c2r_inv);
	Kaba::LinkExternal("CreateSynthesizer", (void*)&GlobalCreateSynthesizer);
	Kaba::LinkExternal("CreateAudioEffect", (void*)&CreateEffect);
	Kaba::LinkExternal("CreateMidiEffect", (void*)&CreateMidiEffect);
	Kaba::LinkExternal("AllowTermination", (void*)&GlobalAllowTermination);
	Kaba::LinkExternal("SetTempBackupFilename", (void*)&GlobalSetTempBackupFilename);
	Kaba::LinkExternal("SelectSample", (void*)&SampleManagerConsole::select);

	Kaba::DeclareClassSize("Range", sizeof(Range));
	Kaba::DeclareClassOffset("Range", "offset", _offsetof(Range, offset));
	Kaba::DeclareClassOffset("Range", "length", _offsetof(Range, length));


	Kaba::DeclareClassSize("Session", sizeof(Session));
	Kaba::DeclareClassOffset("Session", "id", _offsetof(Session, id));
	Kaba::DeclareClassOffset("Session", "storage", _offsetof(Session, storage));
	Kaba::DeclareClassOffset("Session", "win", _offsetof(Session, _kaba_win));
	Kaba::DeclareClassOffset("Session", "song", _offsetof(Session, song));
	Kaba::LinkExternal("Session.i", Kaba::mf(&Session::i));
	Kaba::LinkExternal("Session.w", Kaba::mf(&Session::w));
	Kaba::LinkExternal("Session.e", Kaba::mf(&Session::e));


	PluginData plugin_data;
	Kaba::DeclareClassSize("PluginData", sizeof(PluginData));
	Kaba::LinkExternal("PluginData." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&PluginData::__init__));
	Kaba::DeclareClassVirtualIndex("PluginData", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&PluginData::__delete__), &plugin_data);
	Kaba::DeclareClassVirtualIndex("PluginData", "reset", Kaba::mf(&PluginData::reset), &plugin_data);


	ConfigPanel config_panel;
	Kaba::DeclareClassSize("ConfigPanel", sizeof(ConfigPanel));
	Kaba::LinkExternal("ConfigPanel." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&ConfigPanel::__init__));
	Kaba::DeclareClassVirtualIndex("ConfigPanel", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&ConfigPanel::__delete__), &config_panel);
	Kaba::DeclareClassVirtualIndex("ConfigPanel", "update", Kaba::mf(&ConfigPanel::update), &config_panel);
	Kaba::LinkExternal("ConfigPanel.notify", Kaba::mf(&ConfigPanel::notify));
	Kaba::DeclareClassOffset("ConfigPanel", "c", _offsetof(ConfigPanel, c));


	Effect effect;
	Kaba::DeclareClassSize("AudioEffect", sizeof(Effect));
	Kaba::DeclareClassOffset("AudioEffect", "name", _offsetof(Effect, name));
	Kaba::DeclareClassOffset("AudioEffect", "usable", _offsetof(Effect, usable));
	Kaba::DeclareClassOffset("AudioEffect", "session", _offsetof(Effect, session));
	Kaba::DeclareClassOffset("AudioEffect", "sample_rate", _offsetof(Effect, sample_rate));
	Kaba::LinkExternal("AudioEffect." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&Effect::__init__));
	Kaba::DeclareClassVirtualIndex("AudioEffect", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&Effect::__delete__), &effect);
	Kaba::DeclareClassVirtualIndex("AudioEffect", "process", Kaba::mf(&Effect::process), &effect);
	Kaba::DeclareClassVirtualIndex("AudioEffect", "createPanel", Kaba::mf(&Effect::createPanel), &effect);
	Kaba::LinkExternal("AudioEffect.resetConfig", Kaba::mf(&Effect::resetConfig));
	Kaba::LinkExternal("AudioEffect.resetState", Kaba::mf(&Effect::resetState));
	//Kaba::DeclareClassVirtualIndex("AudioEffect", "updateDialog", Kaba::mf(&Effect::UpdateDialog), &effect);
	Kaba::LinkExternal("AudioEffect.notify", Kaba::mf(&Effect::notify));
	Kaba::DeclareClassVirtualIndex("AudioEffect", "onConfig", Kaba::mf(&Effect::onConfig), &effect);

	MidiEffect midieffect;
	Kaba::DeclareClassSize("MidiEffect", sizeof(MidiEffect));
	Kaba::DeclareClassOffset("MidiEffect", "name", _offsetof(MidiEffect, name));
	Kaba::DeclareClassOffset("MidiEffect", "only_on_selection", _offsetof(MidiEffect, only_on_selection));
	Kaba::DeclareClassOffset("MidiEffect", "range", _offsetof(MidiEffect, range));
	Kaba::DeclareClassOffset("MidiEffect", "usable", _offsetof(MidiEffect, usable));
	Kaba::DeclareClassOffset("MidiEffect", "session", _offsetof(MidiEffect, session));
	Kaba::LinkExternal("MidiEffect." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&MidiEffect::__init__));
	Kaba::DeclareClassVirtualIndex("MidiEffect", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&MidiEffect::__delete__), &midieffect);
	Kaba::DeclareClassVirtualIndex("MidiEffect", "process", Kaba::mf(&MidiEffect::process), &midieffect);
	Kaba::DeclareClassVirtualIndex("MidiEffect", "createPanel", Kaba::mf(&MidiEffect::createPanel), &midieffect);
	Kaba::LinkExternal("MidiEffect.resetConfig", Kaba::mf(&MidiEffect::resetConfig));
	Kaba::LinkExternal("MidiEffect.resetState", Kaba::mf(&MidiEffect::resetState));
	//Kaba::DeclareClassVirtualIndex("MidiEffect", "updateDialog", Kaba::mf(&MidiEffect::UpdateDialog), &midieffect);
	Kaba::LinkExternal("MidiEffect.notify", Kaba::mf(&MidiEffect::notify));
	Kaba::DeclareClassVirtualIndex("MidiEffect", "onConfig", Kaba::mf(&MidiEffect::onConfig), &midieffect);
	Kaba::LinkExternal("MidiEffect.note", Kaba::mf(&MidiEffect::note));
	Kaba::LinkExternal("MidiEffect.skip", Kaba::mf(&MidiEffect::skip));
	Kaba::LinkExternal("MidiEffect.note_x", Kaba::mf(&MidiEffect::note_x));
	Kaba::LinkExternal("MidiEffect.skip_x", Kaba::mf(&MidiEffect::skip_x));

	Kaba::DeclareClassSize("AudioBuffer", sizeof(AudioBuffer));
	Kaba::DeclareClassOffset("AudioBuffer", "offset", _offsetof(AudioBuffer, offset));
	Kaba::DeclareClassOffset("AudioBuffer", "length", _offsetof(AudioBuffer, length));
	Kaba::DeclareClassOffset("AudioBuffer", "channels", _offsetof(AudioBuffer, channels));
	Kaba::DeclareClassOffset("AudioBuffer", "r", _offsetof(AudioBuffer, c[0]));
	Kaba::DeclareClassOffset("AudioBuffer", "l", _offsetof(AudioBuffer, c[1]));
	Kaba::DeclareClassOffset("AudioBuffer", "peaks", _offsetof(AudioBuffer, peaks));
	Kaba::LinkExternal("AudioBuffer." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&AudioBuffer::__init__));
	Kaba::LinkExternal("AudioBuffer." + Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&AudioBuffer::__delete__));
	Kaba::LinkExternal("AudioBuffer.clear", Kaba::mf(&AudioBuffer::clear));
	Kaba::LinkExternal("AudioBuffer." + Kaba::IDENTIFIER_FUNC_ASSIGN, Kaba::mf(&AudioBuffer::__assign__));
	Kaba::LinkExternal("AudioBuffer.range", Kaba::mf(&AudioBuffer::range));
	Kaba::LinkExternal("AudioBuffer.add", Kaba::mf(&AudioBuffer::add));
	Kaba::LinkExternal("AudioBuffer.set", Kaba::mf(&AudioBuffer::set));
	Kaba::LinkExternal("AudioBuffer.get_spectrum", Kaba::mf(&ExtendedAudioBuffer::get_spectrum));


	Kaba::DeclareClassSize("RingBuffer", sizeof(RingBuffer));
	//Kaba::LinkExternal("RingBuffer." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&RingBuffer::__init__));
	Kaba::LinkExternal("RingBuffer.available", Kaba::mf(&RingBuffer::available));
	Kaba::LinkExternal("RingBuffer.read", Kaba::mf(&RingBuffer::read));
	Kaba::LinkExternal("RingBuffer.write", Kaba::mf(&RingBuffer::write));
	Kaba::LinkExternal("RingBuffer.readRef", Kaba::mf(&RingBuffer::readRef));
	Kaba::LinkExternal("RingBuffer.peekRef", Kaba::mf(&RingBuffer::peekRef));
	Kaba::LinkExternal("RingBuffer.writeRef", Kaba::mf(&RingBuffer::writeRef));
	Kaba::LinkExternal("RingBuffer.moveReadPos", Kaba::mf(&RingBuffer::moveReadPos));
	Kaba::LinkExternal("RingBuffer.moveWritePos", Kaba::mf(&RingBuffer::moveWritePos));
	Kaba::LinkExternal("RingBuffer.clear", Kaba::mf(&RingBuffer::clear));

	Kaba::DeclareClassSize("Sample", sizeof(Sample));
	Kaba::DeclareClassOffset("Sample", "name", _offsetof(Sample, name));
	Kaba::DeclareClassOffset("Sample", "type", _offsetof(Sample, type));
	Kaba::DeclareClassOffset("Sample", "buf", _offsetof(Sample, buf));
	Kaba::DeclareClassOffset("Sample", "midi", _offsetof(Sample, midi));
	Kaba::DeclareClassOffset("Sample", "volume", _offsetof(Sample, volume));
	Kaba::DeclareClassOffset("Sample", "uid", _offsetof(Sample, uid));
	Kaba::DeclareClassOffset("Sample", "tags", _offsetof(Sample, tags));
	Kaba::LinkExternal("Sample.createRef", Kaba::mf(&Sample::create_ref));
	Kaba::LinkExternal("Sample.getValue", Kaba::mf(&Sample::getValue));

	Sample sample(0);
	//sample.owner = tsunami->song;
	SampleRef sampleref(&sample);
	Kaba::DeclareClassSize("SampleRef", sizeof(SampleRef));
	Kaba::DeclareClassOffset("SampleRef", "buf", _offsetof(SampleRef, buf));
	Kaba::DeclareClassOffset("SampleRef", "midi", _offsetof(SampleRef, midi));
	Kaba::DeclareClassOffset("SampleRef", "origin", _offsetof(SampleRef, origin));
	Kaba::LinkExternal("SampleRef." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&SampleRef::__init__));
	Kaba::DeclareClassVirtualIndex("SampleRef", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&SampleRef::__delete__), &sampleref);

	MidiSource midi_source;
	Kaba::DeclareClassSize("MidiSource", sizeof(MidiSource));
	Kaba::LinkExternal("MidiSource." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&MidiSource::__init__));
	Kaba::DeclareClassVirtualIndex("MidiSource", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&MidiSource::__delete__), &midi_source);
	Kaba::DeclareClassVirtualIndex("MidiSource", "read", Kaba::mf(&MidiSource::read), &midi_source);

	Synthesizer synth;
	Kaba::DeclareClassSize("Synthesizer", sizeof(Synthesizer));
	Kaba::DeclareClassOffset("Synthesizer", "name", _offsetof(Synthesizer, name));
	Kaba::DeclareClassOffset("Synthesizer", "session", _offsetof(Synthesizer, session));
	Kaba::DeclareClassOffset("Synthesizer", "sample_rate", _offsetof(Synthesizer, sample_rate));
	Kaba::DeclareClassOffset("Synthesizer", "events", _offsetof(Synthesizer, events));
	Kaba::DeclareClassOffset("Synthesizer", "keep_notes", _offsetof(Synthesizer, keep_notes));
	Kaba::DeclareClassOffset("Synthesizer", "active_pitch", _offsetof(Synthesizer, active_pitch));
	Kaba::DeclareClassOffset("Synthesizer", "freq", _offsetof(Synthesizer, tuning.freq));
	Kaba::DeclareClassOffset("Synthesizer", "delta_phi", _offsetof(Synthesizer, delta_phi));
	Kaba::DeclareClassOffset("Synthesizer", "out", _offsetof(Synthesizer, out));
	Kaba::LinkExternal("Synthesizer." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&Synthesizer::__init__));
	Kaba::DeclareClassVirtualIndex("Synthesizer", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&Synthesizer::__delete__), &synth);
	Kaba::DeclareClassVirtualIndex("Synthesizer", "createPanel", Kaba::mf(&Synthesizer::createPanel), &synth);
	Kaba::LinkExternal("Synthesizer.resetConfig", Kaba::mf(&Synthesizer::resetConfig));
	Kaba::LinkExternal("Synthesizer.resetState", Kaba::mf(&Synthesizer::resetState));
	Kaba::LinkExternal("Synthesizer.enablePitch", Kaba::mf(&Synthesizer::enablePitch));
	Kaba::DeclareClassVirtualIndex("Synthesizer", "render", Kaba::mf(&Synthesizer::render), &synth);
	Kaba::DeclareClassVirtualIndex("Synthesizer", "onConfig", Kaba::mf(&Synthesizer::onConfig), &synth);
	Kaba::LinkExternal("Synthesizer.setSampleRate", Kaba::mf(&Synthesizer::setSampleRate));
	Kaba::LinkExternal("Synthesizer.notify", Kaba::mf(&Synthesizer::notify));

	Synthesizer::Output synth_out(NULL);
	Kaba::DeclareClassSize("SynthOutput", sizeof(Synthesizer::Output));
	Kaba::DeclareClassVirtualIndex("SynthOutput", "read", Kaba::mf(&Synthesizer::Output::read), &synth_out);
	//Kaba::DeclareClassVirtualIndex("SynthOutput", "reset", Kaba::mf(&Synthesizer::Output::reset), &synth_out);
	Kaba::DeclareClassVirtualIndex("SynthOutput", "getSampleRate", Kaba::mf(&Synthesizer::Output::getSampleRate), &synth_out);
	Kaba::LinkExternal("SynthOutput.setSource", Kaba::mf(&Synthesizer::Output::setSource));


	DummySynthesizer dsynth;
	Kaba::DeclareClassSize("DummySynthesizer", sizeof(DummySynthesizer));
	Kaba::LinkExternal("DummySynthesizer." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&DummySynthesizer::__init__));
	Kaba::DeclareClassVirtualIndex("DummySynthesizer", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&DummySynthesizer::__delete__), &dsynth);
	Kaba::DeclareClassVirtualIndex("DummySynthesizer", "render", Kaba::mf(&DummySynthesizer::render), &dsynth);
	Kaba::DeclareClassVirtualIndex("DummySynthesizer", "onConfig", Kaba::mf(&DummySynthesizer::onConfig), &dsynth);

	Kaba::DeclareClassSize("EnvelopeADSR", sizeof(EnvelopeADSR));
	Kaba::LinkExternal("EnvelopeADSR.set", Kaba::mf(&EnvelopeADSR::set));
	Kaba::LinkExternal("EnvelopeADSR.set2", Kaba::mf(&EnvelopeADSR::set2));
	Kaba::LinkExternal("EnvelopeADSR.reset", Kaba::mf(&EnvelopeADSR::reset));
	Kaba::LinkExternal("EnvelopeADSR.start", Kaba::mf(&EnvelopeADSR::start));
	Kaba::LinkExternal("EnvelopeADSR.end", Kaba::mf(&EnvelopeADSR::end));
	Kaba::LinkExternal("EnvelopeADSR.get", Kaba::mf(&EnvelopeADSR::get));
	Kaba::DeclareClassOffset("EnvelopeADSR", "just_killed", _offsetof(EnvelopeADSR, just_killed));

	Kaba::DeclareClassSize("BarPattern", sizeof(BarPattern));
	Kaba::DeclareClassOffset("BarPattern", "num_beats", _offsetof(BarPattern, num_beats));
	Kaba::DeclareClassOffset("BarPattern", "length", _offsetof(BarPattern, length));
	//Kaba::DeclareClassOffset("BarPattern", "type", _offsetof(BarPattern, type));
	//Kaba::DeclareClassOffset("BarPattern", "count", _offsetof(BarPattern, count));

	Kaba::DeclareClassSize("MidiNote", sizeof(MidiNote));
	Kaba::DeclareClassOffset("MidiNote", "range", _offsetof(MidiNote, range));
	Kaba::DeclareClassOffset("MidiNote", "pitch", _offsetof(MidiNote, pitch));
	Kaba::DeclareClassOffset("MidiNote", "volume", _offsetof(MidiNote, volume));
	Kaba::DeclareClassOffset("MidiNote", "stringno", _offsetof(MidiNote, stringno));
	Kaba::DeclareClassOffset("MidiNote", "clef_position", _offsetof(MidiNote, clef_position));
	Kaba::DeclareClassOffset("MidiNote", "modifier", _offsetof(MidiNote, modifier));

	Kaba::DeclareClassSize("MidiEventBuffer", sizeof(MidiEventBuffer));
	Kaba::DeclareClassOffset("MidiEventBuffer", "samples", _offsetof(MidiEventBuffer, samples));
	Kaba::LinkExternal("MidiEventBuffer." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&MidiEventBuffer::__init__));
	Kaba::LinkExternal("MidiEventBuffer.getEvents", Kaba::mf(&MidiEventBuffer::getEvents));
	Kaba::LinkExternal("MidiEventBuffer.getNotes", Kaba::mf(&MidiEventBuffer::getNotes));
	Kaba::LinkExternal("MidiEventBuffer.getRange", Kaba::mf(&MidiEventBuffer::getRange));
	Kaba::LinkExternal("MidiEventBuffer.addMetronomeClick", Kaba::mf(&MidiEventBuffer::addMetronomeClick));

	Kaba::DeclareClassSize("MidiNoteBuffer", sizeof(MidiNoteBuffer));
	Kaba::DeclareClassOffset("MidiNoteBuffer", "samples", _offsetof(MidiNoteBuffer, samples));
	Kaba::LinkExternal("MidiNoteBuffer." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&MidiNoteBuffer::__init__));
	Kaba::LinkExternal("MidiNoteBuffer.getEvents", Kaba::mf(&MidiNoteBuffer::getEvents));
	Kaba::LinkExternal("MidiNoteBuffer.getNotes", Kaba::mf(&MidiNoteBuffer::getNotes));
	Kaba::LinkExternal("MidiNoteBuffer.getRange", Kaba::mf(&MidiNoteBuffer::range));

	Kaba::DeclareClassSize("TrackMarker", sizeof(TrackMarker));
	Kaba::DeclareClassOffset("TrackMarker", "text", _offsetof(TrackMarker, text));
	Kaba::DeclareClassOffset("TrackMarker", "range", _offsetof(TrackMarker, range));
	Kaba::DeclareClassOffset("TrackMarker", "fx", _offsetof(TrackMarker, fx));

	Kaba::DeclareClassSize("TrackLayer", sizeof(TrackLayer));
	Kaba::DeclareClassOffset("TrackLayer", "buffers", _offsetof(TrackLayer, buffers));

	Kaba::DeclareClassSize("Track", sizeof(Track));
	Kaba::DeclareClassOffset("Track", "type", _offsetof(Track, type));
	Kaba::DeclareClassOffset("Track", "name", _offsetof(Track, name));
	Kaba::DeclareClassOffset("Track", "layers", _offsetof(Track, layers));
	Kaba::DeclareClassOffset("Track", "volume", _offsetof(Track, volume));
	Kaba::DeclareClassOffset("Track", "panning", _offsetof(Track, panning));
	Kaba::DeclareClassOffset("Track", "muted", _offsetof(Track, muted));
	Kaba::DeclareClassOffset("Track", "fx", _offsetof(Track, fx));
	Kaba::DeclareClassOffset("Track", "midi", _offsetof(Track, midi));
	Kaba::DeclareClassOffset("Track", "synth", _offsetof(Track, synth));
	Kaba::DeclareClassOffset("Track", "samples", _offsetof(Track, samples));
	Kaba::DeclareClassOffset("Track", "markers", _offsetof(Track, markers));
	Kaba::DeclareClassOffset("Track", "root", _offsetof(Track, song));
	//Kaba::DeclareClassOffset("Track", "is_selected", _offsetof(Track, is_selected));
	Kaba::LinkExternal("Track.getBuffers", Kaba::mf(&Track::getBuffers));
	Kaba::LinkExternal("Track.readBuffers", Kaba::mf(&Track::readBuffers));
	Kaba::LinkExternal("Track.setName", Kaba::mf(&Track::setName));
	Kaba::LinkExternal("Track.setMuted", Kaba::mf(&Track::setMuted));
	Kaba::LinkExternal("Track.setVolume", Kaba::mf(&Track::setVolume));
	Kaba::LinkExternal("Track.setPanning", Kaba::mf(&Track::setPanning));
	Kaba::LinkExternal("Track.insertMidiData", Kaba::mf(&Track::insertMidiData));
	Kaba::LinkExternal("Track.addEffect", Kaba::mf(&Track::addEffect));
	Kaba::LinkExternal("Track.deleteEffect", Kaba::mf(&Track::deleteEffect));
	Kaba::LinkExternal("Track.editEffect", Kaba::mf(&Track::editEffect));
	Kaba::LinkExternal("Track.enableEffect", Kaba::mf(&Track::enableEffect));
	Kaba::LinkExternal("Track.addSampleRef", Kaba::mf(&Track::addSampleRef));
	Kaba::LinkExternal("Track.deleteSampleRef", Kaba::mf(&Track::deleteSampleRef));
	Kaba::LinkExternal("Track.editSampleRef", Kaba::mf(&Track::editSampleRef));
	Kaba::LinkExternal("Track.addMidiNote", Kaba::mf(&Track::addMidiNote));
	//Kaba::LinkExternal("Track.addMidiNotes", Kaba::mf(&Track::addMidiNotes));
	Kaba::LinkExternal("Track.deleteMidiNote", Kaba::mf(&Track::deleteMidiNote));
	Kaba::LinkExternal("Track.setSynthesizer", Kaba::mf(&Track::setSynthesizer));
	Kaba::LinkExternal("Track.addMarker", Kaba::mf(&Track::addMarker));
	Kaba::LinkExternal("Track.deleteMarker", Kaba::mf(&Track::deleteMarker));
	Kaba::LinkExternal("Track.editMarker", Kaba::mf(&Track::editMarker));

	Song af = Song(Session::GLOBAL);
	Kaba::DeclareClassSize("Song", sizeof(Song));
	Kaba::DeclareClassOffset("Song", "filename", _offsetof(Song, filename));
	Kaba::DeclareClassOffset("Song", "tag", _offsetof(Song, tags));
	Kaba::DeclareClassOffset("Song", "sample_rate", _offsetof(Song, sample_rate));
	Kaba::DeclareClassOffset("Song", "volume", _offsetof(Song, volume));
	Kaba::DeclareClassOffset("Song", "fx", _offsetof(Song, fx));
	Kaba::DeclareClassOffset("Song", "tracks", _offsetof(Song, tracks));
	Kaba::DeclareClassOffset("Song", "samples", _offsetof(Song, samples));
	Kaba::DeclareClassOffset("Song", "layers", _offsetof(Song, layers));
	Kaba::DeclareClassOffset("Song", "bars", _offsetof(Song, bars));
	Kaba::LinkExternal("Song." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&Song::__init__));
	Kaba::DeclareClassVirtualIndex("Song", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&Song::__delete__), &af);
	Kaba::LinkExternal("Song.newEmpty", Kaba::mf(&Song::newEmpty));
	Kaba::LinkExternal("Song.addTrack", Kaba::mf(&Song::addTrack));
	Kaba::LinkExternal("Song.deleteTrack", Kaba::mf(&Song::deleteTrack));
	Kaba::LinkExternal("Song.getRange", Kaba::mf(&Song::getRange));
	Kaba::LinkExternal("Song.addBar", Kaba::mf(&Song::addBar));
	Kaba::LinkExternal("Song.addPause", Kaba::mf(&Song::addPause));
	Kaba::LinkExternal("Song.editBar", Kaba::mf(&Song::editBar));
	Kaba::LinkExternal("Song.deleteBar", Kaba::mf(&Song::deleteBar));
	Kaba::LinkExternal("Song.addSample", Kaba::mf(&Song::addSample));
	Kaba::LinkExternal("Song.deleteSample", Kaba::mf(&Song::deleteSample));

	AudioSource ar;
	Kaba::DeclareClassSize("AudioSource", sizeof(AudioSource));
	//Kaba::DeclareClassOffset("AudioSource", "sample_rate", _offsetof(AudioSource, sample_rate));
	Kaba::LinkExternal("AudioSource." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&AudioSource::__init__));
	Kaba::DeclareClassVirtualIndex("AudioSource", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&AudioSource::__delete__), &ar);
	Kaba::DeclareClassVirtualIndex("AudioSource", "read", Kaba::mf(&AudioSource::read), &ar);
	Kaba::DeclareClassVirtualIndex("AudioSource", "getSampleRate", Kaba::mf(&AudioSource::getSampleRate), &ar);

	SongRenderer sr(&af);
	Kaba::DeclareClassSize("SongRenderer", sizeof(SongRenderer));
	Kaba::LinkExternal("SongRenderer.prepare", Kaba::mf(&SongRenderer::prepare));
	Kaba::LinkExternal("SongRenderer.render", Kaba::mf(&SongRenderer::render));
	Kaba::LinkExternal("SongRenderer." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&SongRenderer::__init__));
	Kaba::DeclareClassVirtualIndex("SongRenderer", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&SongRenderer::__delete__), &sr);
	Kaba::DeclareClassVirtualIndex("SongRenderer", "read", Kaba::mf(&SongRenderer::read), &sr);
	Kaba::LinkExternal("SongRenderer.range", Kaba::mf(&SongRenderer::range));
	Kaba::LinkExternal("SongRenderer.getPos", Kaba::mf(&SongRenderer::getPos));
	Kaba::LinkExternal("SongRenderer.seek", Kaba::mf(&SongRenderer::seek));
	Kaba::DeclareClassVirtualIndex("SongRenderer", "getSampleRate", Kaba::mf(&SongRenderer::getSampleRate), &sr);

	{
	InputStreamAudio input(Session::GLOBAL, DEFAULT_SAMPLE_RATE);
	Kaba::DeclareClassSize("InputStreamAudio", sizeof(InputStreamAudio));
	Kaba::DeclareClassOffset("InputStreamAudio", "session", _offsetof(InputStreamAudio, session));
	Kaba::DeclareClassOffset("InputStreamAudio", "current_buffer", _offsetof(InputStreamAudio, buffer));
	//Kaba::DeclareClassOffset("InputStreamAudio", "buffer", _offsetof(InputStreamAudio, buffer));
	Kaba::DeclareClassOffset("InputStreamAudio", "source", _offsetof(InputStreamAudio, source));
	//Kaba::DeclareClassOffset("InputStreamAudio", "accumulating", _offsetof(InputStreamAudio, accumulating));
	Kaba::DeclareClassOffset("InputStreamAudio", "capturing", _offsetof(InputStreamAudio, capturing));
	Kaba::LinkExternal("InputStreamAudio." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&InputStreamAudio::__init__));
	Kaba::DeclareClassVirtualIndex("InputStreamAudio", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&InputStreamAudio::__delete__), &input);
	Kaba::LinkExternal("InputStreamAudio.start", Kaba::mf(&InputStreamAudio::start));
	Kaba::LinkExternal("InputStreamAudio.stop",	 Kaba::mf(&InputStreamAudio::stop));
	Kaba::LinkExternal("InputStreamAudio.isCapturing", Kaba::mf(&InputStreamAudio::isCapturing));
	Kaba::LinkExternal("InputStreamAudio.subscribe", Kaba::mf(&InputStreamAudio::subscribe_kaba));
	Kaba::LinkExternal("InputStreamAudio.unsubscribe", Kaba::mf(&InputStreamAudio::unsubscribe));
	Kaba::DeclareClassVirtualIndex("InputStreamAudio", "getSampleRate", Kaba::mf(&InputStreamAudio::getSampleRate), &input);
	Kaba::DeclareClassVirtualIndex("InputStreamAudio", "getSomeSamples", Kaba::mf(&InputStreamAudio::getSomeSamples), &input);
	Kaba::DeclareClassVirtualIndex("InputStreamAudio", "getState", Kaba::mf(&InputStreamAudio::getState), &input);
	Kaba::LinkExternal("InputStreamAudio.setBackupMode", Kaba::mf(&InputStreamAudio::setBackupMode));
	}

	{
	OutputStream stream(Session::GLOBAL, NULL);
	Kaba::DeclareClassSize("OutputStream", sizeof(OutputStream));
	Kaba::LinkExternal("OutputStream." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&OutputStream::__init__));
	Kaba::DeclareClassVirtualIndex("OutputStream", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&OutputStream::__delete__), &stream);
	//Kaba::LinkExternal("OutputStream.setSource", Kaba::mf(&AudioStream::setSource));
	Kaba::LinkExternal("OutputStream.play", Kaba::mf(&OutputStream::play));
	Kaba::LinkExternal("OutputStream.stop", Kaba::mf(&OutputStream::stop));
	Kaba::LinkExternal("OutputStream.pause", Kaba::mf(&OutputStream::pause));
	Kaba::LinkExternal("OutputStream.isPaused", Kaba::mf(&OutputStream::isPaused));
	Kaba::LinkExternal("OutputStream.getPos", Kaba::mf(&OutputStream::getPos));
	Kaba::LinkExternal("OutputStream.getSampleRate", Kaba::mf(&OutputStream::getSampleRate));
	Kaba::LinkExternal("OutputStream.getVolume", Kaba::mf(&OutputStream::getVolume));
	Kaba::LinkExternal("OutputStream.setVolume", Kaba::mf(&OutputStream::setVolume));
	Kaba::LinkExternal("OutputStream.setBufferSize", Kaba::mf(&OutputStream::setBufferSize));
//	Kaba::DeclareClassVirtualIndex("OutputStream", "", Kaba::mf(&OutputStream::__delete__), &stream);
	Kaba::LinkExternal("OutputStream.subscribe", Kaba::mf(&OutputStream::subscribe_kaba));
	Kaba::LinkExternal("OutputStream.unsubscribe", Kaba::mf(&OutputStream::unsubscribe));
	Kaba::DeclareClassVirtualIndex("OutputStream", "getSomeSamples", Kaba::mf(&OutputStream::getSomeSamples), &stream);
	}

	Kaba::DeclareClassSize("AudioView", sizeof(AudioView));
	Kaba::DeclareClassOffset("AudioView", "sel", _offsetof(AudioView, sel));
	Kaba::DeclareClassOffset("AudioView", "stream", _offsetof(AudioView, stream));
	Kaba::DeclareClassOffset("AudioView", "renderer", _offsetof(AudioView, renderer));
	//Kaba::DeclareClassOffset("AudioView", "input", _offsetof(AudioView, input));
	Kaba::LinkExternal("AudioView.subscribe", Kaba::mf(&AudioView::subscribe_kaba));
	Kaba::LinkExternal("AudioView.unsubscribe", Kaba::mf(&AudioView::unsubscribe));

	Kaba::DeclareClassSize("ColorScheme", sizeof(ColorScheme));
	Kaba::DeclareClassOffset("ColorScheme", "background", _offsetof(ColorScheme, background));
	Kaba::DeclareClassOffset("ColorScheme", "background_track", _offsetof(ColorScheme, background_track));
	Kaba::DeclareClassOffset("ColorScheme", "background_track_selected", _offsetof(ColorScheme, background_track_selected));
	Kaba::DeclareClassOffset("ColorScheme", "text", _offsetof(ColorScheme, text));
	Kaba::DeclareClassOffset("ColorScheme", "text_soft1", _offsetof(ColorScheme, text_soft1));
	Kaba::DeclareClassOffset("ColorScheme", "text_soft2", _offsetof(ColorScheme, text_soft2));
	Kaba::DeclareClassOffset("ColorScheme", "text_soft3", _offsetof(ColorScheme, text_soft3));
	Kaba::DeclareClassOffset("ColorScheme", "grid", _offsetof(ColorScheme, grid));
	Kaba::DeclareClassOffset("ColorScheme", "selection", _offsetof(ColorScheme, selection));
	Kaba::DeclareClassOffset("ColorScheme", "hover", _offsetof(ColorScheme, hover));

	Kaba::LinkExternal("Storage.load", Kaba::mf(&Storage::load));
	Kaba::LinkExternal("Storage.save", Kaba::mf(&Storage::save));
	Kaba::DeclareClassOffset("Storage", "current_directory", _offsetof(Storage, current_directory));


	Slider slider;
	Kaba::DeclareClassSize("Slider", sizeof(Slider));
	Kaba::LinkExternal("Slider." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&Slider::__init_ext__));
	Kaba::DeclareClassVirtualIndex("Slider", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&Slider::__delete__), &slider);
	Kaba::LinkExternal("Slider.get", Kaba::mf(&Slider::get));
	Kaba::LinkExternal("Slider.set", Kaba::mf(&Slider::set));

	SongPlugin song_plugin;
	Kaba::DeclareClassSize("SongPlugin", sizeof(SongPlugin));
	Kaba::DeclareClassOffset("SongPlugin", "session", _offsetof(SongPlugin, session));
	Kaba::DeclareClassOffset("SongPlugin", "song", _offsetof(SongPlugin, song));
	Kaba::LinkExternal("SongPlugin." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&SongPlugin::__init__));
	Kaba::DeclareClassVirtualIndex("SongPlugin", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&SongPlugin::__delete__), &song_plugin);
	Kaba::DeclareClassVirtualIndex("SongPlugin", "apply", Kaba::mf(&SongPlugin::apply), &song_plugin);

	TsunamiPlugin tsunami_plugin;
	Kaba::DeclareClassSize("TsunamiPlugin", sizeof(TsunamiPlugin));
	Kaba::DeclareClassOffset("TsunamiPlugin", "session", _offsetof(TsunamiPlugin, session));
	Kaba::DeclareClassOffset("TsunamiPlugin", "song", _offsetof(TsunamiPlugin, song));
	Kaba::DeclareClassOffset("TsunamiPlugin", "args", _offsetof(TsunamiPlugin, args));
	Kaba::LinkExternal("TsunamiPlugin." + Kaba::IDENTIFIER_FUNC_INIT, Kaba::mf(&TsunamiPlugin::__init__));
	Kaba::DeclareClassVirtualIndex("TsunamiPlugin", Kaba::IDENTIFIER_FUNC_DELETE, Kaba::mf(&TsunamiPlugin::__delete__), &tsunami_plugin);
	Kaba::DeclareClassVirtualIndex("TsunamiPlugin", "onStart", Kaba::mf(&TsunamiPlugin::onStart), &tsunami_plugin);
	Kaba::DeclareClassVirtualIndex("TsunamiPlugin", "onStop", Kaba::mf(&TsunamiPlugin::onStop), &tsunami_plugin);
	Kaba::LinkExternal("TsunamiPlugin.stop", Kaba::mf(&TsunamiPlugin::stop_request));

}

void get_plugin_file_data(PluginManager::PluginFile &pf)
{
	pf.image = "";
	try{
		string content = FileRead(pf.filename);
		int p = content.find("// Image = hui:");
		if (p >= 0)
			pf.image = content.substr(p + 11, content.find("\n") - p - 11);
	}catch(...){}
}

void find_plugins_in_dir(const string &dir, int type, PluginManager *pm)
{
	Array<DirEntry> list = dir_search(pm->plugin_dir() + dir, "*.kaba", false);
	for (DirEntry &e : list){
		PluginManager::PluginFile pf;
		pf.type = type;
		pf.name = e.name.replace(".kaba", "");
		pf.filename = pm->plugin_dir() + dir + e.name;
		get_plugin_file_data(pf);
		pm->plugin_files.add(pf);
	}
}

void add_plugins_in_dir(const string &dir, PluginManager *pm, hui::Menu *m, const string &name_space, TsunamiWindow *win, void (TsunamiWindow::*function)())
{
	for (PluginManager::PluginFile &f: pm->plugin_files){
		if (f.filename.find(dir) >= 0){
			string id = "execute-" + name_space + "--" + f.name;
            m->addItemImage(f.name, f.image, id);
            win->event(id, std::bind(function, win));
		}
	}
}

void PluginManager::FindPlugins()
{
	Kaba::Init();

	// "Buffer"
	find_plugins_in_dir("Buffer/Channels/", Plugin::Type::EFFECT, this);
	find_plugins_in_dir("Buffer/Dynamics/", Plugin::Type::EFFECT, this);
	find_plugins_in_dir("Buffer/Echo/", Plugin::Type::EFFECT, this);
	find_plugins_in_dir("Buffer/Pitch/", Plugin::Type::EFFECT, this);
	find_plugins_in_dir("Buffer/Repair/", Plugin::Type::EFFECT, this);
	find_plugins_in_dir("Buffer/Sound/", Plugin::Type::EFFECT, this);
	find_plugins_in_dir("Buffer/Synthesizer/", Plugin::Type::EFFECT, this);

	// "Midi"
	find_plugins_in_dir("Midi/", Plugin::Type::MIDI_EFFECT, this);

	// "All"
	find_plugins_in_dir("All/", Plugin::Type::SONG_PLUGIN, this);

	// rest
	find_plugins_in_dir("Independent/", Plugin::Type::TSUNAMI_PLUGIN, this);

	// "Synthesizer"
	find_plugins_in_dir("Synthesizer/", Plugin::Type::SYNTHESIZER, this);
}

void PluginManager::AddPluginsToMenu(TsunamiWindow *win)
{
	hui::Menu *m = win->getMenu();

	// "Buffer"
	add_plugins_in_dir("Buffer/Channels/", this, m->getSubMenuByID("menu_plugins_channels"), "effect", win, &TsunamiWindow::onMenuExecuteEffect);
	add_plugins_in_dir("Buffer/Dynamics/", this, m->getSubMenuByID("menu_plugins_dynamics"), "effect", win, &TsunamiWindow::onMenuExecuteEffect);
	add_plugins_in_dir("Buffer/Echo/", this, m->getSubMenuByID("menu_plugins_echo"), "effect", win, &TsunamiWindow::onMenuExecuteEffect);
	add_plugins_in_dir("Buffer/Pitch/", this, m->getSubMenuByID("menu_plugins_pitch"), "effect", win, &TsunamiWindow::onMenuExecuteEffect);
	add_plugins_in_dir("Buffer/Repair/", this, m->getSubMenuByID("menu_plugins_repair"), "effect", win, &TsunamiWindow::onMenuExecuteEffect);
	add_plugins_in_dir("Buffer/Sound/", this, m->getSubMenuByID("menu_plugins_sound"), "effect", win, &TsunamiWindow::onMenuExecuteEffect);
	add_plugins_in_dir("Buffer/Synthesizer/", this, m->getSubMenuByID("menu_plugins_synthesizer"), "effect", win, &TsunamiWindow::onMenuExecuteEffect);

	// "Midi"
	add_plugins_in_dir("Midi/", this, m->getSubMenuByID("menu_plugins_on_midi"), "midi-effect", win, &TsunamiWindow::onMenuExecuteMidiEffect);

	// "All"
	add_plugins_in_dir("All/", this, m->getSubMenuByID("menu_plugins_on_all"), "song", win, &TsunamiWindow::onMenuExecuteSongPlugin);

	// rest
	add_plugins_in_dir("Independent/", this, m->getSubMenuByID("menu_plugins_other"), "tsunami", win, &TsunamiWindow::onMenuExecuteTsunamiPlugin);
}

void PluginManager::ApplyFavorite(Configurable *c, const string &name)
{
	favorites->Apply(c, name);
}

void PluginManager::SaveFavorite(Configurable *c, const string &name)
{
	favorites->Save(c, name);
}


string PluginManager::SelectFavoriteName(hui::Window *win, Configurable *c, bool save)
{
	return favorites->SelectName(win, c, save);
}

// always push the script... even if an error occurred
//   don't log error...
Plugin *PluginManager::LoadAndCompilePlugin(int type, const string &filename)
{
	for (Plugin *p: plugins)
		if (filename == p->filename)
			return p;

	//InitPluginData();

	Plugin *p = new Plugin(filename, type);
	p->index = plugins.num;

	plugins.add(p);

	return p;
}

typedef void main_void_func();

void PluginManager::_ExecutePlugin(Session *session, const string &filename)
{
	Plugin *p = LoadAndCompilePlugin(Plugin::Type::OTHER, filename);
	if (!p->usable){
		session->e(p->getError());
		return;
	}

	Kaba::Script *s = p->s;

	main_void_func *f_main = (main_void_func*)s->MatchFunction("main", "void", 0);
	if (f_main){
		f_main();
	}else{
		session->e(_("Plugin does not contain a function 'void main()'"));
	}
}


Plugin *PluginManager::GetPlugin(Session *session, int type, const string &name)
{
	for (PluginFile &pf: plugin_files){
		if ((pf.name == name) and (pf.type == type)){
			Plugin *p = LoadAndCompilePlugin(type, pf.filename);
			if (!p->usable)
				session->e(p->getError());
			return p;
		}
	}
	session->e(format(_("Can't find plugin: %s ..."), name.c_str()));
	return NULL;
}


Array<string> PluginManager::FindSynthesizers()
{
	Array<string> names;
	Array<DirEntry> list = dir_search(plugin_dir() + "Synthesizer/", "*.kaba", false);
	for (DirEntry &e: list)
		names.add(e.name.replace(".kaba", ""));
	names.add("Dummy");
	//names.add("Sample");
	return names;
}

Synthesizer *PluginManager::__LoadSynthesizer(Session *session, const string &name)
{
	string filename = plugin_dir() + "Synthesizer/" + name + ".kaba";
	if (!file_test_existence(filename))
		return NULL;
	Kaba::Script *s;
	try{
		s = Kaba::Load(filename);
	}catch(Kaba::Exception &e){
		session->e(e.message);
		return NULL;
	}
	for (auto *t : s->syntax->classes){
		if (t->get_root()->name != "Synthesizer")
			continue;
		Synthesizer *synth = (Synthesizer*)t->create_instance();
		synth->session = session;
		synth->song = session->song;
		synth->setSampleRate(session->song->sample_rate);
		return synth;
	}
	return NULL;
}
// factory
Synthesizer *PluginManager::CreateSynthesizer(Session *session, const string &name)
{
	if ((name == "Dummy") or (name == ""))
		return new DummySynthesizer;
	/*if (name == "Sample")
		return new SampleSynthesizer;*/
	Synthesizer *s = __LoadSynthesizer(session, name);
	if (s){
		s->name = name;
		s->session = session;
		s->song = session->song;
		s->resetConfig();
		return s;
	}
	session->e(_("unknown synthesizer: ") + name);
	s = new DummySynthesizer;
	s->session = session;
	s->song = session->song;
	s->name = name;
	return s;
}


string PluginManager::plugin_dir()
{
	if (tsunami->directory_static.find("/home/") == 0)
		return "Plugins/";
	return tsunami->directory_static + "Plugins/";
}

Array<string> PluginManager::FindEffects()
{
	Array<string> names;
	string prefix = plugin_dir() + "Buffer/";
	for (auto &pf: plugin_files){
		if (pf.filename.match(prefix + "*")){
			string g = pf.filename.substr(prefix.num, -1).explode("/")[0];
			names.add(g + "/" + pf.name);
		}
	}
	return names;
}

Array<string> PluginManager::FindMidiEffects()
{
	Array<string> names;
	for (auto &pf: plugin_files)
		if (pf.type == Plugin::Type::MIDI_EFFECT)
			names.add(pf.name);
	return names;
}

Array<string> PluginManager::FindConfigurable(int type)
{
	if (type == Configurable::Type::EFFECT)
		return FindEffects();
	if (type == Configurable::Type::MIDI_EFFECT)
		return FindMidiEffects();
	if (type == Configurable::Type::SYNTHESIZER)
		return FindSynthesizers();
	return Array<string>();
}


Effect* PluginManager::ChooseEffect(hui::Panel *parent, Session *session)
{
	ConfigurableSelectorDialog *dlg = new ConfigurableSelectorDialog(parent->win, Configurable::Type::EFFECT, session);
	dlg->run();
	Effect *e = (Effect*)dlg->_return;
	delete(dlg);
	return e;
}

MidiEffect* PluginManager::ChooseMidiEffect(hui::Panel *parent, Session *session)
{
	ConfigurableSelectorDialog *dlg = new ConfigurableSelectorDialog(parent->win, Configurable::Type::MIDI_EFFECT, session);
	dlg->run();
	MidiEffect *e = (MidiEffect*)dlg->_return;
	delete(dlg);
	return e;
}


Synthesizer *PluginManager::ChooseSynthesizer(hui::Window *parent, Session *session, const string &old_name)
{
	ConfigurableSelectorDialog *dlg = new ConfigurableSelectorDialog(parent, Configurable::Type::SYNTHESIZER, session, old_name);
	dlg->run();
	Synthesizer *s = (Synthesizer*)dlg->_return;
	delete(dlg);
	return s;
}

/*Synthesizer* PluginManager::ChooseSynthesizer(HuiPanel *parent)
{
	string name = ChooseConfigurable(parent, Configurable::Type::SYNTHESIZER);
	if (name == "")
		return NULL;
	return CreateSynthesizer(name);
}*/


