CC = clang
CXX = clang++
BASIC_FLAGS = -fPIC -O3 -Wall
CXXFLAGS = $(BASIC_FLAGS) -std=c++11
RTR_FLAGS = $(CXXFLAGS) -D __HIP_PLATFORM_HCC__=1 -D __HIP_ROCclr__=1 -D HIP_VDI=1 -D AMD_INTERNAL_BUILD
AUX_FLAGS = $(RTR_FLAGS) -D HIP_PROF_HIP_API_STRING -D PROF_API_IMPL 
LINKFLAGS = --shared -lc -lstdc++
CIMP = -I ./inc -I $(HSA_INCLUDE) -I $(ROCM_PATH)include -I $(ROCM_PATH)include/roctracer
CIMP2 = -I ./inc -I $(HSA_INCLUDE)  -I $(ROCM_PATH)include/rocprofiler
SRC_DIR = src
AUX_DIR := src/aux
ROCTRACER_FILES_DIR := src/roctracer_files
ROCPROFILER_FILES_DIR := src/rocprofiler_files
OBJ_DIR:= obj
C_NAMES := barectf barectf-platform-linux-fs
AUX_NAMES := roctracer_hip_aux roctracer_hsa_aux roctracer_kfd_aux
CPP_NAMES := hsa_args_str kfd_args_str hip_args_str utils
ROCTRACER_NAMES := roctracer_tool roctracer_tracers
ROCPROFILER_NAMES := rocprofiler_tool rocprofiler_tracers
C_OBJECTS := $(addsuffix .o, $(addprefix $(OBJ_DIR)/, $(C_NAMES)))
AUX_OBJECTS := $(addsuffix .o, $(addprefix $(OBJ_DIR)/, $(AUX_NAMES)))
CPP_OBJECTS := $(addsuffix .o, $(addprefix $(OBJ_DIR)/, $(CPP_NAMES)))
ROCTRACER_OBJECTS := $(addsuffix .o, $(addprefix $(OBJ_DIR)/, $(ROCTRACER_NAMES)))
ROCPROFILER_OBJECTS := $(addsuffix .o, $(addprefix $(OBJ_DIR)/, $(ROCPROFILER_NAMES)))
all: rocprofiler_plugin_lib.so roctracer_plugin_lib.so


rocprofiler_plugin_lib.so: $(C_OBJECTS) $(AUX_OBJECTS) $(CPP_OBJECTS) $(ROCPROFILER_OBJECTS) $(ROCM_PATH)/lib/libhsa-runtime64.so 
	$(CXX)	$(LINKFLAGS) $^	-o $@
	
roctracer_plugin_lib.so: $(C_OBJECTS) $(AUX_OBJECTS) $(CPP_OBJECTS) $(ROCTRACER_OBJECTS) $(ROCM_PATH)/lib/libhsa-runtime64.so 
	$(CXX)	$(LINKFLAGS) $^	-o $@ 
		 
clean:
	$(RM) $(OBJ_DIR)/*.o rocprofiler_plugin_lib.so roctracer_plugin_lib.so
.PHONY : all clean

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c 
	$(CC) $(BASIC_FLAGS) $(CIMP) -c $^ -o $@

/obj/utils.o: src/utils.cpp
	$(CC) $(BASIC_FLAGS) -I ./inc -c $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(RTR_FLAGS) $(CIMP) -c $^ -o $@

$(OBJ_DIR)/%.o: $(AUX_DIR)/%.cpp
	$(CXX) $(AUX_FLAGS)	$(CIMP) -c $^ -o $@
	
$(OBJ_DIR)/%.o: $(ROCTRACER_FILES_DIR)/%.cpp
	$(CXX) $(RTR_FLAGS)	$(CIMP) -c $^ -o $@

$(OBJ_DIR)/%.o: $(ROCPROFILER_FILES_DIR)/%.cpp
	$(CXX) $(CXXFLAGS)	$(CIMP2) -c $^ -o $@
