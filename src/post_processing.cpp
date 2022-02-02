#include <stdio.h>
#include <time.h>
#include <sstream>
#include <fstream>
#include <limits.h>
#include <regex>
#include <filesystem>
#include <unistd.h>

#define ITERATIONS_NUMBER 50
#define BILLION 1E9

long compute_time(struct timespec timespec_struct){
	return BILLION*(long)timespec_struct.tv_sec + timespec_struct.tv_nsec;
}

//Function that calculates the offsets (seconds and nanoseconds) to the Epoch
void calculate_offsets(long* offset_s, long* offset_ns){
	long min_delta = LONG_MAX; 
	long current_delta, time_realtime, time_monotonic0, time_monotonic1, offset, time_monotonic_average;
	struct timespec time_monotonic0_ts, time_monotonic1_ts, time_realtime_ts;
	
	for(int step = 0; step < ITERATIONS_NUMBER; step++){

		clock_gettime( CLOCK_MONOTONIC, &time_monotonic0_ts);
		clock_gettime( CLOCK_REALTIME, &time_realtime_ts);
		clock_gettime( CLOCK_MONOTONIC, &time_monotonic1_ts);

		time_realtime = compute_time(time_realtime_ts);
		time_monotonic0 = compute_time(time_monotonic0_ts);
		time_monotonic1 = compute_time(time_monotonic1_ts);
		current_delta =  time_monotonic1 - time_monotonic0;
		if(current_delta < min_delta){
			min_delta = current_delta;
			time_monotonic_average = (time_monotonic1 + time_monotonic0)/2;
			offset = time_realtime - time_monotonic_average;
			*offset_s = offset/1000000000;
			*offset_ns = offset%1000000000;
		}
	}
}

//Function that reads the different files containing number of events, compute the total number of events, and remove the text files
void compute_and_remove_nb_events(long* total_nb_events, std::string trace_directory){
	long local_nb_events;
	std::regex nb_events_file_pattern("[0-9]+_[a-z]+_nb_events");

	for(auto const& entry : std::filesystem::directory_iterator(trace_directory)){
		if(std::regex_match( entry.path().stem().string(), nb_events_file_pattern)){
			std::ifstream nb_events_file(entry.path().string());
			if(nb_events_file.is_open()){
				nb_events_file >> local_nb_events;
				*total_nb_events += local_nb_events;
				nb_events_file.close();
				if( remove( entry.path().string().c_str() ) != 0 ){
					printf("Error deleting file : %s \n", entry.path().string().c_str());
				}
			}else{
				printf("Could not open file : %s", entry.path().string().c_str());
			}
		}
	}
}

//Function that copy the unupdated metada file, fill it and replace the unupdated metadata file with the updated one
void copy_and_fill_metadata(long total_nb_events, long offset_s, long offset_ns, std::string trace_directory){
	std::stringstream ss_metadata_file;
	std::stringstream ss_tmp_metadata_file;
	ss_metadata_file << trace_directory << "metadata";
	ss_tmp_metadata_file << trace_directory << "tmp_metadata";
	
	std::ifstream metadata_file(ss_metadata_file.str());
	std::ofstream tmp_metadata_file(ss_tmp_metadata_file.str());
	
	if(metadata_file.is_open() && tmp_metadata_file.is_open()){
		std::regex offset_s_pattern("(.*)(offset_s = 0;)(.*)");
		std::regex offset_ns_pattern("(.*)(offset = 0;)(.*)");
		std::regex nb_events_pattern("(.*)(nb_events = 0;)(.*)");

		std::string line;
		while (std::getline(metadata_file, line)){
			if (std::regex_match(line, offset_s_pattern)){
				tmp_metadata_file << "\toffset_s = " << offset_s << ";\n";
			}else if (std::regex_match ( line, offset_ns_pattern)){
				tmp_metadata_file << "\toffset = " << offset_ns << ";\n";
			}else if (std::regex_match ( line, nb_events_pattern)) {
				tmp_metadata_file << "\tnb_events = " << total_nb_events <<";\n";
			}else{
				tmp_metadata_file << line << "\n";
			}
		}

		metadata_file.close();
		tmp_metadata_file.close();
		if(remove(ss_metadata_file.str().c_str()) != 0 ){
			printf("Error when deleting unupdated metadata file \n");
		}
		if(rename(ss_tmp_metadata_file.str().c_str(), ss_metadata_file.str().c_str()) != 0){
			printf("Error when renaming updated metadata file \n");
		}
	}
}

int main(int argc, char *argv[]){

	printf("Computing the offset of the trace \n");
	long offset_s, offset_ns;
	calculate_offsets(&offset_s, &offset_ns);

	std::stringstream ss_trace_directory;
	std::stringstream ss_trace_directory_renamed;
	ss_trace_directory << argv[1] << "/rocprof_ctf_trace/";
	ss_trace_directory_renamed << argv[1] << "/rocprof_ctf_trace_" << getpid() << "_" << time(NULL)<< "/";

	printf("Computing the total number of events\n");
	long total_nb_events = 0;
	compute_and_remove_nb_events(&total_nb_events, ss_trace_directory.str());

	printf("Updating metadata file \n");
	copy_and_fill_metadata(total_nb_events, offset_s, offset_ns, ss_trace_directory.str());
	
	printf("Renaming CTF trace directory\n");
	std::filesystem::rename(ss_trace_directory.str().c_str(), ss_trace_directory_renamed.str().c_str());

	return 0;

}