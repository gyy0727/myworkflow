# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /root/desktop/opensource/myworkflow

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /root/desktop/opensource/myworkflow/build

# Include any dependencies generated for this target.
include CMakeFiles/test_httpparser.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/test_httpparser.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/test_httpparser.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/test_httpparser.dir/flags.make

CMakeFiles/test_httpparser.dir/test/test_httpparser.cc.o: CMakeFiles/test_httpparser.dir/flags.make
CMakeFiles/test_httpparser.dir/test/test_httpparser.cc.o: ../test/test_httpparser.cc
CMakeFiles/test_httpparser.dir/test/test_httpparser.cc.o: CMakeFiles/test_httpparser.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/desktop/opensource/myworkflow/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/test_httpparser.dir/test/test_httpparser.cc.o"
	ccache /usr/bin/ccache  g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/test_httpparser.dir/test/test_httpparser.cc.o -MF CMakeFiles/test_httpparser.dir/test/test_httpparser.cc.o.d -o CMakeFiles/test_httpparser.dir/test/test_httpparser.cc.o -c /root/desktop/opensource/myworkflow/test/test_httpparser.cc

CMakeFiles/test_httpparser.dir/test/test_httpparser.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_httpparser.dir/test/test_httpparser.cc.i"
	/usr/bin/ccache  g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/desktop/opensource/myworkflow/test/test_httpparser.cc > CMakeFiles/test_httpparser.dir/test/test_httpparser.cc.i

CMakeFiles/test_httpparser.dir/test/test_httpparser.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_httpparser.dir/test/test_httpparser.cc.s"
	/usr/bin/ccache  g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/desktop/opensource/myworkflow/test/test_httpparser.cc -o CMakeFiles/test_httpparser.dir/test/test_httpparser.cc.s

# Object files for target test_httpparser
test_httpparser_OBJECTS = \
"CMakeFiles/test_httpparser.dir/test/test_httpparser.cc.o"

# External object files for target test_httpparser
test_httpparser_EXTERNAL_OBJECTS =

../bin/test_httpparser: CMakeFiles/test_httpparser.dir/test/test_httpparser.cc.o
../bin/test_httpparser: CMakeFiles/test_httpparser.dir/build.make
../bin/test_httpparser: /usr/lib/x86_64-linux-gnu/libspdlog.so.1.9.2
../bin/test_httpparser: /usr/lib/x86_64-linux-gnu/libssl.so
../bin/test_httpparser: /usr/lib/x86_64-linux-gnu/libcrypto.so
../bin/test_httpparser: ../lib/libworkflow.a
../bin/test_httpparser: /usr/lib/x86_64-linux-gnu/libfmt.so.8.1.1
../bin/test_httpparser: CMakeFiles/test_httpparser.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/root/desktop/opensource/myworkflow/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../bin/test_httpparser"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_httpparser.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/test_httpparser.dir/build: ../bin/test_httpparser
.PHONY : CMakeFiles/test_httpparser.dir/build

CMakeFiles/test_httpparser.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/test_httpparser.dir/cmake_clean.cmake
.PHONY : CMakeFiles/test_httpparser.dir/clean

CMakeFiles/test_httpparser.dir/depend:
	cd /root/desktop/opensource/myworkflow/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /root/desktop/opensource/myworkflow /root/desktop/opensource/myworkflow /root/desktop/opensource/myworkflow/build /root/desktop/opensource/myworkflow/build /root/desktop/opensource/myworkflow/build/CMakeFiles/test_httpparser.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/test_httpparser.dir/depend

