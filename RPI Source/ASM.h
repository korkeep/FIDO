#ifndef _ASM_H
#define _ASM_H

#include <errno.h>
#include <malloc.h>
#include <openssl/sha.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <wiringPi.h>
#include <wiringSerial.h>

int initialize();
int deinitialize();

char* getRegAssertions(char* b_fcParams, char* appID, char* username);
char* getAuthAssertions(char* b_fcParams, char* appID);
int getDeregisterAssertions(char* appID, char* keyID, int len_keyID);
int InitializeSE();

#endif