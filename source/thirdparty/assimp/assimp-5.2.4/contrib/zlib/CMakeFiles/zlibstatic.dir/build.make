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
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.22.3/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.22.3/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4

# Include any dependencies generated for this target.
include contrib/zlib/CMakeFiles/zlibstatic.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.make

# Include the progress variables for this target.
include contrib/zlib/CMakeFiles/zlibstatic.dir/progress.make

# Include the compile flags for this target's objects.
include contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make

contrib/zlib/CMakeFiles/zlibstatic.dir/adler32.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make
contrib/zlib/CMakeFiles/zlibstatic.dir/adler32.c.o: contrib/zlib/adler32.c
contrib/zlib/CMakeFiles/zlibstatic.dir/adler32.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object contrib/zlib/CMakeFiles/zlibstatic.dir/adler32.c.o"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT contrib/zlib/CMakeFiles/zlibstatic.dir/adler32.c.o -MF CMakeFiles/zlibstatic.dir/adler32.c.o.d -o CMakeFiles/zlibstatic.dir/adler32.c.o -c /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/adler32.c

contrib/zlib/CMakeFiles/zlibstatic.dir/adler32.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zlibstatic.dir/adler32.c.i"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/adler32.c > CMakeFiles/zlibstatic.dir/adler32.c.i

contrib/zlib/CMakeFiles/zlibstatic.dir/adler32.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zlibstatic.dir/adler32.c.s"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/adler32.c -o CMakeFiles/zlibstatic.dir/adler32.c.s

contrib/zlib/CMakeFiles/zlibstatic.dir/compress.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make
contrib/zlib/CMakeFiles/zlibstatic.dir/compress.c.o: contrib/zlib/compress.c
contrib/zlib/CMakeFiles/zlibstatic.dir/compress.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object contrib/zlib/CMakeFiles/zlibstatic.dir/compress.c.o"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT contrib/zlib/CMakeFiles/zlibstatic.dir/compress.c.o -MF CMakeFiles/zlibstatic.dir/compress.c.o.d -o CMakeFiles/zlibstatic.dir/compress.c.o -c /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/compress.c

contrib/zlib/CMakeFiles/zlibstatic.dir/compress.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zlibstatic.dir/compress.c.i"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/compress.c > CMakeFiles/zlibstatic.dir/compress.c.i

contrib/zlib/CMakeFiles/zlibstatic.dir/compress.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zlibstatic.dir/compress.c.s"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/compress.c -o CMakeFiles/zlibstatic.dir/compress.c.s

contrib/zlib/CMakeFiles/zlibstatic.dir/crc32.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make
contrib/zlib/CMakeFiles/zlibstatic.dir/crc32.c.o: contrib/zlib/crc32.c
contrib/zlib/CMakeFiles/zlibstatic.dir/crc32.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object contrib/zlib/CMakeFiles/zlibstatic.dir/crc32.c.o"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT contrib/zlib/CMakeFiles/zlibstatic.dir/crc32.c.o -MF CMakeFiles/zlibstatic.dir/crc32.c.o.d -o CMakeFiles/zlibstatic.dir/crc32.c.o -c /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/crc32.c

contrib/zlib/CMakeFiles/zlibstatic.dir/crc32.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zlibstatic.dir/crc32.c.i"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/crc32.c > CMakeFiles/zlibstatic.dir/crc32.c.i

contrib/zlib/CMakeFiles/zlibstatic.dir/crc32.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zlibstatic.dir/crc32.c.s"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/crc32.c -o CMakeFiles/zlibstatic.dir/crc32.c.s

contrib/zlib/CMakeFiles/zlibstatic.dir/deflate.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make
contrib/zlib/CMakeFiles/zlibstatic.dir/deflate.c.o: contrib/zlib/deflate.c
contrib/zlib/CMakeFiles/zlibstatic.dir/deflate.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object contrib/zlib/CMakeFiles/zlibstatic.dir/deflate.c.o"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT contrib/zlib/CMakeFiles/zlibstatic.dir/deflate.c.o -MF CMakeFiles/zlibstatic.dir/deflate.c.o.d -o CMakeFiles/zlibstatic.dir/deflate.c.o -c /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/deflate.c

contrib/zlib/CMakeFiles/zlibstatic.dir/deflate.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zlibstatic.dir/deflate.c.i"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/deflate.c > CMakeFiles/zlibstatic.dir/deflate.c.i

contrib/zlib/CMakeFiles/zlibstatic.dir/deflate.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zlibstatic.dir/deflate.c.s"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/deflate.c -o CMakeFiles/zlibstatic.dir/deflate.c.s

contrib/zlib/CMakeFiles/zlibstatic.dir/gzclose.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make
contrib/zlib/CMakeFiles/zlibstatic.dir/gzclose.c.o: contrib/zlib/gzclose.c
contrib/zlib/CMakeFiles/zlibstatic.dir/gzclose.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object contrib/zlib/CMakeFiles/zlibstatic.dir/gzclose.c.o"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT contrib/zlib/CMakeFiles/zlibstatic.dir/gzclose.c.o -MF CMakeFiles/zlibstatic.dir/gzclose.c.o.d -o CMakeFiles/zlibstatic.dir/gzclose.c.o -c /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/gzclose.c

contrib/zlib/CMakeFiles/zlibstatic.dir/gzclose.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zlibstatic.dir/gzclose.c.i"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/gzclose.c > CMakeFiles/zlibstatic.dir/gzclose.c.i

contrib/zlib/CMakeFiles/zlibstatic.dir/gzclose.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zlibstatic.dir/gzclose.c.s"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/gzclose.c -o CMakeFiles/zlibstatic.dir/gzclose.c.s

contrib/zlib/CMakeFiles/zlibstatic.dir/gzlib.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make
contrib/zlib/CMakeFiles/zlibstatic.dir/gzlib.c.o: contrib/zlib/gzlib.c
contrib/zlib/CMakeFiles/zlibstatic.dir/gzlib.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object contrib/zlib/CMakeFiles/zlibstatic.dir/gzlib.c.o"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT contrib/zlib/CMakeFiles/zlibstatic.dir/gzlib.c.o -MF CMakeFiles/zlibstatic.dir/gzlib.c.o.d -o CMakeFiles/zlibstatic.dir/gzlib.c.o -c /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/gzlib.c

contrib/zlib/CMakeFiles/zlibstatic.dir/gzlib.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zlibstatic.dir/gzlib.c.i"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/gzlib.c > CMakeFiles/zlibstatic.dir/gzlib.c.i

contrib/zlib/CMakeFiles/zlibstatic.dir/gzlib.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zlibstatic.dir/gzlib.c.s"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/gzlib.c -o CMakeFiles/zlibstatic.dir/gzlib.c.s

contrib/zlib/CMakeFiles/zlibstatic.dir/gzread.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make
contrib/zlib/CMakeFiles/zlibstatic.dir/gzread.c.o: contrib/zlib/gzread.c
contrib/zlib/CMakeFiles/zlibstatic.dir/gzread.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object contrib/zlib/CMakeFiles/zlibstatic.dir/gzread.c.o"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT contrib/zlib/CMakeFiles/zlibstatic.dir/gzread.c.o -MF CMakeFiles/zlibstatic.dir/gzread.c.o.d -o CMakeFiles/zlibstatic.dir/gzread.c.o -c /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/gzread.c

contrib/zlib/CMakeFiles/zlibstatic.dir/gzread.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zlibstatic.dir/gzread.c.i"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/gzread.c > CMakeFiles/zlibstatic.dir/gzread.c.i

contrib/zlib/CMakeFiles/zlibstatic.dir/gzread.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zlibstatic.dir/gzread.c.s"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/gzread.c -o CMakeFiles/zlibstatic.dir/gzread.c.s

contrib/zlib/CMakeFiles/zlibstatic.dir/gzwrite.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make
contrib/zlib/CMakeFiles/zlibstatic.dir/gzwrite.c.o: contrib/zlib/gzwrite.c
contrib/zlib/CMakeFiles/zlibstatic.dir/gzwrite.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building C object contrib/zlib/CMakeFiles/zlibstatic.dir/gzwrite.c.o"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT contrib/zlib/CMakeFiles/zlibstatic.dir/gzwrite.c.o -MF CMakeFiles/zlibstatic.dir/gzwrite.c.o.d -o CMakeFiles/zlibstatic.dir/gzwrite.c.o -c /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/gzwrite.c

contrib/zlib/CMakeFiles/zlibstatic.dir/gzwrite.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zlibstatic.dir/gzwrite.c.i"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/gzwrite.c > CMakeFiles/zlibstatic.dir/gzwrite.c.i

contrib/zlib/CMakeFiles/zlibstatic.dir/gzwrite.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zlibstatic.dir/gzwrite.c.s"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/gzwrite.c -o CMakeFiles/zlibstatic.dir/gzwrite.c.s

contrib/zlib/CMakeFiles/zlibstatic.dir/inflate.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make
contrib/zlib/CMakeFiles/zlibstatic.dir/inflate.c.o: contrib/zlib/inflate.c
contrib/zlib/CMakeFiles/zlibstatic.dir/inflate.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building C object contrib/zlib/CMakeFiles/zlibstatic.dir/inflate.c.o"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT contrib/zlib/CMakeFiles/zlibstatic.dir/inflate.c.o -MF CMakeFiles/zlibstatic.dir/inflate.c.o.d -o CMakeFiles/zlibstatic.dir/inflate.c.o -c /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/inflate.c

contrib/zlib/CMakeFiles/zlibstatic.dir/inflate.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zlibstatic.dir/inflate.c.i"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/inflate.c > CMakeFiles/zlibstatic.dir/inflate.c.i

contrib/zlib/CMakeFiles/zlibstatic.dir/inflate.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zlibstatic.dir/inflate.c.s"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/inflate.c -o CMakeFiles/zlibstatic.dir/inflate.c.s

contrib/zlib/CMakeFiles/zlibstatic.dir/infback.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make
contrib/zlib/CMakeFiles/zlibstatic.dir/infback.c.o: contrib/zlib/infback.c
contrib/zlib/CMakeFiles/zlibstatic.dir/infback.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building C object contrib/zlib/CMakeFiles/zlibstatic.dir/infback.c.o"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT contrib/zlib/CMakeFiles/zlibstatic.dir/infback.c.o -MF CMakeFiles/zlibstatic.dir/infback.c.o.d -o CMakeFiles/zlibstatic.dir/infback.c.o -c /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/infback.c

contrib/zlib/CMakeFiles/zlibstatic.dir/infback.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zlibstatic.dir/infback.c.i"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/infback.c > CMakeFiles/zlibstatic.dir/infback.c.i

contrib/zlib/CMakeFiles/zlibstatic.dir/infback.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zlibstatic.dir/infback.c.s"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/infback.c -o CMakeFiles/zlibstatic.dir/infback.c.s

contrib/zlib/CMakeFiles/zlibstatic.dir/inftrees.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make
contrib/zlib/CMakeFiles/zlibstatic.dir/inftrees.c.o: contrib/zlib/inftrees.c
contrib/zlib/CMakeFiles/zlibstatic.dir/inftrees.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building C object contrib/zlib/CMakeFiles/zlibstatic.dir/inftrees.c.o"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT contrib/zlib/CMakeFiles/zlibstatic.dir/inftrees.c.o -MF CMakeFiles/zlibstatic.dir/inftrees.c.o.d -o CMakeFiles/zlibstatic.dir/inftrees.c.o -c /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/inftrees.c

contrib/zlib/CMakeFiles/zlibstatic.dir/inftrees.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zlibstatic.dir/inftrees.c.i"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/inftrees.c > CMakeFiles/zlibstatic.dir/inftrees.c.i

contrib/zlib/CMakeFiles/zlibstatic.dir/inftrees.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zlibstatic.dir/inftrees.c.s"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/inftrees.c -o CMakeFiles/zlibstatic.dir/inftrees.c.s

contrib/zlib/CMakeFiles/zlibstatic.dir/inffast.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make
contrib/zlib/CMakeFiles/zlibstatic.dir/inffast.c.o: contrib/zlib/inffast.c
contrib/zlib/CMakeFiles/zlibstatic.dir/inffast.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Building C object contrib/zlib/CMakeFiles/zlibstatic.dir/inffast.c.o"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT contrib/zlib/CMakeFiles/zlibstatic.dir/inffast.c.o -MF CMakeFiles/zlibstatic.dir/inffast.c.o.d -o CMakeFiles/zlibstatic.dir/inffast.c.o -c /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/inffast.c

contrib/zlib/CMakeFiles/zlibstatic.dir/inffast.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zlibstatic.dir/inffast.c.i"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/inffast.c > CMakeFiles/zlibstatic.dir/inffast.c.i

contrib/zlib/CMakeFiles/zlibstatic.dir/inffast.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zlibstatic.dir/inffast.c.s"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/inffast.c -o CMakeFiles/zlibstatic.dir/inffast.c.s

contrib/zlib/CMakeFiles/zlibstatic.dir/trees.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make
contrib/zlib/CMakeFiles/zlibstatic.dir/trees.c.o: contrib/zlib/trees.c
contrib/zlib/CMakeFiles/zlibstatic.dir/trees.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_13) "Building C object contrib/zlib/CMakeFiles/zlibstatic.dir/trees.c.o"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT contrib/zlib/CMakeFiles/zlibstatic.dir/trees.c.o -MF CMakeFiles/zlibstatic.dir/trees.c.o.d -o CMakeFiles/zlibstatic.dir/trees.c.o -c /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/trees.c

contrib/zlib/CMakeFiles/zlibstatic.dir/trees.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zlibstatic.dir/trees.c.i"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/trees.c > CMakeFiles/zlibstatic.dir/trees.c.i

contrib/zlib/CMakeFiles/zlibstatic.dir/trees.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zlibstatic.dir/trees.c.s"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/trees.c -o CMakeFiles/zlibstatic.dir/trees.c.s

contrib/zlib/CMakeFiles/zlibstatic.dir/uncompr.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make
contrib/zlib/CMakeFiles/zlibstatic.dir/uncompr.c.o: contrib/zlib/uncompr.c
contrib/zlib/CMakeFiles/zlibstatic.dir/uncompr.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_14) "Building C object contrib/zlib/CMakeFiles/zlibstatic.dir/uncompr.c.o"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT contrib/zlib/CMakeFiles/zlibstatic.dir/uncompr.c.o -MF CMakeFiles/zlibstatic.dir/uncompr.c.o.d -o CMakeFiles/zlibstatic.dir/uncompr.c.o -c /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/uncompr.c

contrib/zlib/CMakeFiles/zlibstatic.dir/uncompr.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zlibstatic.dir/uncompr.c.i"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/uncompr.c > CMakeFiles/zlibstatic.dir/uncompr.c.i

contrib/zlib/CMakeFiles/zlibstatic.dir/uncompr.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zlibstatic.dir/uncompr.c.s"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/uncompr.c -o CMakeFiles/zlibstatic.dir/uncompr.c.s

contrib/zlib/CMakeFiles/zlibstatic.dir/zutil.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/flags.make
contrib/zlib/CMakeFiles/zlibstatic.dir/zutil.c.o: contrib/zlib/zutil.c
contrib/zlib/CMakeFiles/zlibstatic.dir/zutil.c.o: contrib/zlib/CMakeFiles/zlibstatic.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_15) "Building C object contrib/zlib/CMakeFiles/zlibstatic.dir/zutil.c.o"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT contrib/zlib/CMakeFiles/zlibstatic.dir/zutil.c.o -MF CMakeFiles/zlibstatic.dir/zutil.c.o.d -o CMakeFiles/zlibstatic.dir/zutil.c.o -c /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/zutil.c

contrib/zlib/CMakeFiles/zlibstatic.dir/zutil.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/zlibstatic.dir/zutil.c.i"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/zutil.c > CMakeFiles/zlibstatic.dir/zutil.c.i

contrib/zlib/CMakeFiles/zlibstatic.dir/zutil.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/zlibstatic.dir/zutil.c.s"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/zutil.c -o CMakeFiles/zlibstatic.dir/zutil.c.s

# Object files for target zlibstatic
zlibstatic_OBJECTS = \
"CMakeFiles/zlibstatic.dir/adler32.c.o" \
"CMakeFiles/zlibstatic.dir/compress.c.o" \
"CMakeFiles/zlibstatic.dir/crc32.c.o" \
"CMakeFiles/zlibstatic.dir/deflate.c.o" \
"CMakeFiles/zlibstatic.dir/gzclose.c.o" \
"CMakeFiles/zlibstatic.dir/gzlib.c.o" \
"CMakeFiles/zlibstatic.dir/gzread.c.o" \
"CMakeFiles/zlibstatic.dir/gzwrite.c.o" \
"CMakeFiles/zlibstatic.dir/inflate.c.o" \
"CMakeFiles/zlibstatic.dir/infback.c.o" \
"CMakeFiles/zlibstatic.dir/inftrees.c.o" \
"CMakeFiles/zlibstatic.dir/inffast.c.o" \
"CMakeFiles/zlibstatic.dir/trees.c.o" \
"CMakeFiles/zlibstatic.dir/uncompr.c.o" \
"CMakeFiles/zlibstatic.dir/zutil.c.o"

# External object files for target zlibstatic
zlibstatic_EXTERNAL_OBJECTS =

contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/adler32.c.o
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/compress.c.o
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/crc32.c.o
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/deflate.c.o
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/gzclose.c.o
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/gzlib.c.o
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/gzread.c.o
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/gzwrite.c.o
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/inflate.c.o
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/infback.c.o
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/inftrees.c.o
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/inffast.c.o
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/trees.c.o
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/uncompr.c.o
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/zutil.c.o
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/build.make
contrib/zlib/libzlibstatic.a: contrib/zlib/CMakeFiles/zlibstatic.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/CMakeFiles --progress-num=$(CMAKE_PROGRESS_16) "Linking C static library libzlibstatic.a"
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && $(CMAKE_COMMAND) -P CMakeFiles/zlibstatic.dir/cmake_clean_target.cmake
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/zlibstatic.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
contrib/zlib/CMakeFiles/zlibstatic.dir/build: contrib/zlib/libzlibstatic.a
.PHONY : contrib/zlib/CMakeFiles/zlibstatic.dir/build

contrib/zlib/CMakeFiles/zlibstatic.dir/clean:
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib && $(CMAKE_COMMAND) -P CMakeFiles/zlibstatic.dir/cmake_clean.cmake
.PHONY : contrib/zlib/CMakeFiles/zlibstatic.dir/clean

contrib/zlib/CMakeFiles/zlibstatic.dir/depend:
	cd /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4 /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4 /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib /Users/zhouxuguang/work/mycode/GNXEngine/source/thirdparty/assimp-5.2.4/contrib/zlib/CMakeFiles/zlibstatic.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : contrib/zlib/CMakeFiles/zlibstatic.dir/depend

