#include <stdio.h>
#include <stdlib.h>

#include "../verbalEyes_speed_controller.h"

void verbaleyes_network_connect(const char* ssid, const char* key) {
}
bool verbaleyes_network_connected() {
	return 1;
}
uint32_t verbaleyes_network_getip() {
	return 1;
}
void verbaleyes_socket_connect(const char* host, const unsigned short port) {
}
bool verbaleyes_socket_connected() {
	return 0;
}
short verbaleyes_socket_read() {
	return 1;
}
void verbaleyes_socket_write(const char* str, const unsigned int len) {
}
char verbaleyes_conf_read(const unsigned short addr) {
  return 1;
}



char eeprom[1024];
void verbaleyes_conf_write(const unsigned short addr, const char c) {
	if (addr > 1023) {
		printf("ERROR: Received unexpectedly large address %d\n", addr);
		exit(1);
	}

	eeprom[addr] = c;
}

void test_write(int offset, int len, char *data) {
	if (memcmp(data, eeprom + offset, len)) {
		printf("ERROR: EEPROM does not match data\n");
		for (int i = 0; i < len; i++) {
			printf("#%d\t%d\t%d\n", i, (unsigned char)data[i], (unsigned char)eeprom[i + offset]);
		}
		exit(1);
	}
	else {
		printf("===\nWritten data matches\n");
	}
}

bool commited = 0;
void verbaleyes_conf_commit() {
	commited = 1;
}

void test_commit() {
	if (commited != 1) {
		printf("\nShould have commited\n");
		exit(1);
	}
	else {
		printf("\nHas commited\n");
	}
}

int log_buffer_index = 0;
char log_buffer[1024];

void verbaleyes_log(const char* str, const uint32_t len) {
	for (int i = 0; i < len; i++) {
		log_buffer[log_buffer_index++] = str[i];
	}

	printf("%s", str);
	fflush(stdout);
}

void test_log(char *msg) {
	if (strcmp(msg, log_buffer)) {
		printf("ERROR: Logged output did not match\nMSG:\n%sBUFFER:\n%s", msg, log_buffer);
		exit(1);
	}
	else {
		printf("Logs matched\nTest successfull\n");
	}
}

int id = 1;
int index2 = 0;
void test_reset() {
	printf("\nTest Started #%d\n===\n", id++);
	index2 = 0;
	log_buffer_index = 0;
	memset(log_buffer, '\0', 1024);
	memset(eeprom, '\0', 1024);
}

bool test_update(char *str) {
	return updateConfig(str[index2++]);
}






int main(void) {
	// -129-x

	// 00000000 = 0 		| 0
	// 00000001 = 1 		| 1
	// 01111111 = 127 		| 127

	// 10000000 = -128		| -1
	// 10000001 = -127 		| -2
	// 11111111 = -1		| -128

	// printf("%d\n", (signed char)0b00000000);
	// printf("%d\n", (signed char)0b00000001);
	// printf("%d\n", (signed char)0b01111111);
	//
	// printf("%d\n", (signed char)0b10000000);
	// printf("%d\n", (signed char)0b10000001);
	// printf("%d\n", (signed char)0b11111111);
	//
	// printf("%d\n", (signed char)(-129-0b10000000));
	// printf("%d\n", (signed char)(-129-0b10000001));
	// printf("%d\n", (signed char)(-129-0b11111111));
	//
	// return 0;
	// 1 =nn
	{
		test_reset();
		while (test_update("=\n\n"));
		test_write(0, 32, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
		test_log("\r\n[ ] No matching key\r\nDone\r\n");
	}

	// 2 =xxxnn
	{
		test_reset();
		while (test_update("=xxx\n\n"));
		test_write(0, 32, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
		test_log("\r\n[ ] No matching key\r\nDone\r\n");
	}

	// 3 ==nn
	{
		test_reset();
		while (test_update("==\n\n"));
		test_write(0, 32, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
		test_log("\r\n[ ] No matching key\r\nDone\r\n");
	}

	// 4 xx=xxxnn
	{
		test_reset();
		while (test_update("xx=xxx\n\n"));
		test_write(0, 32, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
		test_log("\r\n[ xx ] No matching key\r\nDone\r\n");
	}

	// 5 xx=xxxnxxnn
	{
		test_reset();
		while (test_update("xx=xxx\nxx\n\n"));
		test_write(0, 32, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
		test_log("\r\n[ xx ] No matching key\r\n[ xx ] Aborted\r\nDone\r\n");
	}

	// 6 x000y000z000nn
	{
		test_reset();
		while (test_update("x\0\0\0y\0\0\0z\0\0\0\n\n"));
		test_write(0, 32, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
		test_log("\r\n[ xyz ] Aborted\r\nDone\r\n");
	}

	// 7 nn
	{
		test_reset();
		while (test_update("\n\n"));
		test_write(0, 32, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
		test_log("");
	}

	// 8 xxxnn
	{
		test_reset();
		while (test_update("xxx\n\n"));
		test_write(0, 32, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
		test_log("\r\n[ xxx ] Aborted\r\nDone\r\n");
	}

	// 9 ssidnn
	{
		test_reset();
		while (test_update("ssid\n\n"));
		test_write(0, 32, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
		test_log("\r\n[ ssid ] Aborted\r\nDone\r\n");
	}

	// 10 ssid=nn
	{
		test_reset();
		while (test_update("ssid=\n\n"));
		test_write(0, 32, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
		test_log("\r\n[ ssid ] is now: \r\nDone\r\n");
	}

	// 11 ssid=123nn
	{
		test_reset();
		while (test_update("ssid=123\n\n"));
		test_write(0, 32, "123\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
		test_log("\r\n[ ssid ] is now: 123\r\nDone\r\n");
	}

	// 12 ssid=123123123123123123123123123123123123nn
	{
		test_reset();
		while (test_update("ssid=123123123123123123123123123123123123\n\n"));
		test_write(0, 32, "12312312312312312312312312312312");
		test_log("\r\n[ ssid ] is now: 12312312312312312312312312312312\r\nMaximum input length reached\r\nDone\r\n");
	}

	// 13 ssid=bananann
	{
		test_reset();
		strcpy(eeprom, "12312312312312312312312312312312");
		while (test_update("ssid=banana\n\n"));
		test_write(0, 32, "banana\0002312312312312312312312312");
		test_log("\r\n[ ssid ] is now: banana\r\nDone\r\n");
	}

	// 14 ssid=123===nn
	{
		test_reset();
		while (test_update("ssid=123===\n\n"));
		test_write(0, 32, "123===\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
		test_log("\r\n[ ssid ] is now: 123===\r\nDone\r\n");
	}

	// 15 0000
	{
		test_reset();
		while (test_update("\0\0\0\0\n\n"));
		test_write(0, 32, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
		test_log("");
	}

	// 16 port=1nn
	{
		test_reset();
		while (test_update("port=1\n\n"));
		test_write(128, 2, "\0\1");
		test_log("\r\n[ port ] is now: 1\r\nDone\r\n");
	}

	// 17 port=17nn
	{
		test_reset();
		while (test_update("port=17\n\n"));
		test_write(128, 2, "\0\x11");
		test_log("\r\n[ port ] is now: 17\r\nDone\r\n");
	}

	// 18 port=1024nn
	{
		test_reset();
		while (test_update("port=1024\n\n"));
		test_write(128, 2, "\4\0");
		test_log("\r\n[ port ] is now: 1024\r\nDone\r\n");
	}

	// 19 speedmin=-1nn
	{
		test_reset();
		while (test_update("speedmin=-1\n\n"));
		test_write(226, 2, "\xff\xff");
		test_log("\r\n[ speedmin ] is now: -1\r\nDone\r\n");
	}

	// 20 port=-1024nn
	{
		test_reset();
		while (test_update("port=-1024\n\n"));
		test_write(128, 2, "\xfc\0");
		test_log("\r\n[ port ] is now: -1024\r\nDone\r\n");
	}

	// 20 port=1abc7nn
	{
		test_reset();
		while (test_update("port=1abc7\n\n"));
		test_write(128, 2, "\0\x11");
		test_log("\r\n[ port ] is now: 1abc7\r\nIgnored non-numerical characters for a value of: 17\r\nDone\r\n");
	}

	// 21 port=-9999nn
	{
		// test_reset();
		// while (test_update("port=-99999\n\n"));
		// test_write(128, 2, "\4\0");
		// test_log("\r\n[ port ] is now: -99999\r\nClamped up...\r\nDone\r\n");
	}

	// 22 port=65535nn
	{
		test_reset();
		while (test_update("port=65535\n\n"));
		test_write(128, 2, "\xff\xff");
		test_log("\r\n[ port ] is now: 65535\r\nDone\r\n");
	}

	// 23 port=65536nn
	{
		test_reset();
		while (test_update("port=65536\n\n"));
		test_write(128, 2, "\xff\xff");
		//test_log("\r\n[ port ] is now: 65356\r\nClamped down..\r\nDone\r\n");
	}

	// 24 port=6abc5533nn
	{
		test_reset();
		while (test_update("port=6abc5533\n\n"));
		test_write(128, 2, "\xff\xfd");
		test_log("\r\n[ port ] is now: 6abc5533\r\nIgnored non-numerical characters for a value of: 65533\r\nDone\r\n");
	}

	// LAST xx0000...
	{
		test_reset();
		updateConfig('x');
		updateConfig('x');
		while (updateConfig('\0'));
		if (index == 1) {
			printf("\nERROR: Should have returned 1 first time and 0 second, index is %d\n", index2);
			exit(1);
		}
		test_log("\r\n[ xx ] Aborted\r\nDone\r\n");
	}

	return 0;
}
