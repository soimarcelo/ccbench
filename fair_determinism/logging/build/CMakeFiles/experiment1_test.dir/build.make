# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.30

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
CMAKE_COMMAND = /opt/homebrew/Cellar/cmake/3.30.0/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/Cellar/cmake/3.30.0/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/soimarcelo/projects/ccbench/fair_determinism/logging

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/soimarcelo/projects/ccbench/fair_determinism/logging/build

# Include any dependencies generated for this target.
include CMakeFiles/experiment1_test.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/experiment1_test.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/experiment1_test.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/experiment1_test.dir/flags.make

CMakeFiles/experiment1_test.dir/tests/experiment1_test.cc.o: CMakeFiles/experiment1_test.dir/flags.make
CMakeFiles/experiment1_test.dir/tests/experiment1_test.cc.o: /Users/soimarcelo/projects/ccbench/fair_determinism/logging/tests/experiment1_test.cc
CMakeFiles/experiment1_test.dir/tests/experiment1_test.cc.o: CMakeFiles/experiment1_test.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/Users/soimarcelo/projects/ccbench/fair_determinism/logging/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/experiment1_test.dir/tests/experiment1_test.cc.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/experiment1_test.dir/tests/experiment1_test.cc.o -MF CMakeFiles/experiment1_test.dir/tests/experiment1_test.cc.o.d -o CMakeFiles/experiment1_test.dir/tests/experiment1_test.cc.o -c /Users/soimarcelo/projects/ccbench/fair_determinism/logging/tests/experiment1_test.cc

CMakeFiles/experiment1_test.dir/tests/experiment1_test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/experiment1_test.dir/tests/experiment1_test.cc.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/soimarcelo/projects/ccbench/fair_determinism/logging/tests/experiment1_test.cc > CMakeFiles/experiment1_test.dir/tests/experiment1_test.cc.i

CMakeFiles/experiment1_test.dir/tests/experiment1_test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/experiment1_test.dir/tests/experiment1_test.cc.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/soimarcelo/projects/ccbench/fair_determinism/logging/tests/experiment1_test.cc -o CMakeFiles/experiment1_test.dir/tests/experiment1_test.cc.s

# Object files for target experiment1_test
experiment1_test_OBJECTS = \
"CMakeFiles/experiment1_test.dir/tests/experiment1_test.cc.o"

# External object files for target experiment1_test
experiment1_test_EXTERNAL_OBJECTS =

experiment1_test: CMakeFiles/experiment1_test.dir/tests/experiment1_test.cc.o
experiment1_test: CMakeFiles/experiment1_test.dir/build.make
experiment1_test: CMakeFiles/experiment1_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/Users/soimarcelo/projects/ccbench/fair_determinism/logging/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable experiment1_test"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/experiment1_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/experiment1_test.dir/build: experiment1_test
.PHONY : CMakeFiles/experiment1_test.dir/build

CMakeFiles/experiment1_test.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/experiment1_test.dir/cmake_clean.cmake
.PHONY : CMakeFiles/experiment1_test.dir/clean

CMakeFiles/experiment1_test.dir/depend:
	cd /Users/soimarcelo/projects/ccbench/fair_determinism/logging/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/soimarcelo/projects/ccbench/fair_determinism/logging /Users/soimarcelo/projects/ccbench/fair_determinism/logging /Users/soimarcelo/projects/ccbench/fair_determinism/logging/build /Users/soimarcelo/projects/ccbench/fair_determinism/logging/build /Users/soimarcelo/projects/ccbench/fair_determinism/logging/build/CMakeFiles/experiment1_test.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/experiment1_test.dir/depend

