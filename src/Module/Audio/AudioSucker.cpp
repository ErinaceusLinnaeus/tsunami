/*
 * AudioSucker.cpp
 *
 *  Created on: 17.09.2017
 *      Author: michi
 */

#include "AudioSucker.h"
#include "../../lib/threads/Thread.h"
#include "../../lib/hui/hui.h"
#include "../../Stuff/PerformanceMonitor.h"
#include "../Port/Port.h"
#include "../ModuleFactory.h"
#include "../../Data/base.h"
#include "../../Data/Audio/AudioBuffer.h"

const int AudioSucker::DEFAULT_BUFFER_SIZE = 1024;


class AudioSuckerThread : public Thread
{
public:
	AudioSucker *sucker;
	int perf_channel;
	bool keep_running = true;

	AudioSuckerThread(AudioSucker *s)
	{
		perf_channel = PerformanceMonitor::create_channel("suck");
		sucker = s;
	}
	~AudioSuckerThread()
	{
		PerformanceMonitor::delete_channel(perf_channel);
	}

	void on_run() override
	{
		//msg_write("thread run");
		while(keep_running){
			//msg_write(".");
			if (sucker->running){
				PerformanceMonitor::start_busy(perf_channel);
				int r = sucker->update();
				PerformanceMonitor::end_busy(perf_channel);
				if (r == Port::END_OF_STREAM)
					break;
				if (r == Port::NOT_ENOUGH_DATA){
					hui::Sleep(sucker->no_data_wait);
					continue;
				}
			}else{
				hui::Sleep(0.200f);
			}
			Thread::cancelation_point();
		}
		//msg_write("thread done...");
	}
};

AudioSucker::AudioSucker() :
	Module(ModuleType::PLUMBING, "AudioSucker")
{
	port_in.add(InPortDescription(SignalType::AUDIO, &source, "in"));
	source = nullptr;
	running = false;
	thread = nullptr;//new AudioSuckerThread(this);
	buffer_size = DEFAULT_BUFFER_SIZE;
	no_data_wait = 0.005f;

}

AudioSucker::~AudioSucker()
{
	if (thread){
		thread->keep_running = false;
		thread->join();
		//thread->kill();
		delete(thread);
		thread = nullptr;
	}
}

void AudioSucker::start()
{
	if (running)
		return;
	thread = new AudioSuckerThread(this);
	thread->run();
	running = true;
}

void AudioSucker::stop()
{
	if (thread){
		thread->keep_running = false;
		thread->join();
		delete thread;
		thread = nullptr;
	}
	running = false;
}

void AudioSucker::command(ModuleCommand cmd)
{
	if (cmd == ModuleCommand::START)
		start();
	else if (cmd == ModuleCommand::STOP)
		stop();
}

int AudioSucker::update()
{
	AudioBuffer temp;
	temp.resize(buffer_size);
	int r = source->read_audio(temp);
	if (r == source->NOT_ENOUGH_DATA)
		return r;
	if (r == source->END_OF_STREAM)
		return r;
	return r;
}

