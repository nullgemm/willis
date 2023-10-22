#!/bin/bash

# get in the right folder
path="$(pwd)/$0"
folder=$(dirname "$path")
cd "$folder"/../.. || exit

# get params
build_type=$1
build_backend=$2
build_toolchain=$3

# set params default values if needed
if [ -z "$build_type" ]; then
	build_type=development
fi

if [ -z "$build_backend" ]; then
	build_backend=wayland
fi

if [ -z "$build_toolchain" ]; then
	build_toolchain=native
fi

# generate ninja files
case $build_backend in
	x11)
		rm -rf build make/output
		./make/lib/elf.sh $build_type
		./make/lib/x11.sh $build_type
	;;

	appkit)
		rm -rf build make/output
		./make/lib/macho.sh $build_type $build_toolchain
		./make/lib/appkit.sh $build_type $build_toolchain
	;;

	win)
		rm -rf build make/output
		./make/lib/pe.sh $build_type
		./make/lib/win.sh $build_type
	;;

	wayland)
		rm -rf build make/output
		./make/lib/elf.sh $build_type
		./make/lib/wayland.sh $build_type
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

	appkit)
		samu -f ./make/output/lib_macho.ninja
		samu -f ./make/output/lib_appkit.ninja

		samu -f ./make/output/lib_macho.ninja headers
		samu -f ./make/output/lib_appkit.ninja headers
	;;

	win)
		ninja -f ./make/output/lib_pe.ninja
		ninja -f ./make/output/lib_win.ninja

		ninja -f ./make/output/lib_pe.ninja headers
		ninja -f ./make/output/lib_win.ninja headers
	;;

	wayland)
		samu -f ./make/output/lib_elf.ninja
		samu -f ./make/output/lib_wayland.ninja

		samu -f ./make/output/lib_elf.ninja headers
		samu -f ./make/output/lib_wayland.ninja headers
	;;

	*)
		echo "invalid platform: $build_backend"
		exit 1
	;;
esac
