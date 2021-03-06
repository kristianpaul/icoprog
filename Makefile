
icoprog: icoprog.cc
	gcc -o icoprog -Wall -Os icoprog.cc -lwiringPi -lrt -lstdc++

example.blif: example.v
	yosys -p 'synth_ice40 -blif example.blif' example.v

example.txt: example.blif example.pcf
	arachne-pnr -d 8k -p example.pcf -o example.txt example.blif

example.bin: example.txt
	icepack example.txt example.bin

example_sram: icoprog example.bin
	sudo ./icoprog -p < example.bin

example_flash: icoprog example.bin
	sudo ./icoprog -f < example.bin
	sudo ./icoprog -b

reset: icoprog
	sudo ./icoprog -f < example.pcf
	sudo ./icoprog -b

install: icoprog
	sudo install icoprog /usr/local/bin/
	sudo chmod u+s /usr/local/bin/icoprog
clean:
	rm -f icoprog example.blif example.txt example.bin

.PHONY: example_sram example_flash reset install clean

