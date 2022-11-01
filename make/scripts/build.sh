#!/bin/bash

# get in the right folder
path="$(pwd)/$0"
folder=$(dirname "$path")
cd "$folder"/../.. || exit

# get params
build_type=$1
build_backend=$2

# set params default values if needed
if [ -z "$build_type" ]; then
	build_type=development
fi

if [ -z "$build_backend" ]; then
	build_backend=x11
fi

# generate ninja files
case $build_backend in
	x11)
		rm -rf build make/output
		./make/lib/elf.sh $build_type
		./make/lib/x11.sh $build_type
	;;

	*)
		echo "invalid backend: $build_backend"
		exit 1
	;;
esac

# build
case $build_backend in
	x11)
		samu -f ./make/output/lib_elf.ninja
		samu -f ./make/output/lib_x11.ninja

		samu -f ./make/output/lib_elf.ninja headers
		samu -f ./make/output/lib_x11.ninja headers
	;;

	*)
		echo "invalid platform: $build_backend"
		exit 1
	;;
esac
