#!/bin/sh

# git						cloning the repo
# bash						running the build scripts
# samurai					building the binaries
# gcc						compiling the code
# musl-dev					standard C library
# libxkbcommon-dev			libxkbcommon
# wayland                   wayland libs for client, cursor, egl
# wayland-dev               wayland headers
# wayland-protocols         wayland protocols

apk add --no-cache \
	git \
	bash \
	samurai \
	gcc \
	musl-dev \
	libxkbcommon-dev \
	wayland \
	wayland-dev \
	wayland-protocols
