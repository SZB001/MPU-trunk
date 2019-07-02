#ifndef KEY_ACCESS_H
#define KEY_ACCESS_H

#include  <stdlib.h>
#include  <stdint.h>

uint32_t saGetKey(uint32_t wSeed, uint8_t mode);
uint16_t calcKey(uint16_t seed);
#endif