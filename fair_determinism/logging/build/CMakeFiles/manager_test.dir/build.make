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
include CMakeFiles/manager_test.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/manager_test.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/manager_test.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/manager_test.dir/flags.make

CMakeFiles/manager_test.dir/tests/manager_test.cc.o: CMakeFiles/manager_test.dir/flags.make
CMakeFiles/manager_test.dir/tests/manager_test.cc.o: /Users/soimarcelo/projects/ccbench/fair_determinism/logging/tests/manager_test.cc
CMakeFiles/manager_test.dir/tests/manager_test.cc.o: CMakeFiles/manager_test.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/Users/soimarcelo/projects/ccbench/fair_determinism/logging/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/manager_test.dir/tests/manager_test.cc.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/manager_test.dir/tests/manager_test.cc.o -MF CMakeFiles/manager_test.dir/tests/manager_test.cc.o.d -o CMakeFiles/manager_test.dir/tests/manager_test.cc.o -c /Users/soimarcelo/projects/ccbench/fair_determinism/logging/tests/manager_test.cc

CMakeFiles/manager_test.dir/tests/manager_test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/manager_test.dir/tests/manager_test.cc.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/soimarcelo/projects/ccbench/fair_determinism/logging/tests/manager_test.cc > CMakeFiles/manager_test.dir/tests/manager_test.cc.i

CMakeFiles/manager_test.dir/tests/manager_test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/manager_test.dir/tests/manager_test.cc.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/soimarcelo/projects/ccbench/fair_determinism/logging/tests/manager_test.cc -o CMakeFiles/manager_test.dir/tests/manager_test.cc.s

# Object files for target manager_test
manager_test_OBJECTS = \
"CMakeFiles/manager_test.dir/tests/manager_test.cc.o"

# External object files for target manager_test
manager_test_EXTERNAL_OBJECTS =

manager_test: CMakeFiles/manager_test.dir/tests/manager_test.cc.o
manager_test: CMakeFiles/manager_test.dir/build.make
manager_test: CMakeFiles/manager_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/Users/soimarcelo/projects/ccbench/fair_determinism/logging/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable manager_test"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/manager_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/manager_test.dir/build: manager_test
.PHONY : CMakeFiles/manager_test.dir/build

CMakeFiles/manager_test.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/manager_test.dir/cmake_clean.cmake
.PHONY : CMakeFiles/manager_test.dir/clean

CMakeFiles/manager_test.dir/depend:
	cd /Users/soimarcelo/projects/ccbench/fair_determinism/logging/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/soimarcelo/projects/ccbench/fair_determinism/logging /Users/soimarcelo/projects/ccbench/fair_determinism/logging /Users/soimarcelo/projects/ccbench/fair_determinism/logging/build /Users/soimarcelo/projects/ccbench/fair_determinism/logging/build /Users/soimarcelo/projects/ccbench/fair_determinism/logging/build/CMakeFiles/manager_test.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/manager_test.dir/depend

