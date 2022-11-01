#!/bin/bash

docker run --name willis_container_gcc willis_image_gcc "$@" &> log
