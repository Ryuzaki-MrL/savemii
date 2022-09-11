#include <json.h>
#include <string.hpp>

#define FS_ALIGN(x) ((x + 0x3F) & ~(0x3F))

static json_t *loadJSON(const char *text) {
    json_t *root;
    json_error_t error;

    root = json_loads(text, 0, &error);

    if (root)
        return root;
    else
        return (json_t *) 0;
}

static std::string getDateFromText(char *text) {
    char *out = nullptr;
    json_t *root = loadJSON(text);
    if (root == nullptr)
        return (char *) "";
    out = (char *) json_string_value(json_object_get(root, "Date"));
    std::string buf;
    buf.assign(out);
    json_decref(root);
    return buf;
}

/* Read a file, parse, render back, etc. */
static std::string getDateFromFile(char *filename) {
    FILE *f = nullptr;
    long len = 0;

    /* open in read binary mode */
    f = fopen(filename, "rb");
    /* get the length */
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *data = (char *) aligned_alloc(0x40, FS_ALIGN(len + 1));

    fread(data, 1, len, f);
    data[len] = '\0';
    fclose(f);

    std::string date = getDateFromText(data);
    free(data);
    return date;
}

std::string getSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot) {
    std::string path = stringFormat("sd:/wiiu/backups/%08x%08x/%u/savemiiMeta.json", highID, lowID, slot);
    if (checkEntry(path.c_str()) != 0) {
        std::string info = getDateFromFile((char *) path.c_str());
        return info;
    }
    return "";
}

bool setSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot, std::string date) {
    std::string path = stringFormat("sd:/wiiu/backups/%08x%08x/%u/savemiiMeta.json", highID, lowID, slot);

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
