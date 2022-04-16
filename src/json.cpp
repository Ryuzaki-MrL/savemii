#include "json.h"
#include "string.hpp"

auto doit(char *text) -> char * {
    char *out = nullptr;
    cJSON *json = nullptr;
    cJSON *str;

    json = cJSON_Parse(text);
    if (json == nullptr)
        return (char *) "";
    str = cJSON_GetObjectItemCaseSensitive(json, "Date");
    out = strdup(str->valuestring);
    return out;
}

/* Read a file, parse, render back, etc. */
auto dofile(char *filename) -> char * {
    FILE *f = nullptr;
    long len = 0;
    char *data = nullptr;

    /* open in read binary mode */
    f = fopen(filename, "rb");
    /* get the length */
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);

    data = (char *) malloc(len + 1);

    fread(data, 1, len, f);
    data[len] = '\0';
    fclose(f);

    char *stuff = doit(data);
    free(data);
    return stuff;
}

string getSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot) {
    string path = string_format("sd:/wiiu/backups/%08x%08x/%u/savemiiMeta.json", highID, lowID, slot);
    if (checkEntry(path.c_str()) != 0) {
        string info = dofile((char *)path.c_str());
        return info;
    }
    return "";
}

auto setSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot, string date) -> bool {
    string path = string_format("sd:/wiiu/backups/%08x%08x/%u/savemiiMeta.json", highID, lowID, slot);

    cJSON *config = cJSON_CreateObject();
    if (config == nullptr)
        return false;

    cJSON *entry = cJSON_CreateString(date.c_str());
    if (entry == nullptr) {
        cJSON_Delete(config);
        return false;
    }
    cJSON_AddItemToObject(config, "Date", entry);

    char *configString = cJSON_Print(config);
    cJSON_Delete(config);
    if (configString == nullptr)
        return false;

    FILE *fp = fopen(path.c_str(), "wb");
    if (fp == nullptr)
        return false;

    fwrite(configString, strlen(configString), 1, fp);

    fclose(fp);
    return true;
}
