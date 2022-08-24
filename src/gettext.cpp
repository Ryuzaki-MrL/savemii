/***************************************************************************
 * Copyright (C) 2015 Dimok                                                *
 * Copyright (c) 2022 V10lator <v10lator@myway.de>                         *
 * Copyright (c) 2022 Xpl0itU <DaThinkingChair@protonmail.com>             *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, If not, see <http://www.gnu.org/licenses/>.  *             *
 ***************************************************************************/
#include "gettext.h"
#include "savemng.h"

#include <jansson.h>

#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>

struct MSG;
typedef struct MSG MSG;
struct MSG
{
	uint32_t id;
	const char* msgstr;
	MSG *next;
};

static MSG *baseMSG = NULL;

#define HASHMULTIPLIER 31 // or 37

// Hashing function from https://stackoverflow.com/a/2351171
static inline uint32_t hash_string(const char *str_param)
{
	uint32_t hash = 0;

	while(*str_param != '\0')
		hash = HASHMULTIPLIER * hash + *str_param++;

	return hash;
}

static inline MSG *findMSG(uint32_t id)
{
	for(MSG *msg = baseMSG; msg; msg = msg->next)
		if(msg->id == id)
			return msg;

	return NULL;
}

static void setMSG(const char *msgid, const char *msgstr)
{
	if(!msgstr)
		return;

	uint32_t id = hash_string(msgid);
	MSG *msg = (MSG *)MEMAllocFromDefaultHeap(sizeof(MSG));
	msg->id = id;
	msg->msgstr = strdup(msgstr);
	msg->next = baseMSG;
	baseMSG = msg;
	return;
}

void gettextCleanUp()
{
	while(baseMSG)
	{
		MSG *nextMsg = baseMSG->next;
		MEMFreeToDefaultHeap((void *)(baseMSG->msgstr));
		MEMFreeToDefaultHeap(baseMSG);
		baseMSG = nextMsg;
	}
}

bool gettextLoadLanguage(const char* langFile)
{
	uint8_t *buffer;
	int32_t size = loadFile(langFile, &buffer);
	if(buffer == nullptr)
		return false;

	bool ret = true;
	json_t *json = json_loadb(buffer, size, 0, nullptr);
	if(json)
	{
		size = json_object_size(json);
		if(size != 0)
		{
			const char *key;
			json_t *value;
			json_object_foreach(json, key, value)
				if(json_is_string(value))
					setMSG(key, json_string_value(value));
		}
		else
		{
			ret = false;
		}

		json_decref(json);
	}
	else
	{
		ret = false;
	}

	MEMFreeToDefaultHeap(buffer);
	return ret;
}

const char *gettext(const char *msgid)
{
	MSG *msg = findMSG(hash_string(msgid));
	return msg ? msg->msgstr : msgid;
}