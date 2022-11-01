#!/bin/sh

# git						cloning the repo
# bash						running the build scripts
# samurai					building the binaries
# gcc						compiling the code
# musl-dev					standard C library
# libxcb-dev				libxcb
# xcb-util-xrm-dev			i3gaps xcb xrm helper

apk add --no-cache \
	git \
	bash \
	samurai \
	gcc \
	musl-dev \
	libxcb-dev \
	xcb-util-xrm-dev
