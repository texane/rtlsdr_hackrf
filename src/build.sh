#!/usr/bin/env sh
gcc -fPIC -shared -Wall -O2 librtlsdr.c -o librtlsdr.so.0 -lhackrf -lpthread -lusb
