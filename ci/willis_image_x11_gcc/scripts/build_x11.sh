#!/bin/sh

git clone https://github.com/nullgemm/willis.git
cd ./willis || exit

# TODO remove
git checkout appkit

# test build
./make/scripts/build.sh "$@"
