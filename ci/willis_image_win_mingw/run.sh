#!/bin/bash

# example for a release build
# $ ./run.sh /scripts/build_win.sh release win native
docker run --name willis_container_win_mingw willis_image_win_mingw "$@" &> log
