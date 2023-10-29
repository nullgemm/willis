#!/bin/sh

git clone https://github.com/nullgemm/willis.git
cd ./willis || exit

# get protocols
./make/scripts/wayland_get.sh

# test build
./make/scripts/build.sh "$@"
