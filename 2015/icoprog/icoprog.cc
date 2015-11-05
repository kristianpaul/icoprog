#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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

#define RASPI_D8   0 // PIN 11, GPIO.0
#define RASPI_D7   1 // PIN 12, GPIO.1
#define RASPI_D6   3 // PIN 15, GPIO.3
#define RASPI_D5   4 // PIN 16, GPIO.4
#define RASPI_D4  12 // PIN 19, MOSI
#define RASPI_D3  13 // PIN 21, MISO
#define RASPI_D2  10 // PIN 24, CE0
#define RASPI_D1  24 // PIN 35, GPIO.24
#define RASPI_D0  27 // PIN 36, GPIO.27
#define RASPI_DIR 28 // PIN 38, GPIO.28
#define RASPI_CLK 29 // PIN 40, GPIO.29

void prog_bitstream()
{
	pinMode(RPI_ICE_CLK,     OUTPUT);
	pinMode(RPI_ICE_MOSI,    OUTPUT);
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
	digitalWrite(LOAD_FROM_FLASH, LOW);
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
}

void send_mode()
{
	digitalWrite(RASPI_DIR, HIGH);

	pinMode(RASPI_D8, OUTPUT);
	pinMode(RASPI_D7, OUTPUT);
	pinMode(RASPI_D6, OUTPUT);
	pinMode(RASPI_D5, OUTPUT);
	pinMode(RASPI_D4, OUTPUT);
	pinMode(RASPI_D3, OUTPUT);
	pinMode(RASPI_D2, OUTPUT);
	pinMode(RASPI_D1, OUTPUT);
	pinMode(RASPI_D0, OUTPUT);
}

void recv_mode()
{
	pinMode(RASPI_D8, INPUT);
	pinMode(RASPI_D7, INPUT);
	pinMode(RASPI_D6, INPUT);
	pinMode(RASPI_D5, INPUT);
	pinMode(RASPI_D4, INPUT);
	pinMode(RASPI_D3, INPUT);
	pinMode(RASPI_D2, INPUT);
	pinMode(RASPI_D1, INPUT);
	pinMode(RASPI_D0, INPUT);

	// set twice for extra delay
	digitalWrite(RASPI_DIR, LOW);
	digitalWrite(RASPI_DIR, LOW);
}

void send_word(int v)
{
	digitalWrite(RASPI_D8, (v & 0x100) ? HIGH : LOW);
	digitalWrite(RASPI_D7, (v & 0x080) ? HIGH : LOW);
	digitalWrite(RASPI_D6, (v & 0x040) ? HIGH : LOW);
	digitalWrite(RASPI_D5, (v & 0x020) ? HIGH : LOW);
	digitalWrite(RASPI_D4, (v & 0x010) ? HIGH : LOW);
	digitalWrite(RASPI_D3, (v & 0x008) ? HIGH : LOW);
	digitalWrite(RASPI_D2, (v & 0x004) ? HIGH : LOW);
	digitalWrite(RASPI_D1, (v & 0x002) ? HIGH : LOW);
	digitalWrite(RASPI_D0, (v & 0x001) ? HIGH : LOW);
	digitalWrite(RASPI_CLK, HIGH);
	digitalWrite(RASPI_CLK, LOW);
}

int recv_word()
{
	int v = 0;
	if (digitalRead(RASPI_D8) == HIGH) v |= 0x100;
	if (digitalRead(RASPI_D7) == HIGH) v |= 0x080;
	if (digitalRead(RASPI_D6) == HIGH) v |= 0x040;
	if (digitalRead(RASPI_D5) == HIGH) v |= 0x020;
	if (digitalRead(RASPI_D4) == HIGH) v |= 0x010;
	if (digitalRead(RASPI_D3) == HIGH) v |= 0x008;
	if (digitalRead(RASPI_D2) == HIGH) v |= 0x004;
	if (digitalRead(RASPI_D1) == HIGH) v |= 0x002;
	if (digitalRead(RASPI_D0) == HIGH) v |= 0x001;
	digitalWrite(RASPI_CLK, HIGH);
	digitalWrite(RASPI_CLK, LOW);
	return v;
}

void test_link()
{
	send_mode();
	send_word(0x100);

	srandom(time(NULL));

	for (int k = 0; k < 1000; k++)
	{
		int data_out[20], data_in[20], data_exp[20];

		for (int i = 0; i < 20; i++)
		{
			data_out[i] = random() & 255;
			data_exp[i] = (((data_out[i] << 5) + data_out[i]) ^ 7) & 511;

			send_mode();
			send_word(data_out[i]);

			recv_mode();
			data_in[i] = recv_word();
		}

		printf("Round %d:\n", k);

		for (int i = 0; i < 20; i++)
			printf("%5d", data_out[i]);
		printf("\n");

		for (int i = 0; i < 20; i++)
			printf("%5d", data_in[i]);
		printf("\n");

		for (int i = 0; i < 20; i++)
			printf("%5s", data_in[i] == data_exp[i] ? "ok" : "ERR");
		printf("\n");

		for (int i = 0; i < 20; i++)
			if (data_in[i] != data_exp[i])
				exit(1);
	}

	printf("All tests passed.\n");
}

void prog_image(int command)
{
	send_mode();
	send_word(0x100 + command);

	while (1)
	{
		int byte = getchar();
		if (byte < 0 || 255 < byte)
			break;
		send_word(byte);
	}

	send_word(0x100);
}

void reset_inout()
{
	pinMode(RPI_ICE_CLK,     INPUT);
	pinMode(RPI_ICE_CDONE,   INPUT);
	pinMode(RPI_ICE_MOSI,    INPUT);
	pinMode(RPI_ICE_MISO,    INPUT);
	pinMode(LOAD_FROM_FLASH, INPUT);
	pinMode(RPI_ICE_CRESET,  INPUT);
	pinMode(RPI_ICE_CS,      INPUT);
	pinMode(RPI_ICE_SELECT,  INPUT);

	pinMode(RASPI_D8, INPUT);
	pinMode(RASPI_D7, INPUT);
	pinMode(RASPI_D6, INPUT);
	pinMode(RASPI_D5, INPUT);
	pinMode(RASPI_D4, INPUT);
	pinMode(RASPI_D3, INPUT);
	pinMode(RASPI_D2, INPUT);
	pinMode(RASPI_D1, INPUT);
	pinMode(RASPI_D0, INPUT);

	pinMode(RASPI_DIR, OUTPUT);
	pinMode(RASPI_CLK, OUTPUT);

	digitalWrite(RASPI_DIR, LOW);
	digitalWrite(RASPI_CLK, LOW);
}

int main(int argc, char **argv)
{
	if (argc == 1)
	{
		wiringPiSetup();
		reset_inout();
		prog_bitstream();
		reset_inout();
	}
	else if (argc == 2 && !strcmp(argv[1], "-0"))
	{
		wiringPiSetup();
		reset_inout();
		test_link();
		reset_inout();
	}
	else if (argc == 2 && strlen(argv[1]) == 2 && argv[1][0] == '-' &&
			'1' <= argv[1][1] && argv[1][1] <= '9')
	{
		wiringPiSetup();
		reset_inout();
		prog_image(argv[1][1] - '0');
		reset_inout();
	}
	else
	{
		printf("\n");
		printf("Programming FPGA bit stream:\n");
		printf("    %s < data.bin\n", argv[0]);
		printf("\n");
		printf("Testing bit-parallel link:\n");
		printf("    %s -0\n", argv[0]);
		printf("\n");
		printf("Updating user image 1..9:\n");
		printf("    %s -1 < data.bin\n", argv[0]);
		printf("    %s -2 < data.bin\n", argv[0]);
		printf("    ...\n");
		printf("    %s -9 < data.bin\n", argv[0]);
		printf("\n");
		return 1;
	}

	return 0;
}

