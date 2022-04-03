#include "cJSON.h"
#include <stdbool.h>
#include <stdio.h>
#include <string>

#include "savemng.hpp"
#include "string.hpp"
using namespace std;

string getSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot);
bool setSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot, string date);