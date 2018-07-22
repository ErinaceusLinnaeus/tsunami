/*
 * OutputStream.h
 *
 *  Created on: 01.11.2014
 *      Author: michi
 */

#ifndef SRC_DEVICE_OUTPUTSTREAM_H_
#define SRC_DEVICE_OUTPUTSTREAM_H_



#include "../lib/base/base.h"
#include "../lib/hui/hui.h"
#include "../Data/Song.h"
#include "../Data/Audio/RingBuffer.h"
#include "../Module/Module.h"
#include <atomic>

class AudioPort;
class DeviceManager;
class Device;
class Thread;
class StreamThread;
class Session;

#if HAS_LIB_PULSEAUDIO
struct pa_stream;
#endif

#if HAS_LIB_PORTAUDIO
typedef void PaStream;
struct PaStreamCallbackTimeInfo;
typedef unsigned long PaStreamCallbackFlags;
typedef int PaError;
#endif

class OutputStream : public Module
{
	friend StreamThread;
public:
	//AudioStream();
	OutputStream(Session *session, AudioPort *r);
	virtual ~OutputStream();

	void _cdecl __init__(Session *session, AudioPort *r);
	virtual void _cdecl __delete__();

	void _create_dev();
	void _kill_dev();

	int api;

	static const string MESSAGE_STATE_CHANGE;
	static const string MESSAGE_READ_END_OF_STREAM;
	static const string MESSAGE_PLAY_END_OF_STREAM;
	static const string MESSAGE_UPDATE;


	void _cdecl stop();
	void _cdecl play();
	void _cdecl pause(bool pause);
	void _cdecl update();

	bool fully_initialized;
	void _start_first_time();
	void _unpause();
	void _pause();

	bool _cdecl is_paused();
	void _cdecl set_source(AudioPort *r);
	void _cdecl set_device(Device *d);
	int _cdecl get_pos();

	void _cdecl clear_buffer();

	float _cdecl get_volume();
	void _cdecl set_volume(float _volume);

	void _cdecl set_buffer_size(int _size){ buffer_size = _size; }

private:
	void _read_stream();

	float volume;
	bool paused;
	int buffer_size;
	float update_dt;
	int hui_runner_id;

	AudioPort *source;
	RingBuffer ring_buf;

	std::atomic<bool> read_end_of_stream;
	std::atomic<bool> played_end_of_stream;

	int data_samples;

#if HAS_LIB_PULSEAUDIO
	pa_stream *pulse_stream;
	bool _pulse_test_error(const string &msg);
#endif

#if HAS_LIB_PORTAUDIO
	PaStream *portaudio_stream;
	bool _portaudio_test_error(PaError err, const string &msg);
#endif

	int dev_sample_rate;

	DeviceManager *device_manager;
	Device *device;
	bool killed;

	Thread *thread;
	int perf_channel;

#if HAS_LIB_PULSEAUDIO
	static void pulse_stream_request_callback(pa_stream *p, size_t nbytes, void *userdata);
	static void pulse_stream_underflow_callback(pa_stream *s, void *userdata);
	static void pulse_stream_success_callback(pa_stream *s, int success, void *userdata);
#endif

#if HAS_LIB_PORTAUDIO
	static int portaudio_stream_request_callback(const void *inputBuffer, void *outputBuffer,
	                                             unsigned long frames,
	                                             const PaStreamCallbackTimeInfo* timeInfo,
	                                             PaStreamCallbackFlags statusFlags,
	                                             void *userData);
#endif

	void on_played_end_of_stream();
	void on_read_end_of_stream();


public:
	virtual void module_start(){ play(); }
	virtual void module_stop(){ stop(); }
	virtual void module_pause(bool paused){ pause(paused); }
};

#endif /* SRC_DEVICE_OUTPUTSTREAM_H_ */
