#include "utils.h"
#include <sys/stat.h>
#include <sstream>
#include <unistd.h>
#include <fstream>

void initialize_trace_directory(const char* output_prefix){
	struct stat buffer;
	std::stringstream trace_directory;
	trace_directory << output_prefix << "/rocprof_ctf_trace";
	if(stat(trace_directory.str().c_str(), &buffer) != 0){
		mkdir(trace_directory.str().c_str(), 0777);
		const char* ctf_plugin = getenv("PLUGIN_PATH");
		std::stringstream metadata_file;
		metadata_file << ctf_plugin << "/metadata";
		trace_directory << "/metadata";
		std::ifstream  src(metadata_file.str().c_str(), std::ios::binary);
		std::ofstream  dst(trace_directory.str().c_str(),   std::ios::binary);
		dst << src.rdbuf();
	}
}