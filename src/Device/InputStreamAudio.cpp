/*
 * InputStreamAudio.cpp
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#include "../lib/hui/hui.h"
#include "../Tsunami.h"
#include "../TsunamiWindow.h"
#include "../Stuff/Log.h"
#include "../View/AudioView.h"

#include "DeviceManager.h"
#include "Device.h"
#include "InputStreamAudio.h"
#include "OutputStream.h"

#ifdef DEVICE_PULSEAUDIO
#include <pulse/pulseaudio.h>
#endif


string InputStreamAudio::backup_filename;
string InputStreamAudio::temp_backup_filename;
string InputStreamAudio::cur_backup_filename;
float InputStreamAudio::playback_delay_const;



#ifdef DEVICE_PULSEAUDIO
extern void pa_wait_op(pa_operation *op); // -> DeviceManager.cpp
extern bool pa_wait_stream_ready(pa_stream *s); // -> OutputStream.cpp
#endif

#ifdef DEVICE_PULSEAUDIO

void InputStreamAudio::input_request_callback(pa_stream *p, size_t nbytes, void *userdata)
{
	//printf("read %d\n", (int)nbytes);
	InputStreamAudio *input = (InputStreamAudio*)userdata;
	if (!input->isCapturing())
		return;

	const void *data;
	pa_stream_peek(p, &data, &nbytes);
	input->testError("pa_stream_peek");
	int frames = nbytes / 4 / input->num_channels;

	if (data){
		float *in = (float*)data;

		RingBuffer &buf = input->current_buffer;
		BufferBox b;
		buf.writeRef(b, frames);
		b.deinterleave(in, input->num_channels);

		int done = b.length;
		if (done < frames){
			buf.writeRef(b, frames - done);
			b.deinterleave(&in[2 * done], input->num_channels);
		}

		pa_stream_drop(p);
		input->testError("pa_stream_drop");
	}
	//msg_write(">");
}
void input_notify_callback(pa_stream *p, void *userdata)
{
	printf("sstate... %p:  ", p);
	int s = pa_stream_get_state(p);
	if (s == PA_STREAM_UNCONNECTED)
		printf("unconnected");
	if (s == PA_STREAM_READY)
		printf("ready");
	if (s == PA_STREAM_TERMINATED)
		printf("terminated");
	printf("\n");
}


void input_success_callback(pa_stream *s, int success, void *userdata)
{
	msg_write("--success");
}
#endif


void InputStreamAudio::SyncData::reset()
{
	num_points = 0;
	delay_sum = 0;
	samples_in = 0;
	/*if (tsunami->output->IsPlaying())
		offset_out = tsunami->output->GetRange().offset;*/ // TODO
}

void InputStreamAudio::SyncData::add(int samples)
{
	if (tsunami->win->view->stream->isPlaying()){
		samples_in += samples;
		/*delay_sum += (tsunami->output->GetPos() - offset_out - samples_in);*/ // TODO
		num_points ++;
	}
}

int InputStreamAudio::SyncData::getDelay()
{
	if (num_points > 0)
		return (delay_sum / num_points);
	return 0;
}


InputStreamAudio::InputStreamAudio(int _sample_rate) :
	InputStreamAny("InputStreamAudio", _sample_rate),
	current_buffer(1048576)
{
	capturing = false;
#ifdef DEVICE_PULSEAUDIO
	_stream = NULL;
#endif

	device = tsunami->device_manager->chooseDevice(Device::TYPE_AUDIO_INPUT);
	playback_delay_const = 0;
	if (device)
		playback_delay_const = device->latency;
	backup_filename = hui::Config.getStr("Input.TempFilename", "");
	backup_file = NULL;
	backup_mode = BACKUP_MODE_NONE;

	running = false;

	if (file_test_existence(getBackupFilename()))
		tsunami->log->warn(_("found old recording: ") + getBackupFilename());
}

InputStreamAudio::~InputStreamAudio()
{
	stop();
}

void InputStreamAudio::__init__(int _sample_rate)
{
	new(this) InputStreamAudio(_sample_rate);
}

void InputStreamAudio::__delete__()
{
	this->InputStreamAudio::~InputStreamAudio();
}

Device *InputStreamAudio::getDevice()
{
	return device;
}

void InputStreamAudio::setDevice(Device *_device)
{
	device = _device;
	playback_delay_const = device->latency;

	if (capturing){
		stop();
		start();
	}

	//tsunami->log->info(format("input device '%s' chosen", Pa_GetDeviceInfo(pa_device_no)->name));
}

void InputStreamAudio::stop()
{
	if (!capturing)
		return;
	_stopUpdate();

#ifdef DEVICE_PULSEAUDIO
	pa_stream_disconnect(_stream);
	testError("disconnect");

	for (int i=0; i<1000; i++){
		if (pa_stream_get_state(_stream) == PA_STREAM_TERMINATED)
			break;
		hui::Sleep(0.001f);
	}

	pa_stream_unref(_stream);
	testError("unref");
#endif

	capturing = false;
	accumulating = false;
	current_buffer.clear();
	if (backup_file){
		delete(backup_file);
		backup_file = NULL;
		if (backup_mode != BACKUP_MODE_KEEP)
			file_delete(cur_backup_filename);
	}
}

bool InputStreamAudio::start()
{
	if (capturing)
		stop();

	accumulating = false;
	num_channels = 2;

#ifdef DEVICE_PULSEAUDIO
	pa_sample_spec ss;
	ss.rate = sample_rate;
	ss.channels = 2;
	ss.format = PA_SAMPLE_FLOAT32LE;
	_stream = pa_stream_new(tsunami->device_manager->context, "stream-in", &ss, NULL);
	testError("pa_stream_new");


	pa_stream_set_read_callback(_stream, &input_request_callback, this);
	//pa_stream_set_state_callback(_stream, &input_notify_callback, NULL);

	pa_buffer_attr attr_in;
//	attr_in.fragsize = -1;
	attr_in.fragsize = chunk_size;
	attr_in.maxlength = -1;
	attr_in.minreq = -1;
	attr_in.tlength = -1;
	attr_in.prebuf = -1;
	const char *dev = NULL;
	if (!device->is_default())
		dev = device->internal_name.c_str();
	pa_stream_connect_record(_stream, dev, &attr_in, (pa_stream_flags_t)PA_STREAM_ADJUST_LATENCY);
	// without PA_STREAM_ADJUST_LATENCY, we will get big chunks (split into many small ones, but still "clustered")
	testError("pa_stream_connect_record");

	if (!pa_wait_stream_ready(_stream)){

		tsunami->log->error("pa_wait_for_stream_ready");
		return false;
	}
#endif

	capturing = true;

	if (backup_mode != BACKUP_MODE_NONE){
		cur_backup_filename = getBackupFilename();
		backup_file = FileCreate(getBackupFilename());
		backup_file->SetBinaryMode(true);
	}

	_startUpdate();

	resetSync();
	return capturing;
}

bool InputStreamAudio::testError(const string &msg)
{
#ifdef DEVICE_PULSEAUDIO
	int e = pa_context_errno(tsunami->device_manager->context);
	if (e != 0)
		tsunami->log->error(msg + " (input): " + pa_strerror(e));
	return (e != 0);
#endif
	return false;
}

float InputStreamAudio::getPlaybackDelayConst()
{
	return playback_delay_const;
}

void InputStreamAudio::accumulate(bool enable)
{
	//resetAccumulation();
	current_buffer.clear();
	accumulating = enable;
}

void InputStreamAudio::resetAccumulation()
{
	buffer.clear();
}

int InputStreamAudio::getSampleCount()
{
	return buffer.length;
}

void InputStreamAudio::setPlaybackDelayConst(float f)
{
	playback_delay_const = f;
	hui::Config.setFloat("Input.PlaybackDelay", playback_delay_const);
}

int InputStreamAudio::doCapturing()
{
	if (!capturing)
		return 0;

	int avail = current_buffer.available();
	sync.add(avail);

	if (!accumulating)
		return avail;

	BufferBox b;
	current_buffer.readRef(b, avail);
	buffer.append(b);

	if (backup_file){
		// write to file
		string data;
		b.exports(data, 2, SAMPLE_FORMAT_32_FLOAT);

		backup_file->WriteBuffer(&data[0], data.num);
	}

	return avail;
}

void InputStreamAudio::resetSync()
{
	sync.reset();
}

void InputStreamAudio::getSomeSamples(BufferBox &buf, int num_samples)
{
	current_buffer.peekRef(buf, -num_samples);
}

bool InputStreamAudio::isCapturing()
{
	return capturing;
}

int InputStreamAudio::getState()
{
	if (isCapturing())
		return STATE_PLAYING;
	return STATE_STOPPED;
}

int InputStreamAudio::getDelay()
{
	return sync.getDelay() - playback_delay_const * (float)sample_rate / 1000.0f;
}

string InputStreamAudio::getDefaultBackupFilename()
{
#ifdef OS_WINDOWS
	return "c:\\tsunami-input.raw";
#else
	return "/tmp/tsunami-input.raw";
#endif
}

string InputStreamAudio::getBackupFilename()
{
	if (temp_backup_filename.num > 0)
		return temp_backup_filename;
	if (backup_filename.num > 0)
		return backup_filename;
	return getDefaultBackupFilename();
}

void InputStreamAudio::setBackupFilename(const string &filename)
{
	backup_filename = filename;
	hui::Config.setStr("Input.TempFilename", backup_filename);
}

void InputStreamAudio::setTempBackupFilename(const string &filename)
{
	temp_backup_filename = filename;
}

void InputStreamAudio::_startUpdate()
{
	if (running)
		return;
	hui_runner_id = hui::RunRepeated(update_dt, std::bind(&InputStreamAudio::update, this));
	running = true;
}

void InputStreamAudio::_stopUpdate()
{
	if (!running)
		return;
	hui::CancelRunner(hui_runner_id);
	hui_runner_id = -1;
	running = false;
}

void InputStreamAudio::update()
{
	if (doCapturing() > 0)
		notify(MESSAGE_CAPTURE);

	running = isCapturing();
}
