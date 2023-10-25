#!/bin/bash

# example for a release build
# $ ./run.sh /scripts/build_wayland.sh release wayland native
docker run --name willis_container_wayland_gcc willis_image_wayland_gcc "$@" &> log
