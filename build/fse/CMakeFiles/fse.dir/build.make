# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
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
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/dc/image_dedup/iDedup

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/dc/image_dedup/iDedup/build

# Include any dependencies generated for this target.
include fse/CMakeFiles/fse.dir/depend.make

# Include the progress variables for this target.
include fse/CMakeFiles/fse.dir/progress.make

# Include the compile flags for this target's objects.
include fse/CMakeFiles/fse.dir/flags.make

fse/CMakeFiles/fse.dir/lib/entropy_common.c.o: fse/CMakeFiles/fse.dir/flags.make
fse/CMakeFiles/fse.dir/lib/entropy_common.c.o: ../fse/lib/entropy_common.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dc/image_dedup/iDedup/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object fse/CMakeFiles/fse.dir/lib/entropy_common.c.o"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fse.dir/lib/entropy_common.c.o   -c /home/dc/image_dedup/iDedup/fse/lib/entropy_common.c

fse/CMakeFiles/fse.dir/lib/entropy_common.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fse.dir/lib/entropy_common.c.i"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dc/image_dedup/iDedup/fse/lib/entropy_common.c > CMakeFiles/fse.dir/lib/entropy_common.c.i

fse/CMakeFiles/fse.dir/lib/entropy_common.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fse.dir/lib/entropy_common.c.s"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dc/image_dedup/iDedup/fse/lib/entropy_common.c -o CMakeFiles/fse.dir/lib/entropy_common.c.s

fse/CMakeFiles/fse.dir/lib/entropy_common.c.o.requires:

.PHONY : fse/CMakeFiles/fse.dir/lib/entropy_common.c.o.requires

fse/CMakeFiles/fse.dir/lib/entropy_common.c.o.provides: fse/CMakeFiles/fse.dir/lib/entropy_common.c.o.requires
	$(MAKE) -f fse/CMakeFiles/fse.dir/build.make fse/CMakeFiles/fse.dir/lib/entropy_common.c.o.provides.build
.PHONY : fse/CMakeFiles/fse.dir/lib/entropy_common.c.o.provides

fse/CMakeFiles/fse.dir/lib/entropy_common.c.o.provides.build: fse/CMakeFiles/fse.dir/lib/entropy_common.c.o


fse/CMakeFiles/fse.dir/lib/hist.c.o: fse/CMakeFiles/fse.dir/flags.make
fse/CMakeFiles/fse.dir/lib/hist.c.o: ../fse/lib/hist.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dc/image_dedup/iDedup/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object fse/CMakeFiles/fse.dir/lib/hist.c.o"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fse.dir/lib/hist.c.o   -c /home/dc/image_dedup/iDedup/fse/lib/hist.c

fse/CMakeFiles/fse.dir/lib/hist.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fse.dir/lib/hist.c.i"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dc/image_dedup/iDedup/fse/lib/hist.c > CMakeFiles/fse.dir/lib/hist.c.i

fse/CMakeFiles/fse.dir/lib/hist.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fse.dir/lib/hist.c.s"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dc/image_dedup/iDedup/fse/lib/hist.c -o CMakeFiles/fse.dir/lib/hist.c.s

fse/CMakeFiles/fse.dir/lib/hist.c.o.requires:

.PHONY : fse/CMakeFiles/fse.dir/lib/hist.c.o.requires

fse/CMakeFiles/fse.dir/lib/hist.c.o.provides: fse/CMakeFiles/fse.dir/lib/hist.c.o.requires
	$(MAKE) -f fse/CMakeFiles/fse.dir/build.make fse/CMakeFiles/fse.dir/lib/hist.c.o.provides.build
.PHONY : fse/CMakeFiles/fse.dir/lib/hist.c.o.provides

fse/CMakeFiles/fse.dir/lib/hist.c.o.provides.build: fse/CMakeFiles/fse.dir/lib/hist.c.o


fse/CMakeFiles/fse.dir/lib/fse_decompress.c.o: fse/CMakeFiles/fse.dir/flags.make
fse/CMakeFiles/fse.dir/lib/fse_decompress.c.o: ../fse/lib/fse_decompress.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dc/image_dedup/iDedup/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object fse/CMakeFiles/fse.dir/lib/fse_decompress.c.o"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fse.dir/lib/fse_decompress.c.o   -c /home/dc/image_dedup/iDedup/fse/lib/fse_decompress.c

fse/CMakeFiles/fse.dir/lib/fse_decompress.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fse.dir/lib/fse_decompress.c.i"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dc/image_dedup/iDedup/fse/lib/fse_decompress.c > CMakeFiles/fse.dir/lib/fse_decompress.c.i

fse/CMakeFiles/fse.dir/lib/fse_decompress.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fse.dir/lib/fse_decompress.c.s"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dc/image_dedup/iDedup/fse/lib/fse_decompress.c -o CMakeFiles/fse.dir/lib/fse_decompress.c.s

fse/CMakeFiles/fse.dir/lib/fse_decompress.c.o.requires:

.PHONY : fse/CMakeFiles/fse.dir/lib/fse_decompress.c.o.requires

fse/CMakeFiles/fse.dir/lib/fse_decompress.c.o.provides: fse/CMakeFiles/fse.dir/lib/fse_decompress.c.o.requires
	$(MAKE) -f fse/CMakeFiles/fse.dir/build.make fse/CMakeFiles/fse.dir/lib/fse_decompress.c.o.provides.build
.PHONY : fse/CMakeFiles/fse.dir/lib/fse_decompress.c.o.provides

fse/CMakeFiles/fse.dir/lib/fse_decompress.c.o.provides.build: fse/CMakeFiles/fse.dir/lib/fse_decompress.c.o


fse/CMakeFiles/fse.dir/lib/fse_compress.c.o: fse/CMakeFiles/fse.dir/flags.make
fse/CMakeFiles/fse.dir/lib/fse_compress.c.o: ../fse/lib/fse_compress.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dc/image_dedup/iDedup/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object fse/CMakeFiles/fse.dir/lib/fse_compress.c.o"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fse.dir/lib/fse_compress.c.o   -c /home/dc/image_dedup/iDedup/fse/lib/fse_compress.c

fse/CMakeFiles/fse.dir/lib/fse_compress.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fse.dir/lib/fse_compress.c.i"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dc/image_dedup/iDedup/fse/lib/fse_compress.c > CMakeFiles/fse.dir/lib/fse_compress.c.i

fse/CMakeFiles/fse.dir/lib/fse_compress.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fse.dir/lib/fse_compress.c.s"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dc/image_dedup/iDedup/fse/lib/fse_compress.c -o CMakeFiles/fse.dir/lib/fse_compress.c.s

fse/CMakeFiles/fse.dir/lib/fse_compress.c.o.requires:

.PHONY : fse/CMakeFiles/fse.dir/lib/fse_compress.c.o.requires

fse/CMakeFiles/fse.dir/lib/fse_compress.c.o.provides: fse/CMakeFiles/fse.dir/lib/fse_compress.c.o.requires
	$(MAKE) -f fse/CMakeFiles/fse.dir/build.make fse/CMakeFiles/fse.dir/lib/fse_compress.c.o.provides.build
.PHONY : fse/CMakeFiles/fse.dir/lib/fse_compress.c.o.provides

fse/CMakeFiles/fse.dir/lib/fse_compress.c.o.provides.build: fse/CMakeFiles/fse.dir/lib/fse_compress.c.o


fse/CMakeFiles/fse.dir/lib/fseU16.c.o: fse/CMakeFiles/fse.dir/flags.make
fse/CMakeFiles/fse.dir/lib/fseU16.c.o: ../fse/lib/fseU16.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dc/image_dedup/iDedup/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object fse/CMakeFiles/fse.dir/lib/fseU16.c.o"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fse.dir/lib/fseU16.c.o   -c /home/dc/image_dedup/iDedup/fse/lib/fseU16.c

fse/CMakeFiles/fse.dir/lib/fseU16.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fse.dir/lib/fseU16.c.i"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dc/image_dedup/iDedup/fse/lib/fseU16.c > CMakeFiles/fse.dir/lib/fseU16.c.i

fse/CMakeFiles/fse.dir/lib/fseU16.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fse.dir/lib/fseU16.c.s"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dc/image_dedup/iDedup/fse/lib/fseU16.c -o CMakeFiles/fse.dir/lib/fseU16.c.s

fse/CMakeFiles/fse.dir/lib/fseU16.c.o.requires:

.PHONY : fse/CMakeFiles/fse.dir/lib/fseU16.c.o.requires

fse/CMakeFiles/fse.dir/lib/fseU16.c.o.provides: fse/CMakeFiles/fse.dir/lib/fseU16.c.o.requires
	$(MAKE) -f fse/CMakeFiles/fse.dir/build.make fse/CMakeFiles/fse.dir/lib/fseU16.c.o.provides.build
.PHONY : fse/CMakeFiles/fse.dir/lib/fseU16.c.o.provides

fse/CMakeFiles/fse.dir/lib/fseU16.c.o.provides.build: fse/CMakeFiles/fse.dir/lib/fseU16.c.o


fse/CMakeFiles/fse.dir/lib/huf_compress.c.o: fse/CMakeFiles/fse.dir/flags.make
fse/CMakeFiles/fse.dir/lib/huf_compress.c.o: ../fse/lib/huf_compress.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dc/image_dedup/iDedup/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object fse/CMakeFiles/fse.dir/lib/huf_compress.c.o"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fse.dir/lib/huf_compress.c.o   -c /home/dc/image_dedup/iDedup/fse/lib/huf_compress.c

fse/CMakeFiles/fse.dir/lib/huf_compress.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fse.dir/lib/huf_compress.c.i"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dc/image_dedup/iDedup/fse/lib/huf_compress.c > CMakeFiles/fse.dir/lib/huf_compress.c.i

fse/CMakeFiles/fse.dir/lib/huf_compress.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fse.dir/lib/huf_compress.c.s"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dc/image_dedup/iDedup/fse/lib/huf_compress.c -o CMakeFiles/fse.dir/lib/huf_compress.c.s

fse/CMakeFiles/fse.dir/lib/huf_compress.c.o.requires:

.PHONY : fse/CMakeFiles/fse.dir/lib/huf_compress.c.o.requires

fse/CMakeFiles/fse.dir/lib/huf_compress.c.o.provides: fse/CMakeFiles/fse.dir/lib/huf_compress.c.o.requires
	$(MAKE) -f fse/CMakeFiles/fse.dir/build.make fse/CMakeFiles/fse.dir/lib/huf_compress.c.o.provides.build
.PHONY : fse/CMakeFiles/fse.dir/lib/huf_compress.c.o.provides

fse/CMakeFiles/fse.dir/lib/huf_compress.c.o.provides.build: fse/CMakeFiles/fse.dir/lib/huf_compress.c.o


fse/CMakeFiles/fse.dir/lib/huf_decompress.c.o: fse/CMakeFiles/fse.dir/flags.make
fse/CMakeFiles/fse.dir/lib/huf_decompress.c.o: ../fse/lib/huf_decompress.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dc/image_dedup/iDedup/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object fse/CMakeFiles/fse.dir/lib/huf_decompress.c.o"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fse.dir/lib/huf_decompress.c.o   -c /home/dc/image_dedup/iDedup/fse/lib/huf_decompress.c

fse/CMakeFiles/fse.dir/lib/huf_decompress.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fse.dir/lib/huf_decompress.c.i"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/dc/image_dedup/iDedup/fse/lib/huf_decompress.c > CMakeFiles/fse.dir/lib/huf_decompress.c.i

fse/CMakeFiles/fse.dir/lib/huf_decompress.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fse.dir/lib/huf_decompress.c.s"
	cd /home/dc/image_dedup/iDedup/build/fse && /usr/bin/gcc-7 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/dc/image_dedup/iDedup/fse/lib/huf_decompress.c -o CMakeFiles/fse.dir/lib/huf_decompress.c.s

fse/CMakeFiles/fse.dir/lib/huf_decompress.c.o.requires:

.PHONY : fse/CMakeFiles/fse.dir/lib/huf_decompress.c.o.requires

fse/CMakeFiles/fse.dir/lib/huf_decompress.c.o.provides: fse/CMakeFiles/fse.dir/lib/huf_decompress.c.o.requires
	$(MAKE) -f fse/CMakeFiles/fse.dir/build.make fse/CMakeFiles/fse.dir/lib/huf_decompress.c.o.provides.build
.PHONY : fse/CMakeFiles/fse.dir/lib/huf_decompress.c.o.provides

fse/CMakeFiles/fse.dir/lib/huf_decompress.c.o.provides.build: fse/CMakeFiles/fse.dir/lib/huf_decompress.c.o


# Object files for target fse
fse_OBJECTS = \
"CMakeFiles/fse.dir/lib/entropy_common.c.o" \
"CMakeFiles/fse.dir/lib/hist.c.o" \
"CMakeFiles/fse.dir/lib/fse_decompress.c.o" \
"CMakeFiles/fse.dir/lib/fse_compress.c.o" \
"CMakeFiles/fse.dir/lib/fseU16.c.o" \
"CMakeFiles/fse.dir/lib/huf_compress.c.o" \
"CMakeFiles/fse.dir/lib/huf_decompress.c.o"

# External object files for target fse
fse_EXTERNAL_OBJECTS =

fse/libfse.a: fse/CMakeFiles/fse.dir/lib/entropy_common.c.o
fse/libfse.a: fse/CMakeFiles/fse.dir/lib/hist.c.o
fse/libfse.a: fse/CMakeFiles/fse.dir/lib/fse_decompress.c.o
fse/libfse.a: fse/CMakeFiles/fse.dir/lib/fse_compress.c.o
fse/libfse.a: fse/CMakeFiles/fse.dir/lib/fseU16.c.o
fse/libfse.a: fse/CMakeFiles/fse.dir/lib/huf_compress.c.o
fse/libfse.a: fse/CMakeFiles/fse.dir/lib/huf_decompress.c.o
fse/libfse.a: fse/CMakeFiles/fse.dir/build.make
fse/libfse.a: fse/CMakeFiles/fse.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dc/image_dedup/iDedup/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Linking C static library libfse.a"
	cd /home/dc/image_dedup/iDedup/build/fse && $(CMAKE_COMMAND) -P CMakeFiles/fse.dir/cmake_clean_target.cmake
	cd /home/dc/image_dedup/iDedup/build/fse && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/fse.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
fse/CMakeFiles/fse.dir/build: fse/libfse.a

.PHONY : fse/CMakeFiles/fse.dir/build

fse/CMakeFiles/fse.dir/requires: fse/CMakeFiles/fse.dir/lib/entropy_common.c.o.requires
fse/CMakeFiles/fse.dir/requires: fse/CMakeFiles/fse.dir/lib/hist.c.o.requires
fse/CMakeFiles/fse.dir/requires: fse/CMakeFiles/fse.dir/lib/fse_decompress.c.o.requires
fse/CMakeFiles/fse.dir/requires: fse/CMakeFiles/fse.dir/lib/fse_compress.c.o.requires
fse/CMakeFiles/fse.dir/requires: fse/CMakeFiles/fse.dir/lib/fseU16.c.o.requires
fse/CMakeFiles/fse.dir/requires: fse/CMakeFiles/fse.dir/lib/huf_compress.c.o.requires
fse/CMakeFiles/fse.dir/requires: fse/CMakeFiles/fse.dir/lib/huf_decompress.c.o.requires

.PHONY : fse/CMakeFiles/fse.dir/requires

fse/CMakeFiles/fse.dir/clean:
	cd /home/dc/image_dedup/iDedup/build/fse && $(CMAKE_COMMAND) -P CMakeFiles/fse.dir/cmake_clean.cmake
.PHONY : fse/CMakeFiles/fse.dir/clean

fse/CMakeFiles/fse.dir/depend:
	cd /home/dc/image_dedup/iDedup/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dc/image_dedup/iDedup /home/dc/image_dedup/iDedup/fse /home/dc/image_dedup/iDedup/build /home/dc/image_dedup/iDedup/build/fse /home/dc/image_dedup/iDedup/build/fse/CMakeFiles/fse.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : fse/CMakeFiles/fse.dir/depend

