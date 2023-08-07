#!/bin/sh

git clone https://github.com/nullgemm/willis.git
cd ./willis || exit

# TODO remove
git checkout win32

# test build
./make/scripts/build.sh "$@"
