#!/bin/bash

ar -x willis_win.a
ar -x ../willis_pe.a

x86_64-w64-mingw32-gcc -shared -o willis_win.dll *.o -lshcore -lgdi32 -ldwmapi
