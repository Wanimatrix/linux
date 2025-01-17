// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 Facebook

#include <linux/ptrace.h>
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

struct {
	__uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
	__type(key, int);
	__type(value, int);
} perf_buf_map SEC(".maps");

SEC("tp/syscalls/sys_enter_nanosleep")
int handle_sys_enter(void *ctx)
{
	int cpu = bpf_get_smp_processor_id();

	bpf_perf_event_output(ctx, &perf_buf_map, BPF_F_CURRENT_CPU,
			      &cpu, sizeof(cpu));
	return 0;
}

char _license[] SEC("license") = "GPL";
