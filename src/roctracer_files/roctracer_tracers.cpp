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

#include <tracer.h>
#include "roctracer_trace_entries.h"
#include <roctracer_tracers.h>
#include <roctracer_hsa_aux.h>
#include <roctracer_hip_aux.h>
#include <hsa_args_str.h>
#include <hip_args_str.h>
#include <mutex>
#include <cxxabi.h>

static inline bool is_hip_kernel_launch_api(const uint32_t &cid)
{
	bool ret =
		(cid == HIP_API_ID_hipLaunchKernel) ||
		(cid == HIP_API_ID_hipLaunchCooperativeKernel) ||
		(cid == HIP_API_ID_hipLaunchCooperativeKernelMultiDevice) ||
		(cid == HIP_API_ID_hipExtLaunchMultiKernelMultiDevice) ||
		(cid == HIP_API_ID_hipModuleLaunchKernel) ||
		(cid == HIP_API_ID_hipExtModuleLaunchKernel) ||
		(cid == HIP_API_ID_hipHccModuleLaunchKernel);
	return ret;
}

static inline const char *cxx_demangle(const char *symbol)
{
	size_t funcnamesize;
	int status;
	const char *ret = (symbol != NULL) ? abi::__cxa_demangle(symbol, NULL, &funcnamesize, &status) : symbol;
	return (ret != NULL) ? ret : strdup(symbol);
}

//rocTX tracing function
void trace_roctx(roctx_event_t *roctx_event, struct barectf_default_ctx *ctx)
{
	if(roctx_event->message == NULL){
		barectf_trace_roctx(ctx, roctx_event->cid, roctx_event->pid, roctx_event->tid, roctx_event->rid, "");
	}else{
		barectf_trace_roctx(ctx, roctx_event->cid, roctx_event->pid, roctx_event->tid, roctx_event->rid, roctx_event->message);
	}
}

void rocTX_Tracer::roctx_flush_cb(roctx_trace_entry_t *entry)
{
	callback(entry->time, (tracing_function)trace_roctx, new roctx_event_t(entry->time, entry->tid, entry->cid, entry->pid, entry->rid, entry->message));
}

//HSA API tracing function
void trace_hsa_api(hsa_api_event_t *hsa_api_event, struct barectf_default_ctx *ctx)
{
	hsa_api_string_pair_t arguments = hsa_api_pair_of_args(hsa_api_event->cid, hsa_api_event->data);
	barectf_trace_hsa_api(ctx, hsa_api_event->cid, hsa_api_event->pid, hsa_api_event->tid, arguments.first.c_str(), arguments.second.c_str(), hsa_api_event->end);
}

void HSA_API_Tracer::hsa_api_flush_cb(hsa_api_trace_entry_t *entry)
{
	callback(entry->begin, (tracing_function)trace_hsa_api, new hsa_api_event_t(entry->begin, entry->tid, entry->cid, entry->pid, entry->data, entry->end));
}

//HSA activity tracing function
void trace_hsa_activity(hsa_activity_event_t *hsa_activity_event, struct barectf_default_ctx *ctx)
{
	barectf_trace_hsa_activity(ctx, hsa_activity_event->pid, hsa_activity_event->record_index, hsa_activity_event->end_ns);
}

void HSA_Activity_Tracer::hsa_activity_flush_cb(hsa_activity_trace_entry_t *entry)
{
	callback(entry->record->begin_ns, (tracing_function)trace_hsa_activity, new hsa_activity_event_t(entry->record->begin_ns, entry->pid, entry->index, entry->record->end_ns));
}

//HIP API tracing function
void trace_hip_api(hip_api_event_t *hip_api_event, struct barectf_default_ctx *ctx)
{
	if(hip_api_event->domain == ACTIVITY_DOMAIN_EXT_API){
		barectf_trace_hip_api(ctx, UINT32_MAX, hip_api_event->pid, hip_api_event->tid, hip_api_event->name, hip_api_event->end);
	}else{
		std::stringstream ss;
		std::string arguments = hip_api_arguments(hip_api_event->cid, &(hip_api_event->data));
		ss << arguments;
		if (is_hip_kernel_launch_api(hip_api_event->cid) && hip_api_event->name)
		{
			const char *kernel_name = cxx_demangle(hip_api_event->name);
			ss << ", " << kernel_name;
			free((char*) kernel_name);
		}
		ss << ", " << (hip_api_event->data).correlation_id;
		barectf_trace_hip_api(ctx, hip_api_event->cid, hip_api_event->pid, hip_api_event->tid, ss.str().c_str(), hip_api_event->end);
	}

}

void HIP_API_Tracer::hip_api_flush_cb(hip_api_trace_entry_t *entry)
{
	callback(entry->begin, (tracing_function)trace_hip_api, new hip_api_event_t(entry->begin, entry->tid, entry->cid, entry->pid, entry->data, entry->name, entry->end, entry->domain));
}

//HIP activity callback function
void trace_hip_activity(hip_activity_event_t *hip_activity_event, struct barectf_default_ctx *ctx)
{
	roctracer_record_t record = hip_activity_event->record;
	barectf_trace_hip_activity(ctx, record.device_id, record.queue_id, hip_activity_event->name, record.correlation_id, hip_activity_event->pid, record.end_ns);
}

void HIP_Activity_Tracer::hip_activity_flush_cb(hip_activity_trace_entry_t *entry)
{
	callback(entry->record->begin_ns, (tracing_function)trace_hip_activity, new hip_activity_event_t(entry->record->begin_ns, entry->name, *(entry->record), entry->pid));
}
