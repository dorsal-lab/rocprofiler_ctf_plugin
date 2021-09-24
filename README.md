# CTF Plugin for rocprofiler and roctracer

The goal of the plugin is to generate CTF traces at runtime with rocprofiler and roctracer and avoid conversion overhead.
The plugin currently allow to generate traces for rocTX, HSA API/activity, HIP API/activity, KFD API and kernel events with counter metrics.
The plugin uses barectf and priority queues to sort and trace events provided by ROCProfiler/ROCTracer with minimal overhead.


## Usage

To generate CTF traces you need to run the rocprof command with the --output-plugin option, followed by the path to the plugin directory and an output directory for the traces (-d option):

```rocprof --output-plugin ctf/plugin/directory --hsa-trace -d my_traces my_program```

Finally, to compute and write the offsets of the trace and the number of events in the metadata file, you must run the post_processing.py python script located in the ```scripts``` directory with the trace directory as argument:
```
python ctf/plugin/directory/scripts/post_processing.py my_traces
```

## Informations about the generated traces

For each API, the plugin will generate interval events sorted by beginning with an end timestamp field.
When generating a CTF trace, there will be the following files in the CTF_trace directory lying in the output directory:
- a metadata file
- a stream named `metrics_stream` containing the names of the collected metrics 
- a stream named `strings_association_stream` containing events that associates the cids of the traced functions to their names
- multiple streams for each traced API with the following names : `<pid>_<traced API>_<stream_identifier>` where `<stream identifier>` is an unique identifier.


## To build the plugin

You will need the following things:
- python3
- ROCTracer with interface implementation  : <https://github.com/dorsal-lab/roctracer/tree/rocm-4.3.x-PR5> (branch rocm-4.3.x-PR5)
- ROCProfiler with interface implementation : <https://github.com/dorsal-lab/rocprofiler/tree/rocm-4.3.x-PR5> (branch rocm-4.3.x-PR5)

### Set the environment:

The plugin needs header files from ROCm. You will need to set the following environment variables:
- `export HSA_INCLUDE=<path to hsa-runtime includes>` (/opt/rocm/include/hsa by default)
- `export ROCM_PATH=<path to rocm>` (/opt/rocm by default)
- `export HIP_PATH=<path to hip api>` (/opt/rocm/include/hip by default) this directory must contain `hcc_detail/hip_prof_str.h` file


To build:
```
cd <your path/ctf_plugin> && ./build.sh
```
It will:
- generate cpp files with functions to convert APIs data to strings from `<hsa|kfd|hip>_prof_str.h`
- build the shared libraries `rocprofiler_plugin_lib.so` and `roctracer_plugin_lib.so` with functions that will be loaded from tool.cpp and tracer_tool.cpp files in `rocprofiler/roctracer` 

