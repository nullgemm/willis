#!/bin/bash

# get in the right folder
path="$(pwd)/$0"
folder=$(dirname "$path")
cd "$folder"/../.. || exit

# get wayland headers
mkdir -p res/wayland_headers

wayland-scanner private-code \
	< /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml \
	> res/wayland_headers/zwp-pointer-constraints-protocol.c
wayland-scanner client-header \
	< /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml \
	> res/wayland_headers/zwp-pointer-constraints-protocol.h

wayland-scanner private-code \
	< /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml \
	> res/wayland_headers/zwp-relative-pointer-protocol.c
wayland-scanner client-header \
	< /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml \
	> res/wayland_headers/zwp-relative-pointer-protocol.h
