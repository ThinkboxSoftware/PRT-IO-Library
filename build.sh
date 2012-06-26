#!/bin/sh

g++ -O2 -fPIC -I. -I$HOME/FranticDev/zlib-$(arch)/include -I$HOME/FranticDev/openexr-$(arch)/include/OpenEXR -L$HOME/FranticDev/zlib-$(arch)/lib -L$HOME/FranticDev/openexr-$(arch)/lib example.cpp -lHalf -lz
