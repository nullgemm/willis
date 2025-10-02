#!/bin/bash

# get into the script's folder
cd "$(dirname "$0")" || exit
cd ../../..

# utilitary variables
tag=$(git tag --sort v:refname | tail -n 1)
folder_ninja="build"
folder_objects="$folder_ninja/shared"
folder_willis="willis_bin_$tag"
folder_library="$folder_willis/lib/willis"
mkdir -p "$folder_objects"

# list link flags (order matters)
link+=("-lxcb-errors")
link+=("-lxcb-shm")
link+=("-lxcb")
link+=("-lxcb-cursor")
link+=("-lxcb-image")
link+=("-lxcb-randr")
link+=("-lxcb-render")
link+=("-lxcb-render-util")
link+=("-lxcb-sync")
link+=("-lxcb-xfixes")
link+=("-lxcb-xinput")
link+=("-lxcb-xkb")
link+=("-lxcb-xrm")
link+=("-lxkbcommon")
link+=("-lxkbcommon-x11")
link+=("-lpthread")

# list objs (order matters)
obj+=("$folder_objects/willis_x11.o")
obj+=("$folder_objects/willis_elf.o")

# parse soname
soname="$folder_library/x11/willis_x11.so"

# extract objects from static archives
ar --output "$folder_objects" -x "$folder_library/x11/willis_x11.a"
ar --output "$folder_objects" -x "$folder_library/willis_elf.a"

# build shared object
gcc -shared -o $soname "${obj[@]}" "${link[@]}"
