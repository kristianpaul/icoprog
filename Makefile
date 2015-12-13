
icoprog: icoprog.cc
	gcc -o icoprog -Wall -Os icoprog.cc -lwiringPi -lrt -lstdc++

example.blif: example.v
	yosys -p 'synth_ice40 -blif example.blif' example.v

example.txt: example.blif example.pcf
	arachne-pnr -d 8k -p example.pcf -o example.txt example.blif

example.bin: example.txt
	icepack example.txt example.bin

example: icoprog example.bin
	sudo ./icoprog -p < example.bin

install: icoprog
	sudo install icoprog /usr/local/bin/
	sudo chmod u+s /usr/local/bin/icoprog
clean:
	rm -f icoprog example.blif example.txt example.bin

.PHONY: example install clean

