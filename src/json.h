#ifndef _JSON_H
#define _JSON_H

#include "cJSON.h"

#include <cstdio>
#include <string>

#include "savemng.h"

std::string getSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot);

auto setSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot, char *date) -> bool;

#endif