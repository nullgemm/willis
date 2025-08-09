#!/bin/bash

ar -x willis_wayland.a
ar -x ../willis_elf.a
gcc -shared -o willis_wayland.so *.o -lwayland-client -lwayland-cursor -lxkbcommon -lpthread
