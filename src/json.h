#include "cJSON.h"
#include <stdbool.h>
#include <stdio.h>

#include "savemng.h"

char *getSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot);

bool setSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot, char *date);