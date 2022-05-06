#!/bin/bash

if [ -z "$ROCM_PATH" ]; then export ROCM_PATH=/opt/rocm; fi
if [ -z "$HSA_INCLUDE" ]; then export HSA_INCLUDE=/opt/rocm/include/hsa; fi
if [ -z "$HIP_PATH" ]; then export HIP_PATH=/opt/rocm/include/hip; fi
if [ -z "$ROCTRACER_PATH" ]; then export ROCTRACER_PATH=/opt/rocm/roctracer; fi
if [ -z "$ROCPROFILER_PATH" ]; then export ROCPROFILER_PATH=/opt/rocm/rocprofiler; fi


ARG_IN=""
while [ 1 ] ; do
  ARG_IN=$1
  if [ "$1" = "--barectf-generate" ] ; then
    barectf generate --headers-dir=./inc --code-dir=./src --metadata-dir=. ./config.yaml
    python3 ./scripts/configure_metadata.py metadata
  else
    break
  fi
  shift
done

#Generate headers and cpp files for parsing

python3 ./scripts/hsa_args_gen.py $ROCTRACER_PATH/include/hsa_prof_str.h
python3 ./scripts/hip_args_gen.py $HIP_PATH/amd_detail/hip_prof_str.h

mkdir -p obj
make all