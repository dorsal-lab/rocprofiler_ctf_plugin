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

#include "tracer.h"
#include "rocprofiler_tracers.h"
#include "rocprofiler_trace_entries.h"
#include <string.h>

void trace_kernel_event(kernel_event_t *kernel_event, struct barectf_default_ctx *ctx)
{
    barectf_default_trace_kernel_event(ctx,
									kernel_event->dispatch,
                                    kernel_event->gpu_id,
                                    kernel_event->queue_id,
                                    kernel_event->queue_index,
                                    kernel_event->pid,
                                    kernel_event->tid,
                                    kernel_event->grd,
                                    kernel_event->wgr,
                                    kernel_event->lds,
                                    kernel_event->scr,
                                    kernel_event->vgpr,
                                    kernel_event->sgpr,
                                    kernel_event->fbar,
                                    kernel_event->sig,
                                    kernel_event->obj,
                                    kernel_event->kernel_name,
                                    kernel_event->dispatch_time,
                                    kernel_event->complete_time,
									kernel_event->end);
	free(kernel_event->kernel_name);
}

void Kernel_Event_Tracer::kernel_flush_cb(kernel_trace_entry_t* entry)
{
  kernel_event_t *kernel_event = new kernel_event_t(entry->begin,
	entry->dispatch,
	entry->gpu_id,
	entry->queue_id,
	entry->queue_index,
	entry->pid, entry->tid,
	entry->grid_size,
	entry->workgroup_size,
	entry->lds_size,
	entry->scratch_size,
	entry->vgpr,
	entry->sgpr,
	entry->fbarrier_count,
	entry->signal_handle,
	entry->object,
	strdup(entry->kernel_name),
	entry->dispatch_time,
	entry->end,
	entry->complete);
  callback(entry->begin, (tracing_function)trace_kernel_event, kernel_event);
}
