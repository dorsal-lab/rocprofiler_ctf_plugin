import sys
import glob
import os
import time
import re
from datetime import datetime

if(len(sys.argv) != 2):
    print("Error: exactly one argument is needed\n")

list_of_nb_events_files = glob.glob(sys.argv[1] + "/**/*nb_events.txt", recursive = True)
total_nb_events = 0;

if len(list_of_nb_events_files) == 0:
    exit()
    
for file in list_of_nb_events_files:
    f = open(file, "r")
    total_nb_events += int(f.read())
    os.remove(file) 

total_offset = 0
offset_seconds = 0
offset_ns = 0
min_delta = sys.maxsize
iterations_number = 50

for i in range(iterations_number):
    monotonic0 = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
    realtime = time.clock_gettime_ns(time.CLOCK_REALTIME)
    monotonic1 = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
    current_delta = monotonic1 - monotonic0
    if(current_delta < min_delta):
        min_delta = current_delta
        monotonic_average = (monotonic0 + monotonic1)//2
        offset = realtime - monotonic_average
        offset_seconds = offset//1000000000
        offset_ns = offset%1000000000

metadata_file = glob.glob(sys.argv[1] + "/**/metadata", recursive = True)[0]
metadata_f = open(metadata_file, "r")
list_of_lines = metadata_f.readlines()

nb_events_pattern = re.compile(r"\tnb_events = 0;\n")
offset_s_pattern = re.compile(r"\toffset_s = 0;\n")
offset_pattern = re.compile(r"\toffset = 0;\n")

for line_number in range(len(list_of_lines)):
    line = list_of_lines[line_number]
    if(nb_events_pattern.match(line)):
        list_of_lines[line_number] = "\tnb_events = " + str(total_nb_events) +";\n"
    if(offset_s_pattern.match(line)):
        list_of_lines[line_number] = "\toffset_s = " + str(offset_seconds) +";\n"
    if(offset_pattern.match(line)):
        list_of_lines[line_number] = "\toffset = " + str(offset_ns) +";\n"


metadata_f = open(metadata_file, "w")
metadata_f.writelines(list_of_lines)
metadata_f.close()

ctf_directory = glob.glob(sys.argv[1] + "/**/rocprof_ctf_trace", recursive = True)[0]
os.rename(ctf_directory, ctf_directory + "_" + str(os.getpid()) + "_" + datetime.now().strftime("%d%m%Y_%H%M%S"))