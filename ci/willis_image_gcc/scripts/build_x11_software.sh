#!/bin/sh

git clone https://github.com/nullgemm/willis.git
cd ./willis || exit

# TODO remove
git checkout next

# test build
./make/scripts/build.sh
