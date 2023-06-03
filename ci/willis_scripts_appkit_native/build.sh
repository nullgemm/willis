#!/bin/sh

git clone https://github.com/nullgemm/willis.git
cd ./willis || exit

# TODO remove
git checkout next

# test build
export PATH=/scripts/sdk/target/bin/:/bin:/sbin:/usr/bin:/usr/sbin
export LD_LIBRARY_PATH=/scripts/sdk/target/lib/
./make/scripts/build.sh release appkit native
