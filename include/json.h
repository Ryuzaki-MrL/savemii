#pragma once

#include <cstdio>
#include <jansson.h>
#include <string>

#include <savemng.h>

std::string getSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot);
bool setSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot, std::string date);