/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef cJSON_Utils__h
#define cJSON_Utils__h

#ifdef __cplusplus
extern "C" {
#endif

#include "cJSON.h"

/* Implement RFC6901 (https://tools.ietf.org/html/rfc6901) JSON Pointer spec. */
auto cJSONUtils_GetPointer(cJSON *object, const char *pointer) -> CJSON_PUBLIC(cJSON *);

auto cJSONUtils_GetPointerCaseSensitive(cJSON *object, const char *pointer) -> CJSON_PUBLIC(cJSON *);

/* Implement RFC6902 (https://tools.ietf.org/html/rfc6902) JSON Patch spec. */
/* NOTE: This modifies objects in 'from' and 'to' by sorting the elements by their key */
auto cJSONUtils_GeneratePatches(cJSON *from, cJSON *to) -> CJSON_PUBLIC(cJSON *);

auto cJSONUtils_GeneratePatchesCaseSensitive(cJSON *from, cJSON *to) -> CJSON_PUBLIC(cJSON *);
/* Utility for generating patch array entries. */
CJSON_PUBLIC(void)
cJSONUtils_AddPatchToArray(cJSON *array, const char *operation, const char *path,
                           const cJSON *value);
/* Returns 0 for success. */
auto cJSONUtils_ApplyPatches(cJSON *object, const cJSON *patches) -> CJSON_PUBLIC(int);

auto cJSONUtils_ApplyPatchesCaseSensitive(cJSON *object, const cJSON *patches) -> CJSON_PUBLIC(int);

/*
// Note that ApplyPatches is NOT atomic on failure. To implement an atomic ApplyPatches, use:
//int cJSONUtils_AtomicApplyPatches(cJSON **object, cJSON *patches)
//{
//    cJSON *modme = cJSON_Duplicate(*object, 1);
//    int error = cJSONUtils_ApplyPatches(modme, patches);
//    if (!error)
//    {
//        cJSON_Delete(*object);
//        *object = modme;
//    }
//    else
//    {
//        cJSON_Delete(modme);
//    }
//
//    return error;
//}
// Code not added to library since this strategy is a LOT slower.
*/

/* Implement RFC7386 (https://tools.ietf.org/html/rfc7396) JSON Merge Patch spec. */
/* target will be modified by patch. return value is new ptr for target. */
auto cJSONUtils_MergePatch(cJSON *target, const cJSON *patch) -> CJSON_PUBLIC(cJSON *);

auto cJSONUtils_MergePatchCaseSensitive(cJSON *target, const cJSON *patch) -> CJSON_PUBLIC(cJSON *);
/* generates a patch to move from -> to */
/* NOTE: This modifies objects in 'from' and 'to' by sorting the elements by their key */
auto cJSONUtils_GenerateMergePatch(cJSON *from, cJSON *to) -> CJSON_PUBLIC(cJSON *);

auto cJSONUtils_GenerateMergePatchCaseSensitive(cJSON *from, cJSON *to) -> CJSON_PUBLIC(cJSON *);

/* Given a root object and a target object, construct a pointer from one to the other. */
auto cJSONUtils_FindPointerFromObjectTo(const cJSON *object, const cJSON *target) -> CJSON_PUBLIC(char *);

/* Sorts the members of the object into alphabetical order. */
CJSON_PUBLIC(void)
cJSONUtils_SortObject(cJSON *object);

CJSON_PUBLIC(void)
cJSONUtils_SortObjectCaseSensitive(cJSON *object);

#ifdef __cplusplus
}
#endif

#endif
