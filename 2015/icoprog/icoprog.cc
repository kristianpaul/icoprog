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

bool verbose = false;
char current_send_recv_mode = 0;
int current_recv_ep = -1;

void prog_bitstream(bool reset_only = false)
{
	pinMode(RPI_ICE_CLK,     OUTPUT);
	pinMode(RPI_ICE_MOSI,    OUTPUT);
	pinMode(LOAD_FROM_FLASH, OUTPUT);
	pinMode(RPI_ICE_CRESET,  OUTPUT);
	pinMode(RPI_ICE_CS,      OUTPUT);
	pinMode(RPI_ICE_SELECT,  OUTPUT);

	fprintf(stderr, "reset..\n");

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

	fprintf(stderr, "cdone: %s\n", digitalRead(RPI_ICE_CDONE) == HIGH ? "high" : "low");

	if (reset_only)
		return;

	fprintf(stderr, "programming..\n");

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
	fprintf(stderr, "cdone: %s\n", digitalRead(RPI_ICE_CDONE) == HIGH ? "high" : "low");
}

void epsilon_sleep()
{
	for (int i = 0; i < 1000; i++)
		asm volatile ("");
}

void send_word(int v)
{
	if (current_send_recv_mode != 's')
	{
		digitalWrite(RASPI_DIR, HIGH);
		epsilon_sleep();

		pinMode(RASPI_D8, OUTPUT);
		pinMode(RASPI_D7, OUTPUT);
		pinMode(RASPI_D6, OUTPUT);
		pinMode(RASPI_D5, OUTPUT);
		pinMode(RASPI_D4, OUTPUT);
		pinMode(RASPI_D3, OUTPUT);
		pinMode(RASPI_D2, OUTPUT);
		pinMode(RASPI_D1, OUTPUT);
		pinMode(RASPI_D0, OUTPUT);

		current_send_recv_mode = 's';
	}

	if (verbose) {
		fprintf(stderr, "<%03x>", v);
		fflush(stderr);
	}

	digitalWrite(RASPI_D8, (v & 0x100) ? HIGH : LOW);
	digitalWrite(RASPI_D7, (v & 0x080) ? HIGH : LOW);
	digitalWrite(RASPI_D6, (v & 0x040) ? HIGH : LOW);
	digitalWrite(RASPI_D5, (v & 0x020) ? HIGH : LOW);
	digitalWrite(RASPI_D4, (v & 0x010) ? HIGH : LOW);
	digitalWrite(RASPI_D3, (v & 0x008) ? HIGH : LOW);
	digitalWrite(RASPI_D2, (v & 0x004) ? HIGH : LOW);
	digitalWrite(RASPI_D1, (v & 0x002) ? HIGH : LOW);
	digitalWrite(RASPI_D0, (v & 0x001) ? HIGH : LOW);

	epsilon_sleep();
	digitalWrite(RASPI_CLK, HIGH);
	epsilon_sleep();
	digitalWrite(RASPI_CLK, LOW);
	epsilon_sleep();
}

int recv_word(int timeout = 0)
{
	if (current_send_recv_mode != 'r')
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

		digitalWrite(RASPI_DIR, LOW);
		epsilon_sleep();

		current_send_recv_mode = 'r';
	}

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

	epsilon_sleep();
	digitalWrite(RASPI_CLK, HIGH);
	epsilon_sleep();
	digitalWrite(RASPI_CLK, LOW);
	epsilon_sleep();

	if (verbose) {
		fprintf(stderr, "[%03x]", v);
		fflush(stderr);
	}

	if (v >= 0x100)
		current_recv_ep = v & 0xff;

	if (timeout && (v == 0x1ff || v == 0x1fe)) {
		if (timeout == 1) {
			fprintf(stderr, "Timeout!\n");
			exit(1);
		}
		return recv_word(timeout - 1);
	}

	return v;
}

void link_sync(int trignum = -1)
{
	while (recv_word() != 0x1ff) { }

	send_word(0x1ff);
	send_word(0x0ff);

	if (trignum >= 0)
		send_word(trignum);

	while (recv_word() == 0x1fe) { }
}

void test_link()
{
	link_sync();
	send_word(0x100);

	srandom(time(NULL));

	for (int k = 0; k < 1000; k++)
	{
		int data_out[20], data_in[20], data_exp[20];

		fprintf(stderr, "Round %d:\n", k);

		for (int i = 0; i < 20; i++) {
			data_out[i] = random() & 255;
			data_exp[i] = (((data_out[i] << 5) + data_out[i]) ^ 7) & 255;
			send_word(data_out[i]);
		}

		for (int i = 0; i < 20; i++)
			fprintf(stderr, "%5d", data_out[i]);
		fprintf(stderr, "\n");

		for (int i = 0; i < 20; i++)
			do {
				data_in[i] = recv_word(64);
			} while (data_in[i] >= 0x100 || current_recv_ep != 0);

		for (int i = 0; i < 20; i++)
			fprintf(stderr, "%5d", data_in[i]);
		fprintf(stderr, "\n");

		for (int i = 0; i < 20; i++)
			if (data_in[i] == data_exp[i])
				fprintf(stderr, "%5s", "ok");
			else
				fprintf(stderr, " E%3d", data_exp[i]);
		fprintf(stderr, "\n");

		for (int i = 0; i < 20; i++)
			if (data_in[i] != data_exp[i]) {
				fprintf(stderr, "Test(s) failed!\n");
				exit(1);
			}
	}

	fprintf(stderr, "All tests passed.\n");
}

void write_endpoint(int epnum, int trignum)
{
	link_sync(trignum);
	send_word(0x100 + epnum);

	while (1)
	{
		int byte = getchar();
		if (byte < 0 || 255 < byte)
			break;
		send_word(byte);
	}

	link_sync();
}

void read_endpoint(int epnum, int trignum)
{
	link_sync(trignum);

	for (int timeout = 0; timeout < 1000; timeout++) {
		int byte = recv_word();
		if (current_recv_ep == epnum && byte < 0x100) {
			putchar(byte);
			timeout = 0;
		}
	}

	link_sync();
}

void read_dbgvcd(int nbits)
{
	link_sync(1);

	printf("$var event 1 ! clock $end\n");
	for (int i = 0; i < nbits; i++)
		printf("$var wire 1 n%d debug_%d $end\n", i, i);
	printf("$enddefinitions $end\n");

	int nbytes = (nbits+7) / 8;
	int clock_cnt = 0;
	int byte_cnt = 0;

	for (int timeout = 0; timeout < 1000; timeout++)
	{
		int byte = recv_word();

		if (current_recv_ep != 1 || byte >= 0x100)
			continue;

		if (byte_cnt == 0)
			printf("#%d\n1!\n", clock_cnt);

		for (int bit = 0;  8*byte_cnt + bit < nbits && bit < 8; bit++)
			printf("b%d n%d\n", (byte >> bit) & 1, 8*byte_cnt + bit);

		if (++byte_cnt == nbytes) {
			byte_cnt = 0;
			clock_cnt++;
		}

		timeout = 0;
	}

	link_sync();
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

	current_send_recv_mode = 0;
}

void help(const char *progname)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Resetting FPGA:\n");
	fprintf(stderr, "    %s -R\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "Programming FPGA bit stream:\n");
	fprintf(stderr, "    %s -p < data.bin\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "Testing bit-parallel link (using ep0):\n");
	fprintf(stderr, "    %s -T\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "Writing a file to ep N:\n");
	fprintf(stderr, "    %s -w N < data.bin\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "Reading a file from ep N:\n");
	fprintf(stderr, "    %s -r N > data.bin\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "Dumping a VCD file (from ep1, using trig1)\n");
	fprintf(stderr, "  with a debugger core with N bits width:\n");
	fprintf(stderr, "    %s -V N > dbg_trace.vcd\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "Additional options:\n");
	fprintf(stderr, "    -v      verbose output\n");
	fprintf(stderr, "    -t N    send trigger N before -w/-r\n");
	exit(1);
}

int main(int argc, char **argv)
{
	int opt, n = -1, t = -1;
	char mode = 0;

	while ((opt = getopt(argc, argv, "RpTw:r:vt:V:")) != -1)
	{
		switch (opt)
		{
		case 'w':
		case 'r':
		case 'V':
			n = atoi(optarg);
			// fall through

		case 'R':
		case 'p':
		case 'T':
			if (mode)
				help(argv[0]);
			mode = opt;
			break;

		case 'v':
			verbose = true;
			break;

		case 't':
			t = atoi(optarg);
			break;

		default:
			help(argv[0]);
		}
	}

	if (optind != argc || !mode)
		help(argv[0]);

	if (mode == 'R') {
		wiringPiSetup();
		reset_inout();
		prog_bitstream(true);
		reset_inout();
	}
	
	if (mode == 'p') {
		wiringPiSetup();
		reset_inout();
		prog_bitstream();
		reset_inout();
	}

	if (mode == 'T') {
		wiringPiSetup();
		reset_inout();
		test_link();
		reset_inout();
	}

	if (mode == 'w') {
		wiringPiSetup();
		reset_inout();
		write_endpoint(n, t);
		reset_inout();
	}

	if (mode == 'r') {
		wiringPiSetup();
		reset_inout();
		read_endpoint(n, t);
		reset_inout();
	}

	if (mode == 'V') {
		wiringPiSetup();
		reset_inout();
		read_dbgvcd(n);
		reset_inout();
	}

	return 0;
}

