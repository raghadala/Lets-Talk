#ifndef _HELPER_H_
#define _HELPER_H_
#include <stdbool.h>

void* keyboardInput(void* args);

void* sendUDP(void* args);

void* receiveUDP(void* args);

void* screenDisplay(void* args);

void* terminate_everything();

#endif