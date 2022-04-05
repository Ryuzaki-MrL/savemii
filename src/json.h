#ifndef _JSON_H
#define _JSON_H

#include "cJSON.h"

#include <cstdio>

#include "savemng.h"

auto getSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot) -> char *;

auto setSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot, char *date) -> bool;

#endif