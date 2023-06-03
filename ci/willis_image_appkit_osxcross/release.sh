#!/bin/bash

echo "please clean docker"

sudo rm -rf willis_bin_v* log
sudo docker rm willis_container_appkit_osxcross
sudo docker rmi willis_image_appkit_osxcross
sudo docker rmi alpine:edge

sudo ./build.sh
sudo ./run.sh /scripts/build_appkit.sh release appkit osxcross
sudo ./artifact.sh

sudo chown -R $(id -un):$(id -gn) willis_bin_v*
