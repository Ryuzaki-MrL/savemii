#include "json.h"

char* doit(char *text)
{
    char *out = NULL;
    cJSON *json = NULL;
    cJSON *readValue = NULL;

    json = cJSON_Parse(text);
    if (!json)
    {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
    }
    else
    {
        cJSON *str;
        str = cJSON_GetObjectItemCaseSensitive(json, "Date");
        out = strdup(str->valuestring);
        return out;
    }
}

/* Read a file, parse, render back, etc. */
char* dofile(char *filename)
{
    FILE *f = NULL;
    long len = 0;
    char *data = NULL;

    /* open in read binary mode */
    f = fopen(filename,"rb");
    /* get the length */
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);

    data = (char*)malloc(len + 1);

    fread(data, 1, len, f);
    data[len] = '\0';
    fclose(f);

    char* stuff = doit(data);
    return stuff;
}

long getFilesize(FILE *fp)
{
	fseek(fp, 0L, SEEK_END);
  
    // calculating the size of the file
    long int res = ftell(fp);
  
    return res;
}

char* getSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot)
{
    char path[PATH_SIZE];
    sprintf(path, "/vol/external01/wiiu/backups/%08x%08x/%u/savemiiMeta.json", highID, lowID, slot);
    if(checkEntry(path)){
        char* info = dofile(path);
	    return info;
    }
    return "";
}

bool setSlotDate(uint32_t highID, uint32_t lowID, uint8_t slot, char* date)
{
    char path[PATH_SIZE];
    sprintf(path, "/vol/external01/wiiu/backups/%08x%08x/%u/savemiiMeta.json", highID, lowID, slot);
    if(checkEntry(path)){
        cJSON *config = cJSON_CreateObject();
        if(config == NULL)
            return false;
        
        cJSON *entry = cJSON_CreateString(date);
        if(entry == NULL)
        {
            cJSON_Delete(config);
            return false;
        }
        cJSON_AddItemToObject(config, "Date", entry);

        char *configString = cJSON_Print(config);
        cJSON_Delete(config);
        if(configString == NULL)
            return false;
        
        FILE *fp = fopen(path, "wb");
        if(fp == NULL)
            return false;
        
        fwrite(configString, strlen(configString), 1, fp);
        
        return true;
    }
    return false;
}