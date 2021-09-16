# CTF Plugin for rocprofiler and roctracer

The goal of the plugin is to generate CTF traces at runtime with rocprofiler and roctracer and avoid conversion overhead.
The plugin currently allow to generate traces for HSA API/activity, HIP API/activity, KFD API and kernel events with counter metrics.
The plugin uses barectf and priority queues to sort and trace events provided by rocprofiler/roctracer with minimal overhead.


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
- multiple streams for each traced API with the following names : `<pid>_<traced API>_<stream_identifier>` where `<stream identifier>` is the tid of the tracing thread.


## To build the plugin

You will need the following things:
- python3
- rocprofiler files : <https://github.com/ROCm-Developer-Tools/rocprofiler>
- roctracer files : <https://github.com/ROCm-Developer-Tools/roctracer>

### Set the environment:

The plugin needs source and header files from roctracer and rocprofiler directories. You will need to set the following environment variables:
- `export HSA_INCLUDE=<path to hsa-runtime includes>` (/opt/rocm/include/hsa by default)
- `export ROCM_PATH=<path to rocm>` (/opt/rocm by default)
- `export HIP_PATH=<path to hip api>` (/opt/rocm/include/hip by default) this directory must contain `hcc_detail/hip_prof_str.h` file
- `export ROCTRACER_INCLUDES=<path to roctracer includes>`  (/opt/rocm/roctracer/include by default) this directory must contain `roctracer_<hip|hsa|kfd>.h`, `<hsa|kfd>_prof_str.h` and `<hip|hsa|kfd>_ostream_ops.h` files
- `export ROCPROFILER_INCLUDES=<path to rocprofiler includes>` (/opt/rocm/rocprofiler/include by default)

To build:
```
cd <your path/ctf_plugin> && ./build.sh
```
It will:
- generate cpp files with functions to convert APIs data to strings from `<hsa|kfd|hip>_prof_str.h`
- build the shared libraries `rocprofiler_plugin_tool.so` and `roctracer_plugin_tool.so` with functions that will be loaded from tool.cpp and tracer_tool.cpp files in `rocprofiler/roctracer` 

Currently you have to manually modify rocprofiler/roctracer to allow the use of the module. To do so:
- replace `tracer_tool.cpp` file in `roctracer/test/tool` by `tracer_tool.cpp` in `rocprofiler_roctracer_files` directory 
- replace `tool.cpp` file in `rocprofiler/test/tool by tool.cpp` in `rocprofiler_roctracer_files` directory

Rebuild libtracer_tool.so and libtool.so in roctracer and rocprofiler with those 2 new files.
Those new tool files will overload flushing functions in current roctracer/rocprofiler implementation with the plugin functions.
- replace `rpl_run.sh` file in `rocprofiler/bin` by `rpl_run.sh` in `rocprofiler_roctracer_files` directory
This new script will instanciate an environment variable if --output-plugin option is given in rocprof command

