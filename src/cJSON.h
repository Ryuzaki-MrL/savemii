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

#ifndef cJSON__h
#define cJSON__h

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__

/* When compiling for windows, we specify a specific calling convention to avoid issues where we are being called from a project with a different default calling convention.  For windows you have 3 define options:

CJSON_HIDE_SYMBOLS - Define this in the case where you don't want to ever dllexport symbols
CJSON_EXPORT_SYMBOLS - Define this on library build when you want to dllexport symbols (default)
CJSON_IMPORT_SYMBOLS - Define this if you want to dllimport symbol

For *nix builds that support visibility attribute, you can define similar behavior by

setting default visibility to hidden by adding
-fvisibility=hidden (for gcc)
or
-xldscope=hidden (for sun cc)
to CFLAGS

then using the CJSON_API_VISIBILITY flag to "export" the same symbols the way CJSON_EXPORT_SYMBOLS does

*/

#define CJSON_CDECL   __cdecl
#define CJSON_STDCALL __stdcall

/* export symbols by default, this is necessary for copy pasting the C and header file */
#if !defined(CJSON_HIDE_SYMBOLS) && !defined(CJSON_IMPORT_SYMBOLS) && !defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_EXPORT_SYMBOLS
#endif

#if defined(CJSON_HIDE_SYMBOLS)
#define CJSON_PUBLIC(type) type CJSON_STDCALL
#elif defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_PUBLIC(type) __declspec(dllexport) type CJSON_STDCALL
#elif defined(CJSON_IMPORT_SYMBOLS)
#define CJSON_PUBLIC(type) __declspec(dllimport) type CJSON_STDCALL
#endif
#else /* !__WINDOWS__ */
#define CJSON_CDECL
#define CJSON_STDCALL

#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined(__SUNPRO_C)) && defined(CJSON_API_VISIBILITY)
#define CJSON_PUBLIC(type) __attribute__((visibility("default"))) type
#else
#define CJSON_PUBLIC(type) type
#endif
#endif

/* project version */
#define CJSON_VERSION_MAJOR 1
#define CJSON_VERSION_MINOR 7
#define CJSON_VERSION_PATCH 15

#include <cstddef>

/* cJSON Types: */
#define cJSON_Invalid       (0)
#define cJSON_False         (1 << 0)
#define cJSON_True          (1 << 1)
#define cJSON_NULL          (1 << 2)
#define cJSON_Number        (1 << 3)
#define cJSON_String        (1 << 4)
#define cJSON_Array         (1 << 5)
#define cJSON_Object        (1 << 6)
#define cJSON_Raw           (1 << 7) /* raw json */

#define cJSON_IsReference   256
#define cJSON_StringIsConst 512

/* The cJSON structure: */
using cJSON = struct cJSON {
    /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct cJSON *next;
    struct cJSON *prev;
    /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
    struct cJSON *child;

    /* The type of the item, as above. */
    int type;

    /* The item's string, if type==cJSON_String  and type == cJSON_Raw */
    char *valuestring;
    /* writing to valueint is DEPRECATED, use cJSON_SetNumberValue instead */
    int valueint;
    /* The item's number, if type==cJSON_Number */
    double valuedouble;

    /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    char *string;
};

using cJSON_Hooks = struct cJSON_Hooks {
    /* malloc/free are CDECL on Windows regardless of the default calling convention of the compiler, so ensure the hooks allow passing those functions directly. */
    void *(CJSON_CDECL *malloc_fn)(size_t sz);

    void(CJSON_CDECL *free_fn)(void *ptr);
};

using cJSON_bool = int;

/* Limits how deeply nested arrays/objects can be before cJSON rejects to parse them.
 * This is to prevent stack overflows. */
#ifndef CJSON_NESTING_LIMIT
#define CJSON_NESTING_LIMIT 1000
#endif

/* returns the version of cJSON as a string */
CJSON_PUBLIC(const char *)
cJSON_Version(void);

/* Supply malloc, realloc and free functions to cJSON */
CJSON_PUBLIC(void)
cJSON_InitHooks(cJSON_Hooks *hooks);

/* Memory Management: the caller is always responsible to free the results from all variants of cJSON_Parse (with cJSON_Delete) and cJSON_Print (with stdlib free, cJSON_Hooks.free_fn, or cJSON_free as appropriate). The exception is cJSON_PrintPreallocated, where the caller has full responsibility of the buffer. */
/* Supply a block of JSON, and this returns a cJSON object you can interrogate. */
auto cJSON_Parse(const char *value) -> CJSON_PUBLIC(cJSON *);

auto cJSON_ParseWithLength(const char *value, size_t buffer_length) -> CJSON_PUBLIC(cJSON *);
/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails, then return_parse_end will contain a pointer to the error so will match cJSON_GetErrorPtr(). */
auto cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated) -> CJSON_PUBLIC(cJSON *);

auto cJSON_ParseWithLengthOpts(const char *value, size_t buffer_length, const char **return_parse_end,
                               cJSON_bool require_null_terminated) -> CJSON_PUBLIC(cJSON *);

/* Render a cJSON entity to text for transfer/storage. */
auto cJSON_Print(const cJSON *item) -> CJSON_PUBLIC(char *);
/* Render a cJSON entity to text for transfer/storage without any formatting. */
auto cJSON_PrintUnformatted(const cJSON *item) -> CJSON_PUBLIC(char *);
/* Render a cJSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
auto cJSON_PrintBuffered(const cJSON *item, int prebuffer, cJSON_bool fmt) -> CJSON_PUBLIC(char *);
/* Render a cJSON entity to text using a buffer already allocated in memory with given length. Returns 1 on success and 0 on failure. */
/* NOTE: cJSON is not always 100% accurate in estimating how much memory it will use, so to be safe allocate 5 bytes more than you actually need */
auto cJSON_PrintPreallocated(cJSON *item, char *buffer, int length, cJSON_bool format) -> CJSON_PUBLIC(cJSON_bool);
/* Delete a cJSON entity and all subentities. */
CJSON_PUBLIC(void)
cJSON_Delete(cJSON *item);

/* Returns the number of items in an array (or object). */
auto cJSON_GetArraySize(const cJSON *array) -> CJSON_PUBLIC(int);
/* Retrieve item number "index" from array "array". Returns NULL if unsuccessful. */
auto cJSON_GetArrayItem(const cJSON *array, int index) -> CJSON_PUBLIC(cJSON *);
/* Get item "string" from object. Case insensitive. */
auto cJSON_GetObjectItem(const cJSON *object, const char *string) -> CJSON_PUBLIC(cJSON *);

auto cJSON_GetObjectItemCaseSensitive(const cJSON *object, const char *string) -> CJSON_PUBLIC(cJSON *);

auto cJSON_HasObjectItem(const cJSON *object, const char *string) -> CJSON_PUBLIC(cJSON_bool);
/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when cJSON_Parse() returns 0. 0 when cJSON_Parse() succeeds. */
CJSON_PUBLIC(const char *)
cJSON_GetErrorPtr(void);

/* Check item type and return its value */
auto cJSON_GetStringValue(const cJSON *item) -> CJSON_PUBLIC(char *);

auto cJSON_GetNumberValue(const cJSON *item) -> CJSON_PUBLIC(double);

/* These functions check the type of an item */
auto cJSON_IsInvalid(const cJSON *item) -> CJSON_PUBLIC(cJSON_bool);

auto cJSON_IsFalse(const cJSON *item) -> CJSON_PUBLIC(cJSON_bool);

auto cJSON_IsTrue(const cJSON *item) -> CJSON_PUBLIC(cJSON_bool);

auto cJSON_IsBool(const cJSON *item) -> CJSON_PUBLIC(cJSON_bool);

auto cJSON_IsNull(const cJSON *item) -> CJSON_PUBLIC(cJSON_bool);

auto cJSON_IsNumber(const cJSON *item) -> CJSON_PUBLIC(cJSON_bool);

auto cJSON_IsString(const cJSON *item) -> CJSON_PUBLIC(cJSON_bool);

auto cJSON_IsArray(const cJSON *item) -> CJSON_PUBLIC(cJSON_bool);

auto cJSON_IsObject(const cJSON *item) -> CJSON_PUBLIC(cJSON_bool);

auto cJSON_IsRaw(const cJSON *item) -> CJSON_PUBLIC(cJSON_bool);

/* These calls create a cJSON item of the appropriate type. */
auto cJSON_CreateNull(void) -> CJSON_PUBLIC(cJSON *);

auto cJSON_CreateTrue(void) -> CJSON_PUBLIC(cJSON *);

auto cJSON_CreateFalse(void) -> CJSON_PUBLIC(cJSON *);

auto cJSON_CreateBool(cJSON_bool boolean) -> CJSON_PUBLIC(cJSON *);

auto cJSON_CreateNumber(double num) -> CJSON_PUBLIC(cJSON *);

auto cJSON_CreateString(const char *string) -> CJSON_PUBLIC(cJSON *);
/* raw json */
auto cJSON_CreateRaw(const char *raw) -> CJSON_PUBLIC(cJSON *);

auto cJSON_CreateArray(void) -> CJSON_PUBLIC(cJSON *);

auto cJSON_CreateObject(void) -> CJSON_PUBLIC(cJSON *);

/* Create a string where valuestring references a string so
 * it will not be freed by cJSON_Delete */
auto cJSON_CreateStringReference(const char *string) -> CJSON_PUBLIC(cJSON *);
/* Create an object/array that only references it's elements so
 * they will not be freed by cJSON_Delete */
auto cJSON_CreateObjectReference(const cJSON *child) -> CJSON_PUBLIC(cJSON *);

auto cJSON_CreateArrayReference(const cJSON *child) -> CJSON_PUBLIC(cJSON *);

/* These utilities create an Array of count items.
 * The parameter count cannot be greater than the number of elements in the number array, otherwise array access will be out of bounds.*/
auto cJSON_CreateIntArray(const int *numbers, int count) -> CJSON_PUBLIC(cJSON *);

auto cJSON_CreateFloatArray(const float *numbers, int count) -> CJSON_PUBLIC(cJSON *);

auto cJSON_CreateDoubleArray(const double *numbers, int count) -> CJSON_PUBLIC(cJSON *);

auto cJSON_CreateStringArray(const char *const *strings, int count) -> CJSON_PUBLIC(cJSON *);

/* Append item to the specified array/object. */
auto cJSON_AddItemToArray(cJSON *array, cJSON *item) -> CJSON_PUBLIC(cJSON_bool);

auto cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item) -> CJSON_PUBLIC(cJSON_bool);
/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the cJSON object.
 * WARNING: When this function was used, make sure to always check that (item->type & cJSON_StringIsConst) is zero before
 * writing to `item->string` */
auto cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item) -> CJSON_PUBLIC(cJSON_bool);
/* Append reference to item to the specified array/object. Use this when you want to add an existing cJSON to a new cJSON, but don't want to corrupt your existing cJSON. */
auto cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item) -> CJSON_PUBLIC(cJSON_bool);

auto cJSON_AddItemReferenceToObject(cJSON *object, const char *string, cJSON *item) -> CJSON_PUBLIC(cJSON_bool);

/* Remove/Detach items from Arrays/Objects. */
auto cJSON_DetachItemViaPointer(cJSON *parent, cJSON *item) -> CJSON_PUBLIC(cJSON *);

auto cJSON_DetachItemFromArray(cJSON *array, int which) -> CJSON_PUBLIC(cJSON *);

CJSON_PUBLIC(void)
cJSON_DeleteItemFromArray(cJSON *array, int which);

auto cJSON_DetachItemFromObject(cJSON *object, const char *string) -> CJSON_PUBLIC(cJSON *);

auto cJSON_DetachItemFromObjectCaseSensitive(cJSON *object, const char *string) -> CJSON_PUBLIC(cJSON *);

CJSON_PUBLIC(void)
cJSON_DeleteItemFromObject(cJSON *object, const char *string);

CJSON_PUBLIC(void)
cJSON_DeleteItemFromObjectCaseSensitive(cJSON *object, const char *string);

/* Update array items. */
auto cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem) -> CJSON_PUBLIC(cJSON_bool); /* Shifts pre-existing items to the right. */
auto cJSON_ReplaceItemViaPointer(cJSON *parent, cJSON *item, cJSON *replacement) -> CJSON_PUBLIC(cJSON_bool);

auto cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem) -> CJSON_PUBLIC(cJSON_bool);

auto cJSON_ReplaceItemInObject(cJSON *object, const char *string, cJSON *newitem) -> CJSON_PUBLIC(cJSON_bool);

auto cJSON_ReplaceItemInObjectCaseSensitive(cJSON *object, const char *string, cJSON *newitem) -> CJSON_PUBLIC(cJSON_bool);

/* Duplicate a cJSON item */
auto cJSON_Duplicate(const cJSON *item, cJSON_bool recurse) -> CJSON_PUBLIC(cJSON *);
/* Duplicate will create a new, identical cJSON item to the one you pass, in new memory that will
 * need to be released. With recurse!=0, it will duplicate any children connected to the item.
 * The item->next and ->prev pointers are always zero on return from Duplicate. */
/* Recursively compare two cJSON items for equality. If either a or b is NULL or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or case insensitive (0) */
auto cJSON_Compare(const cJSON *a, const cJSON *b, cJSON_bool case_sensitive) -> CJSON_PUBLIC(cJSON_bool);

/* Minify a strings, remove blank characters(such as ' ', '\t', '\r', '\n') from strings.
 * The input pointer json cannot point to a read-only address area, such as a string constant, 
 * but should point to a readable and writable address area. */
CJSON_PUBLIC(void)
cJSON_Minify(char *json);

/* Helper functions for creating and adding items to an object at the same time.
 * They return the added item or NULL on failure. */
auto cJSON_AddNullToObject(cJSON *object, const char *name) -> CJSON_PUBLIC(cJSON *);

auto cJSON_AddTrueToObject(cJSON *object, const char *name) -> CJSON_PUBLIC(cJSON *);

auto cJSON_AddFalseToObject(cJSON *object, const char *name) -> CJSON_PUBLIC(cJSON *);

auto cJSON_AddBoolToObject(cJSON *object, const char *name, cJSON_bool boolean) -> CJSON_PUBLIC(cJSON *);

auto cJSON_AddNumberToObject(cJSON *object, const char *name, double number) -> CJSON_PUBLIC(cJSON *);

auto cJSON_AddStringToObject(cJSON *object, const char *name, const char *string) -> CJSON_PUBLIC(cJSON *);

auto cJSON_AddRawToObject(cJSON *object, const char *name, const char *raw) -> CJSON_PUBLIC(cJSON *);

auto cJSON_AddObjectToObject(cJSON *object, const char *name) -> CJSON_PUBLIC(cJSON *);

auto cJSON_AddArrayToObject(cJSON *object, const char *name) -> CJSON_PUBLIC(cJSON *);

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define cJSON_SetIntValue(object, number) ((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
/* helper for the cJSON_SetNumberValue macro */
auto cJSON_SetNumberHelper(cJSON *object, double number) -> CJSON_PUBLIC(double);

#define cJSON_SetNumberValue(object, number) ((object != NULL) ? cJSON_SetNumberHelper(object, (double) number) : (number))
/* Change the valuestring of a cJSON_String object, only takes effect when type of object is cJSON_String */
auto cJSON_SetValuestring(cJSON *object, const char *valuestring) -> CJSON_PUBLIC(char *);

/* Macro for iterating over an array or object */
#define cJSON_ArrayForEach(element, array) for (element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

/* malloc/free objects using the malloc/free functions that have been set with cJSON_InitHooks */
auto cJSON_malloc(size_t size) -> CJSON_PUBLIC(void *);

CJSON_PUBLIC(void)
cJSON_free(void *object);

#ifdef __cplusplus
}
#endif

#endif
