if [ -z "$ROCM_PATH" ]; then export ROCM_PATH=/opt/rocm; fi
if [ -z "$HSA_INCLUDE" ]; then export HSA_INCLUDE=/opt/rocm/include/hsa; fi
if [ -z "$HIP_PATH" ]; then export HIP_PATH=/opt/rocm/include/hip; fi

#Generate headers and cpp files for parsing
python3 ./scripts/kfd_args_gen.py $ROCM_PATH/include/roctracer/kfd_prof_str.h
python3 ./scripts/hsa_args_gen.py $ROCM_PATH/include/roctracer/hsa_prof_str.h
python3 ./scripts/hip_args_gen.py $HIP_PATH/hcc_detail/hip_prof_str.h

mkdir -p obj
make all