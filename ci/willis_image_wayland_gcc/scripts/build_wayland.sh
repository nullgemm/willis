#!/bin/sh

git clone https://github.com/nullgemm/willis.git
cd ./willis || exit

# TODO remove
git checkout next

# get protocols
./make/scripts/wayland_get.sh

# test build
./make/scripts/build.sh "$@"
