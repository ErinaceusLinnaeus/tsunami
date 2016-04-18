/*
 * AudioInputMidi.cpp
 *
 *  Created on: 19.02.2013
 *      Author: michi
 */

#include "AudioInputMidi.h"
#include "AudioStream.h"
#include "Synth/Synthesizer.h"
#include "Renderer/SynthesizerRenderer.h"
#include "../Tsunami.h"
#include "../Stuff/Log.h"
#include <alsa/asoundlib.h>
#include "Device.h"
#include "DeviceManager.h"


static const float DEFAULT_UPDATE_TIME = 0.005f;
const string AudioInputMidi::MESSAGE_CAPTURE = "Capture";


AudioInputMidi::MidiPort::MidiPort()
{
	client = port = -1;
}


AudioInputMidi::AudioInputMidi(int _sample_rate) :
	PeakMeterSource("AudioInputMidi")
{
	subs = NULL;
	sample_rate = _sample_rate;
	update_dt = DEFAULT_UPDATE_TIME;
	chunk_size = 512;

	running = false;

	device_manager = tsunami->device_manager;

	init();


	preview_renderer = new SynthesizerRenderer(NULL);
	preview_stream = new AudioStream(preview_renderer);
	preview_stream->setBufferSize(2048);
}

AudioInputMidi::~AudioInputMidi()
{
	stop();
	if (subs)
		unconnect();
	delete(preview_renderer);
	delete(preview_stream);
}

void AudioInputMidi::init()
{
}

void AudioInputMidi::setPreviewSynthesizer(Synthesizer *s)
{
	preview_renderer->setSynthesizer(s);
	/*preview_renderer->setAutoStop(false);
	if (s and capturing)
		preview_stream->play();*/
}

bool AudioInputMidi::connectMidiPort(AudioInputMidi::MidiPort &p)
{
	if (subs)
		unconnect();

	if ((p.client < 0) or (p.port < 0))
		return true;

	snd_seq_addr_t sender, dest;
	sender.client = p.client;
	sender.port = p.port;
	dest.client = snd_seq_client_id(device_manager->handle);
	dest.port = device_manager->portid;
	cur_midi_port = p;

	snd_seq_port_subscribe_malloc(&subs);
	snd_seq_port_subscribe_set_sender(subs, &sender);
	snd_seq_port_subscribe_set_dest(subs, &dest);
	int r = snd_seq_subscribe_port(device_manager->handle, subs);
	if (r != 0){
		tsunami->log->error(string("Error connecting to midi port: ") + snd_strerror(r));
		snd_seq_port_subscribe_free(subs);
		subs = NULL;
		cur_midi_port = no_midi_port;
	}
	return r == 0;

	// simple version raises "no permission" error...?!?
	/*int r = snd_seq_connect_to(handle, portid, p.client, p.port);
	if (r != 0)
		tsunami->log->Error(string("Error connecting to midi port: ") + snd_strerror(r));
	return r == 0;*/
}

bool AudioInputMidi::unconnect()
{
	if (!subs)
		return true;
	int r = snd_seq_unsubscribe_port(device_manager->handle, subs);
	if (r != 0)
		tsunami->log->error(string("Error unconnecting from midi port: ") + snd_strerror(r));
	snd_seq_port_subscribe_free(subs);
	subs = NULL;
	cur_midi_port = no_midi_port;
	return r == 0;
}

AudioInputMidi::MidiPort AudioInputMidi::getCurMidiPort()
{
	return cur_midi_port;
}


Array<AudioInputMidi::MidiPort> AudioInputMidi::findMidiPorts()
{
	Array<MidiPort> ports;
	snd_seq_client_info_t *cinfo;
	snd_seq_port_info_t *pinfo;

	snd_seq_client_info_alloca(&cinfo);
	snd_seq_port_info_alloca(&pinfo);
	snd_seq_client_info_set_client(cinfo, -1);
	while (snd_seq_query_next_client(device_manager->handle, cinfo) >= 0){
		snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
		snd_seq_port_info_set_port(pinfo, -1);
		while (snd_seq_query_next_port(device_manager->handle, pinfo) >= 0){
			if ((snd_seq_port_info_get_capability(pinfo) & SND_SEQ_PORT_CAP_READ) == 0)
				continue;
			if ((snd_seq_port_info_get_capability(pinfo) & SND_SEQ_PORT_CAP_SUBS_READ) == 0)
				continue;
			MidiPort p;
			p.client = snd_seq_client_info_get_client(cinfo);
			p.client_name = snd_seq_client_info_get_name(cinfo);
			p.port = snd_seq_port_info_get_port(pinfo);
			p.port_name = snd_seq_port_info_get_name(pinfo);
			ports.add(p);
		}
	}
	return ports;
}

void AudioInputMidi::setDevice(Device *d)
{
	device = d;
}

Device *AudioInputMidi::getDevice()
{
	return device;
}

void AudioInputMidi::accumulate(bool enable)
{
	accumulating = enable;
}

void AudioInputMidi::resetAccumulation()
{
	midi.clear();
	midi.samples = 0;
	offset = 0;
}

int AudioInputMidi::getSampleCount()
{
	return offset * (double)sample_rate;
}

void AudioInputMidi::clearInputQueue()
{
	while (true){
		snd_seq_event_t *ev;
		int r = snd_seq_event_input(device_manager->handle, &ev);
		if (r < 0)
			break;
		snd_seq_free_event(ev);
	}
}

bool AudioInputMidi::start()
{
	if (!device_manager->handle)
		return false;
	accumulating = false;
	offset = 0;
	resetAccumulation();

	clearInputQueue();
	/*preview_renderer->setAutoStop(false);
	if (preview_renderer->getSynthesizer())
		preview_stream->play();*/

	timer.reset();

	_startUpdate();
	capturing = true;
	return true;
}

void AudioInputMidi::stop()
{
	_stopUpdate();
	capturing = false;
	//preview_renderer->setAutoStop(true);
	preview_renderer->endAllNotes();
	//preview_stream->stop();

	midi.sanify(Range(0, midi.samples));
}

int AudioInputMidi::doCapturing()
{
	double dt = timer.get();
	double offset_new = offset + dt;
	int pos = offset * (double)sample_rate;
	int pos_new = offset_new * (double)sample_rate;
	current_midi.clear();
	current_midi.samples = pos_new - pos;
	if (accumulating)
		offset = offset_new;

	while (true){
		snd_seq_event_t *ev;
		int r = snd_seq_event_input(device_manager->handle, &ev);
		if (r < 0)
			break;
		int pitch = ev->data.note.note;
		switch (ev->type) {
			case SND_SEQ_EVENT_NOTEON:
				current_midi.add(MidiEvent(0, pitch, (float)ev->data.note.velocity / 127.0f));
				break;
			case SND_SEQ_EVENT_NOTEOFF:
				current_midi.add(MidiEvent(0, pitch, 0));
				break;
		}
		snd_seq_free_event(ev);
	}

	if (current_midi.num > 0)
		preview_renderer->feed(current_midi);
	if ((current_midi.num > 0) and (!preview_stream->isPlaying()))
		preview_stream->play();

	if (accumulating)
		midi.append(current_midi);

	return current_midi.samples;
}

bool AudioInputMidi::isCapturing()
{
	return capturing;
}

int AudioInputMidi::getState()
{
	if (isCapturing())
		return STATE_PLAYING;
	return STATE_STOPPED;
}

void AudioInputMidi::getSomeSamples(BufferBox &buf, int num_samples)
{
	preview_stream->getSomeSamples(buf, num_samples);
}


int AudioInputMidi::getDelay()
{
	return 0;
}

void AudioInputMidi::resetSync()
{
}

void AudioInputMidi::_startUpdate()
{
	if (running)
		return;
	hui_runner_id = HuiRunRepeatedM(update_dt, this, &AudioInputMidi::update);
	running = true;
}

void AudioInputMidi::_stopUpdate()
{
	if (!running)
		return;
	HuiCancelRunner(hui_runner_id);
	hui_runner_id = -1;
	running = false;
}

void AudioInputMidi::update()
{
	if (doCapturing() > 0)
		notify(MESSAGE_CAPTURE);

	running = isCapturing();
}

float AudioInputMidi::getSampleRate()
{
	return sample_rate;
}

void AudioInputMidi::setUpdateDt(float dt)
{
	if (dt > 0)
		update_dt = dt;
	else
		update_dt = DEFAULT_UPDATE_TIME;
}

void AudioInputMidi::setChunkSize(int size)
{
	if (size > 0)
		chunk_size = size;
	else
		chunk_size = 512;
}
