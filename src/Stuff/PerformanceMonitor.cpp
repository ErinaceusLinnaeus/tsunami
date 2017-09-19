/*
 * PerformanceMonitor.cpp
 *
 *  Created on: 18.09.2017
 *      Author: michi
 */

#include "PerformanceMonitor.h"
#include "../lib/hui/hui.h"

#define ALLOW_PERF_MON	0

#if ALLOW_PERF_MON
struct Channel
{
	string name;
	float cpu_usage;
	float t_busy, t_idle, t_last;
	bool used;
	hui::Timer timer;
	int state;
	void reset()
	{
		t_busy = t_idle = 0;
		state = -1;
	}
	void update()
	{
		cpu_usage = t_busy / (t_busy + t_idle);
	}
};
static Array<Channel> channels;

static void reset()
{
	//timer.get();
	for (auto c: channels){
		c.t_busy = c.t_idle = c.t_last = 0;
		c.state = -1;
	}
}

static void show()
{
	printf("----- cpu usage -----\n");
	for (auto c: channels)
		if (c.used){
			float cpu_usage = c.t_busy / (c.t_busy + c.t_idle);
			printf("[%s]: %.1f%%\n", c.name.c_str(), c.cpu_usage * 100.0f);
		}
	reset();
}
#endif

void PerformanceMonitor::init()
{
#if ALLOW_PERF_MON
	reset();
#endif
}

int PerformanceMonitor::create_channel(const string &name)
{
#if ALLOW_PERF_MON
	foreachi(Channel &c, channels, i)
		if (!c.used){
			c.used = true;
			c.name = name;
			c.t_busy = c.t_idle = c.t_last = 0;
			c.state = -1;
			return i;
		}
	Channel c;
	c.name = name;
	c.t_busy = c.t_idle = c.t_last = 0;
	c.state = -1;
	c.used = true;
	channels.add(c);
	return channels.num - 1;
#else
	return -1;
#endif
}

void PerformanceMonitor::delete_channel(int channel)
{
#if ALLOW_PERF_MON
	channels[channel].used = false;
#endif
}

void PerformanceMonitor::start_busy(int channel)
{
#if ALLOW_PERF_MON
	//float t = timer.peek();
	//printf("+ %d %f\n", channel, t);
	auto &c = channels[channel];
	float dt = c.timer.get();
	c.t_idle += dt;//t - c.t_last;
	//c.t_last = t;
	c.state = 1;
	//if (t > 5)
	//	show();
#endif
}

void PerformanceMonitor::end_busy(int channel)
{
#if ALLOW_PERF_MON
	//float t = timer.peek();
	auto &c = channels[channel];
	//printf("- %d %f\n", channel, t);
	float dt = c.timer.get();
	c.t_busy += dt;// t - c.t_last;
	//c.t_last = t;
	c.state = 0;
	//if (t > 5)
	//	show();
#endif
}
