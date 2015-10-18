#!/bin/bash
set -ex
yosys -p 'synth_ice40 -blif example.blif' example.v
arachne-pnr -d 8k -p example.pcf -o example.txt example.blif
icepack example.txt example.bin
rm -f example.txt example.blif
