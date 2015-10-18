#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <wiringPi.h>

#define RPI_ICE_CLK      7 // PIN  7, GPIO.7
#define RPI_ICE_CDONE    2 // PIN 13, GPIO.2
#define RPI_ICE_MOSI    21 // PIN 29, GPIO.21
#define RPI_ICE_MISO    22 // PIN 31, GPIO.22
#define LOAD_FROM_FLASH 23 // PIN 33, GPIO.23
#define RPI_ICE_CRESET  25 // PIN 37, GPIO.25
#define RPI_ICE_CS      11 // PIN 26, CE1
#define RPI_ICE_SELECT  26 // PIN 32, GPIO.26

int main()
{
	wiringPiSetup();

	pinMode(RPI_ICE_CLK,     OUTPUT);
	pinMode(RPI_ICE_CDONE,   INPUT);
	pinMode(RPI_ICE_MOSI,    OUTPUT);
	pinMode(RPI_ICE_MISO,    INPUT);
	pinMode(LOAD_FROM_FLASH, OUTPUT);
	pinMode(RPI_ICE_CRESET,  OUTPUT);
	pinMode(RPI_ICE_CS,      OUTPUT);
	pinMode(RPI_ICE_SELECT,  OUTPUT);

	printf("reset..\n");

	// enable reset
	digitalWrite(RPI_ICE_CRESET, LOW);

	// start clock high
	digitalWrite(RPI_ICE_CLK, HIGH);

	// select SRAM programming mode
	digitalWrite(LOAD_FROM_FLASH, HIGH);
	digitalWrite(RPI_ICE_SELECT, LOW);
	digitalWrite(RPI_ICE_CS, LOW);
	usleep(100);

	// release reset
	digitalWrite(RPI_ICE_CRESET, HIGH);
	usleep(2000);

	printf("cdone: %s\n", digitalRead(RPI_ICE_CDONE) == HIGH ? "high" : "low");

	printf("programming..\n");

	while (1)
	{
		int byte = getchar();
		if (byte < 0)
			break;
		for (int i = 7; i >= 0; i--) {
			digitalWrite(RPI_ICE_MOSI, ((byte >> i) & 1) ? HIGH : LOW);
			digitalWrite(RPI_ICE_CLK, LOW);
			digitalWrite(RPI_ICE_CLK, HIGH);
		}
	}

	for (int i = 0; i < 49; i++) {
		digitalWrite(RPI_ICE_CLK, LOW);
		digitalWrite(RPI_ICE_CLK, HIGH);
	}

	usleep(2000);
	printf("cdone: %s\n", digitalRead(RPI_ICE_CDONE) == HIGH ? "high" : "low");

	return 0;
}

