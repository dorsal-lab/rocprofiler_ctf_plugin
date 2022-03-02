import os, re

PLUGIN_MAJOR = 1
PLUGIN_MINOR = 0

stream = os.popen('apt-cache show rocm-libs')
rocm_libs_info = stream.readlines()
version_pattern = re.compile(r"Version: ([0-9]\.[0-9]\.[0-9]).*")
version_pattern2 = re.compile(r".*Version.*")
for line in rocm_libs_info:
    if(version_pattern.search(line)):
        version_number = version_pattern.search(line).group(1)
        rocm_version_line = "\trocm_version = \"" + version_number + "\";\n"

metadata_f = open("metadata", "r")
list_of_lines = metadata_f.readlines()

tracer_name_pattern = re.compile((r".*tracer_name.*"))
env_begin_pattern = re.compile(re.escape(r"env {"))
rocm_version_line_number = 0
plugin_major_line_number = 0
plugin_major_line = "\tplugin_major = {};\n".format(PLUGIN_MAJOR)
plugin_minor_line_number = 0
plugin_minor_line = "\tplugin_minor = {};\n".format(PLUGIN_MINOR)
nb_events_line_number = 0
nb_events_line = "\tnb_events = 0;\n"

for line_number in range(len(list_of_lines)) :
    line = list_of_lines[line_number]
    if(env_begin_pattern.match(line)):
        rocm_version_line_number = line_number + 1
        plugin_major_line_number = line_number + 2
        plugin_minor_line_number = line_number + 3
        nb_events_line_number = line_number + 4
    if(tracer_name_pattern.match(line)):
        list_of_lines[line_number] = "\ttracer_name = \"rocprof\";\n"

list_of_lines.insert(rocm_version_line_number, rocm_version_line)
list_of_lines.insert(plugin_major_line_number, plugin_major_line)
list_of_lines.insert(plugin_minor_line_number, plugin_minor_line)
list_of_lines.insert(nb_events_line_number, nb_events_line)



metadata_f = open("metadata", "w")
metadata_f.writelines(list_of_lines)
metadata_f.close()
