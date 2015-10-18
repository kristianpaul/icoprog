
icoprog: icoprog.cc
	gcc -o icoprog -Wall -Os icoprog.cc -lwiringPi

test: icoprog
	sudo ./icoprog < example.bin

clean:
	rm -f icoprog

.PHONY: test clean

