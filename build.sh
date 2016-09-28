#!/bin/bash

CMAKE=/Applications/CMake.app/Contents/bin/cmake

BUILD_DIRS="$PWD/ $PWD/test"

for dir in $BUILD_DIRS;
do
    cd $dir/cmake
	echo "Current directory $PWD..."
	echo "Generating makefiles..."
	$CMAKE -G"Unix Makefiles" ..

	echo "Building $PWD..."
	make -f Makefile
done

echo "Build completed..."



