#include "json.h"
#include "string.hpp"

json_t *load_json(const char *text) {
    json_t *root;
    json_error_t error;

    root = json_loads(text, 0, &error);

    if (root)
        return root;
    else
        return (json_t *)0;
}

auto doit(char *text) -> char * {
    char *out = nullptr;
    json_t *root = load_json(text);
    if(root == nullptr)
        return (char *) "";
    out = json_string_value(json_object_get(root, "Date"));
    json_decref(root);
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
  
    json_t *config = json_object();
    if (config == nullptr)
        return false;

    json_object_set_new(config, "Date", json_string(date.c_str()));

    char *configString = json_dumps(config, 0);
    if (configString == nullptr)
        return false;
  
    json_decref(config);

    FILE *fp = fopen(path.c_str(), "wb");
    if (fp == nullptr)
        return false;

    fwrite(configString, strlen(configString), 1, fp);

    fclose(fp);
    return true;
}
