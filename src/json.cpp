#include "json.h"

char *doit(char *text) {
    char *out = nullptr;
    cJSON *json = nullptr;
    cJSON *str;

    json = cJSON_Parse(text);
    if (!json)
        return "";
    else {
        str = cJSON_GetObjectItemCaseSensitive(json, "Date");
        out = strdup(str->valuestring);
        return out;
    }
}

/* Read a file, parse, render back, etc. */
char *dofile(char *filename) {
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

long getFilesize(FILE *fp) {
    fseek(fp, 0L, SEEK_END);

    // calculating the size of the file
    long int res = ftell(fp);

    return res;
}

char *getSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot) {
    char path[PATH_SIZE];
    sprintf(path, "sd:/wiiu/backups/%08x%08x/%u/savemiiMeta.json", highID, lowID, slot);
    if (checkEntry(path)) {
        char *info = dofile(path);
        return info;
    } else
        return "";
}

bool setSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot, char *date) {
    char path[PATH_SIZE];
    sprintf(path, "sd:/wiiu/backups/%08x%08x/%u/savemiiMeta.json", highID, lowID, slot);

    cJSON *config = cJSON_CreateObject();
    if (config == nullptr)
        return false;

    cJSON *entry = cJSON_CreateString(date);
    if (entry == nullptr) {
        cJSON_Delete(config);
        return false;
    }
    cJSON_AddItemToObject(config, "Date", entry);

    char *configString = cJSON_Print(config);
    cJSON_Delete(config);
    if (configString == nullptr)
        return false;

    FILE *fp = fopen(path, "wb");
    if (fp == nullptr)
        return false;

    fwrite(configString, strlen(configString), 1, fp);

    fclose(fp);
    return true;
}
