
icoprog: icoprog.cc
	gcc -o icoprog -Wall -Os icoprog.cc -lwiringPi -lstdc++

test: icoprog
	sudo ./icoprog < example.bin

install: icoprog
	sudo install icoprog /usr/local/bin/
	sudo chmod u+s /usr/local/bin/icoprog
clean:
	rm -f icoprog

.PHONY: test clean

