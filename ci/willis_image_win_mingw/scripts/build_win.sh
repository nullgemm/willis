#!/bin/sh

git clone https://github.com/nullgemm/willis.git
cd ./willis || exit

# test build
./make/scripts/build.sh "$@"
