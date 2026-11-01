#!/bin/sh
cd "$(dirname "$0")/.."
exe=./build/Release/wallfall-proof.exe
for s in $(cat tests/frames/scenes.txt); do timeout 30 $exe --replay tests/frames/$s $s; done
timeout 30 $exe --sound tests/frames/sound
timeout 30 $exe --pad
