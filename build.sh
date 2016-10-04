#!/bin/bash

CMAKE=/Applications/CMake.app/Contents/bin/cmake
#CMAKE="cmake -DCMAKE_BUILD_TYPE=Debug"

BUILD_DIRS="$PWD/ $PWD/test"

for dir in $BUILD_DIRS;
do
    if [ ! -d "$dir/cmake" ]; then
	cd $dir
	echo "Creating make directory [$dir/cmake]..."
	mkdir cmake
	cd -
    fi
    cd $dir/cmake
	echo "Current directory $PWD..."
	echo "Generating makefiles..."
	$CMAKE -G"Unix Makefiles" ..

	echo "Building $PWD..."
	make -f Makefile
done

echo "Build completed..."



