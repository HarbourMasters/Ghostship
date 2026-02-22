#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t textRand[];
extern uint8_t textMarioRando[];


void PortEnhancements_Register();
void PortEnhancements_Init();
void PortEnhancements_Exit();

#ifdef __cplusplus
};
#endif