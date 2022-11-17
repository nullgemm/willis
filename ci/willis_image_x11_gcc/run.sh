#!/bin/bash

# example for a release build
# $ ./run.sh /scripts/build_x11.sh release x11
docker run --name willis_container_x11_gcc willis_image_x11_gcc "$@" &> log
