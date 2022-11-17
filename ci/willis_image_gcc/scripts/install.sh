#!/bin/sh

# git						cloning the repo
# bash						running the build scripts
# samurai					building the binaries
# gcc						compiling the code
# musl-dev					standard C library
# libxcb-dev				libxcb
# libxkbcommon-dev			libxkbcommon

apk add --no-cache \
	git \
	bash \
	samurai \
	gcc \
	musl-dev \
	libxcb-dev \
	libxkbcommon-dev \
