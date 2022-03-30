/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Yoann Heitz <yoann.heitz@polymtl.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "barectf-platform-linux-fs.h"
#include "barectf.h"
#include "tracer.h"
#include "rocprofiler_tracers.h"
#include "rocprofiler_trace_entries.h"
#include "rocprofiler.h"
#include "utils.h"
#include <time.h>
#include <string.h>
#include <fstream>

#define SECONDS_TO_NANOSECONDS 1000000000

barectf_platform_linux_fs_ctx *platform_metrics;
barectf_default_ctx *ctx_metrics;

barectf_platform_linux_fs_ctx *platform_metrics_names;
barectf_default_ctx *ctx_metrics_names;

uint64_t metrics_clock = 0;
uint64_t metrics_names_ts = 0;
uint32_t metric_id = 0;
uint32_t metrics_number = 0;
uint64_t nb_events = 0;
uint64_t kernel_id = 0;

bool metrics_set_empty = true;
bool metrics_names_flushed = false;
bool first_kernel_traced = false;
bool second_kernel_traced = false;

const char *output_dir;
thread_local uint32_t dev_index = 0;
bool kernel_event_initialized = false;
Kernel_Event_Tracer *kernel_event_tracer;
struct timespec tp;


//Initialize kernel events tracing
extern "C" void init_plugin_lib(const char *prefix, std::vector<std::string> metrics_vector)
{
	if (!kernel_event_initialized)
	{
		output_dir = prefix;
		initialize_trace_directory(output_dir);
		kernel_event_tracer = new Kernel_Event_Tracer(prefix, "kernel_events_");
		kernel_event_initialized = true;
		if(!metrics_vector.empty()){
			//Initialize metrics streams

			std::stringstream ss_metrics_names_stream;
			ss_metrics_names_stream << prefix << "/rocprof_ctf_trace/" << GetPid() << "_metrics_names";
			clock_gettime(CLOCK_MONOTONIC, &tp);
			metrics_names_ts = SECONDS_TO_NANOSECONDS * tp.tv_sec + tp.tv_nsec;
			platform_metrics_names = barectf_platform_linux_fs_init(15000, ss_metrics_names_stream.str().c_str(), 0, 0, 0, &metrics_names_ts);
			ctx_metrics_names = barectf_platform_linux_fs_get_barectf_ctx(platform_metrics_names);

			std::stringstream ss_metrics_stream;
			ss_metrics_stream << prefix << "/rocprof_ctf_trace/" << GetPid() << "_metrics";
			platform_metrics = barectf_platform_linux_fs_init(15000, ss_metrics_stream.str().c_str(), 0, 0, 0, &metrics_clock);
			ctx_metrics = barectf_platform_linux_fs_get_barectf_ctx(platform_metrics);
		}
	}
	else
	{
		printf("kernel events tracing already initialized");
	}
}

void write_nb_events()
{
	std::ostringstream outData;
	std::stringstream ss_metadata;
	ss_metadata << output_dir << "/rocprof_ctf_trace/" << GetPid() << "_rocprofiler_nb_events.txt";
	outData << nb_events;
	std::ofstream out_file(ss_metadata.str());
	out_file << outData.str();
}

extern "C" void kernel_flush_cb(kernel_trace_entry_t* entry)
{
	kernel_id++;
	dev_index = entry->gpu_id;
	kernel_event_tracer->kernel_flush_cb(entry);
}

//Write metrics for a kernel event
extern "C" void metric_flush_cb(metric_trace_entry_t *entry)
{
	if (kernel_id == 1)
	{
		barectf_trace_metric_name(ctx_metrics_names, metric_id, entry->name);
		metrics_number++;
		nb_events++;
	}
	
	clock_gettime(CLOCK_MONOTONIC, &tp);
	metrics_clock = SECONDS_TO_NANOSECONDS * tp.tv_sec + tp.tv_nsec;
	switch (entry->metric_type) {
		case ROCPROFILER_DATA_KIND_INT64 : {
			barectf_trace_metric_uint64(ctx_metrics, metric_id % metrics_number, entry->dispatch, dev_index, entry->result_uint64);
			nb_events++;}
			break;
		case ROCPROFILER_DATA_KIND_DOUBLE : {
			barectf_trace_metric_double(ctx_metrics, metric_id % metrics_number, entry->dispatch, dev_index, entry->result_double);
			nb_events++;}
			break;
		default:
			break;
		}
	metric_id++;
}

extern "C" void close_plugin_lib()
{
	if (kernel_event_initialized)
	{
		kernel_event_tracer->flush((Tracer<kernel_event_t>::tracing_function)trace_kernel_event);
		barectf_platform_linux_fs_fini(platform_metrics);
		barectf_trace_metric_name_end(ctx_metrics_names);
		nb_events++;
		barectf_platform_linux_fs_fini(platform_metrics_names);
		if (kernel_event_tracer != NULL)
		{
			nb_events += kernel_event_tracer->get_nb_events();
			write_nb_events();
			delete kernel_event_tracer;
		}
	}
}