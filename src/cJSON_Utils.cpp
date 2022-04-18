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

/* disable warnings about old C89 functions in MSVC */
#if !defined(_CRT_SECURE_NO_DEPRECATE) && defined(_MSC_VER)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#ifdef __GNUCC__
#pragma GCC visibility push(default)
#endif
#if defined(_MSC_VER)
#pragma warning(push)
/* disable warning about single line comments in system headers */
#pragma warning(disable : 4001)
#endif

#include <cctype>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#ifdef __GNUCC__
#pragma GCC visibility pop
#endif

#include "cJSON_Utils.h"

/* define our own boolean type */
#ifdef true
#undef true
#endif
#define true ((cJSON_bool) 1)

#ifdef false
#undef false
#endif
#define false ((cJSON_bool) 0)

static auto cJSONUtils_strdup(const unsigned char *const string) -> unsigned char * {
    size_t length = 0;
    unsigned char *copy = nullptr;

    length = strlen((const char *) string) + sizeof("");
    copy = (unsigned char *) cJSON_malloc(length);
    if (copy == nullptr) {
        return nullptr;
    }
    memcpy(copy, string, length);

    return copy;
}

/* string comparison which doesn't consider NULL pointers equal */
static auto
compare_strings(const unsigned char *string1, const unsigned char *string2, const cJSON_bool case_sensitive) -> int {
    if ((string1 == nullptr) || (string2 == nullptr)) {
        return 1;
    }

    if (string1 == string2) {
        return 0;
    }

    if (case_sensitive != 0) {
        return strcmp((const char *) string1, (const char *) string2);
    }

    for (; tolower(*string1) == tolower(*string2); (void) string1++, string2++) {
        if (*string1 == '\0') {
            return 0;
        }
    }

    return tolower(*string1) - tolower(*string2);
}

/* securely comparison of floating-point variables */
static auto compare_double(double a, double b) -> cJSON_bool {
    double maxVal = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return static_cast<cJSON_bool>(fabs(a - b) <= maxVal * DBL_EPSILON);
}


/* Compare the next path element of two JSON pointers, two NULL pointers are considered unequal: */
static auto
compare_pointers(const unsigned char *name, const unsigned char *pointer, const cJSON_bool case_sensitive) -> cJSON_bool {
    if ((name == nullptr) || (pointer == nullptr)) {
        return false;
    }

    for (; (*name != '\0') && (*pointer != '\0') &&
           (*pointer != '/');
         (void) name++, pointer++) /* compare until next '/' */
    {
        if (*pointer == '~') {
            /* check for escaped '~' (~0) and '/' (~1) */
            if (((pointer[1] != '0') || (*name != '~')) && ((pointer[1] != '1') || (*name != '/'))) {
                /* invalid escape sequence or wrong character in *name */
                return false;
            }
            pointer++;

        } else if (((case_sensitive == 0) && (tolower(*name) != tolower(*pointer))) ||
                   ((case_sensitive != 0) && (*name != *pointer))) {
            return false;
        }
    }
    if (((*pointer != 0) && (*pointer != '/')) != (*name != 0)) {
        /* one string has ended, the other not */
        return false;
        ;
    }

    return true;
}

/* calculate the length of a string if encoded as JSON pointer with ~0 and ~1 escape sequences */
static auto pointer_encoded_length(const unsigned char *string) -> size_t {
    size_t length;
    for (length = 0; *string != '\0'; (void) string++, length++) {
        /* character needs to be escaped? */
        if ((*string == '~') || (*string == '/')) {
            length++;
        }
    }

    return length;
}

/* copy a string while escaping '~' and '/' with ~0 and ~1 JSON pointer escape codes */
static void encode_string_as_pointer(unsigned char *destination, const unsigned char *source) {
    for (; source[0] != '\0'; (void) source++, destination++) {
        if (source[0] == '/') {
            destination[0] = '~';
            destination[1] = '1';
            destination++;
        } else if (source[0] == '~') {
            destination[0] = '~';
            destination[1] = '0';
            destination++;
        } else {
            destination[0] = source[0];
        }
    }

    destination[0] = '\0';
}

auto cJSONUtils_FindPointerFromObjectTo(const cJSON *const object, const cJSON *const target) -> CJSON_PUBLIC(char *) {
    size_t child_index = 0;
    cJSON *current_child = nullptr;

    if ((object == nullptr) || (target == nullptr)) {
        return nullptr;
    }

    if (object == target) {
        /* found */
        return (char *) cJSONUtils_strdup((const unsigned char *) "");
    }

    /* recursively search all children of the object or array */
    for (current_child = object->child;
         current_child != nullptr; (void) (current_child = current_child->next), child_index++) {
        auto *target_pointer = (unsigned char *) cJSONUtils_FindPointerFromObjectTo(current_child, target);
        /* found the target? */
        if (target_pointer != nullptr) {
            if (cJSON_IsArray(object) != 0) {
                /* reserve enough memory for a 64 bit integer + '/' and '\0' */
                auto *full_pointer = (unsigned char *) cJSON_malloc(
                        strlen((char *) target_pointer) + 20 + sizeof("/"));
                /* check if conversion to unsigned long is valid
                 * This should be eliminated at compile time by dead code elimination
                 * if size_t is an alias of unsigned long, or if it is bigger */
                if (child_index > ULONG_MAX) {
                    cJSON_free(target_pointer);
                    cJSON_free(full_pointer);
                    return nullptr;
                }
                sprintf((char *) full_pointer, "/%lu%s", (unsigned long) child_index,
                        target_pointer); /* /<array_index><path> */
                cJSON_free(target_pointer);

                return (char *) full_pointer;
            }

            if (cJSON_IsObject(object) != 0) {
                auto *full_pointer = (unsigned char *) cJSON_malloc(strlen((char *) target_pointer) +
                                                                    pointer_encoded_length(
                                                                            (unsigned char *) current_child->string) +
                                                                    2);
                full_pointer[0] = '/';
                encode_string_as_pointer(full_pointer + 1, (unsigned char *) current_child->string);
                strcat((char *) full_pointer, (char *) target_pointer);
                cJSON_free(target_pointer);

                return (char *) full_pointer;
            }

            /* reached leaf of the tree, found nothing */
            cJSON_free(target_pointer);
            return nullptr;
        }
    }

    /* not found */
    return nullptr;
}

/* non broken version of cJSON_GetArrayItem */
static auto get_array_item(const cJSON *array, size_t item) -> cJSON * {
    cJSON *child = array != nullptr ? array->child : nullptr;
    while ((child != nullptr) && (item > 0)) {
        item--;
        child = child->next;
    }

    return child;
}

static auto decode_array_index_from_pointer(const unsigned char *const pointer, size_t *const index) -> cJSON_bool {
    size_t parsed_index = 0;
    size_t position = 0;

    if ((pointer[0] == '0') && ((pointer[1] != '\0') && (pointer[1] != '/'))) {
        /* leading zeroes are not permitted */
        return 0;
    }

    for (position = 0; (pointer[position] >= '0') && (pointer[0] <= '9'); position++) {
        parsed_index = (10 * parsed_index) + (size_t) (pointer[position] - '0');
    }

    if ((pointer[position] != '\0') && (pointer[position] != '/')) {
        return 0;
    }

    *index = parsed_index;

    return 1;
}

static auto get_item_from_pointer(cJSON *const object, const char *pointer, const cJSON_bool case_sensitive) -> cJSON * {
    cJSON *current_element = object;

    if (pointer == nullptr) {
        return nullptr;
    }

    /* follow path of the pointer */
    while ((pointer[0] == '/') && (current_element != nullptr)) {
        pointer++;
        if (cJSON_IsArray(current_element) != 0) {
            size_t index = 0;
            if (decode_array_index_from_pointer((const unsigned char *) pointer, &index) == 0) {
                return nullptr;
            }

            current_element = get_array_item(current_element, index);
        } else if (cJSON_IsObject(current_element) != 0) {
            current_element = current_element->child;
            /* GetObjectItem. */
            while ((current_element != nullptr) &&
                   (compare_pointers((unsigned char *) current_element->string, (const unsigned char *) pointer,
                                     case_sensitive) == 0)) {
                current_element = current_element->next;
            }
        } else {
            return nullptr;
        }

        /* skip to the next path token or end of string */
        while ((pointer[0] != '\0') && (pointer[0] != '/')) {
            pointer++;
        }
    }

    return current_element;
}

auto cJSONUtils_GetPointer(cJSON *const object, const char *pointer) -> CJSON_PUBLIC(cJSON *) {
    return get_item_from_pointer(object, pointer, false);
}

auto cJSONUtils_GetPointerCaseSensitive(cJSON *const object, const char *pointer) -> CJSON_PUBLIC(cJSON *) {
    return get_item_from_pointer(object, pointer, true);
}

/* JSON Patch implementation. */
static void decode_pointer_inplace(unsigned char *string) {
    unsigned char *decoded_string = string;

    if (string == nullptr) {
        return;
    }

    for (; *string != 0u; (void) decoded_string++, string++) {
        if (string[0] == '~') {
            if (string[1] == '0') {
                decoded_string[0] = '~';
            } else if (string[1] == '1') {
                decoded_string[1] = '/';
            } else {
                /* invalid escape sequence */
                return;
            }

            string++;
        }
    }

    decoded_string[0] = '\0';
}

/* non-broken cJSON_DetachItemFromArray */
static auto detach_item_from_array(cJSON *array, size_t which) -> cJSON * {
    cJSON *c = array->child;
    while ((c != nullptr) && (which > 0)) {
        c = c->next;
        which--;
    }
    if (c == nullptr) {
        /* item doesn't exist */
        return nullptr;
    }
    if (c != array->child) {
        /* not the first element */
        c->prev->next = c->next;
    }
    if (c->next != nullptr) {
        c->next->prev = c->prev;
    }
    if (c == array->child) {
        array->child = c->next;
    } else if (c->next == nullptr) {
        array->child->prev = c->prev;
    }
    /* make sure the detached item doesn't point anywhere anymore */
    c->prev = c->next = nullptr;

    return c;
}

/* detach an item at the given path */
static auto detach_path(cJSON *object, const unsigned char *path, const cJSON_bool case_sensitive) -> cJSON * {
    unsigned char *parent_pointer = nullptr;
    unsigned char *child_pointer = nullptr;
    cJSON *parent = nullptr;
    cJSON *detached_item = nullptr;

    /* copy path and split it in parent and child */
    parent_pointer = cJSONUtils_strdup(path);
    if (parent_pointer == nullptr) {
        goto cleanup;
    }

    child_pointer = (unsigned char *) strrchr((char *) parent_pointer, '/'); /* last '/' */
    if (child_pointer == nullptr) {
        goto cleanup;
    }
    /* split strings */
    child_pointer[0] = '\0';
    child_pointer++;

    parent = get_item_from_pointer(object, (char *) parent_pointer, case_sensitive);
    decode_pointer_inplace(child_pointer);

    if (cJSON_IsArray(parent) != 0) {
        size_t index = 0;
        if (decode_array_index_from_pointer(child_pointer, &index) == 0) {
            goto cleanup;
        }
        detached_item = detach_item_from_array(parent, index);
    } else if (cJSON_IsObject(parent) != 0) {
        detached_item = cJSON_DetachItemFromObject(parent, (char *) child_pointer);
    } else {
        /* Couldn't find object to remove child from. */
        goto cleanup;
    }

cleanup:
    if (parent_pointer != nullptr) {
        cJSON_free(parent_pointer);
    }

    return detached_item;
}

/* sort lists using mergesort */
static auto sort_list(cJSON *list, const cJSON_bool case_sensitive) -> cJSON * {
    cJSON *first = list;
    cJSON *second = list;
    cJSON *current_item = list;
    cJSON *result = list;
    cJSON *result_tail = nullptr;

    if ((list == nullptr) || (list->next == nullptr)) {
        /* One entry is sorted already. */
        return result;
    }

    while ((current_item != nullptr) && (current_item->next != nullptr) &&
           (compare_strings((unsigned char *) current_item->string, (unsigned char *) current_item->next->string,
                            case_sensitive) < 0)) {
        /* Test for list sorted. */
        current_item = current_item->next;
    }
    if ((current_item == nullptr) || (current_item->next == nullptr)) {
        /* Leave sorted lists unmodified. */
        return result;
    }

    /* reset pointer to the beginning */
    current_item = list;
    while (current_item != nullptr) {
        /* Walk two pointers to find the middle. */
        second = second->next;
        current_item = current_item->next;
        /* advances current_item two steps at a time */
        if (current_item != nullptr) {
            current_item = current_item->next;
        }
    }
    if ((second != nullptr) && (second->prev != nullptr)) {
        /* Split the lists */
        second->prev->next = nullptr;
        second->prev = nullptr;
    }

    /* Recursively sort the sub-lists. */
    first = sort_list(first, case_sensitive);
    second = sort_list(second, case_sensitive);
    result = nullptr;

    /* Merge the sub-lists */
    while ((first != nullptr) && (second != nullptr)) {
        cJSON *smaller = nullptr;
        if (compare_strings((unsigned char *) first->string, (unsigned char *) second->string, case_sensitive) < 0) {
            smaller = first;
        } else {
            smaller = second;
        }

        if (result == nullptr) {
            /* start merged list with the smaller element */
            result_tail = smaller;
            result = smaller;
        } else {
            /* add smaller element to the list */
            result_tail->next = smaller;
            smaller->prev = result_tail;
            result_tail = smaller;
        }

        if (first == smaller) {
            first = first->next;
        } else {
            second = second->next;
        }
    }

    if (first != nullptr) {
        /* Append rest of first list. */
        if (result == nullptr) {
            return first;
        }
        result_tail->next = first;
        first->prev = result_tail;
    }
    if (second != nullptr) {
        /* Append rest of second list */
        if (result == nullptr) {
            return second;
        }
        result_tail->next = second;
        second->prev = result_tail;
    }

    return result;
}

static void sort_object(cJSON *const object, const cJSON_bool case_sensitive) {
    if (object == nullptr) {
        return;
    }
    object->child = sort_list(object->child, case_sensitive);
}

static auto compare_json(cJSON *a, cJSON *b, const cJSON_bool case_sensitive) -> cJSON_bool {
    if ((a == nullptr) || (b == nullptr) || ((a->type & 0xFF) != (b->type & 0xFF))) {
        /* mismatched type. */
        return false;
    }
    switch (a->type & 0xFF) {
        case cJSON_Number:
            /* numeric mismatch. */
            if ((a->valueint != b->valueint) || (compare_double(a->valuedouble, b->valuedouble) == 0)) {
                return false;
            } else {
                return true;
            }

        case cJSON_String:
            /* string mismatch. */
            if (strcmp(a->valuestring, b->valuestring) != 0) {
                return false;
            } else {
                return true;
            }

        case cJSON_Array:
            for ((void) (a = a->child), b = b->child; (a != nullptr) && (b != nullptr); (void) (a = a->next), b = b->next) {
                cJSON_bool identical = compare_json(a, b, case_sensitive);
                if (identical == 0) {
                    return false;
                }
            }

            /* array size mismatch? (one of both children is not NULL) */
            if ((a != nullptr) || (b != nullptr)) {
                return false;
            }
            return true;


        case cJSON_Object:
            sort_object(a, case_sensitive);
            sort_object(b, case_sensitive);
            for ((void) (a = a->child), b = b->child; (a != nullptr) && (b != nullptr); (void) (a = a->next), b = b->next) {
                cJSON_bool identical = false;
                /* compare object keys */
                if (compare_strings((unsigned char *) a->string, (unsigned char *) b->string, case_sensitive) != 0) {
                    /* missing member */
                    return false;
                }
                identical = compare_json(a, b, case_sensitive);
                if (identical == 0) {
                    return false;
                }
            }

            /* object length mismatch (one of both children is not null) */
            if ((a != nullptr) || (b != nullptr)) {
                return false;
            }
            return true;


        default:
            break;
    }

    /* null, true or false */
    return true;
}

/* non broken version of cJSON_InsertItemInArray */
static auto insert_item_in_array(cJSON *array, size_t which, cJSON *newitem) -> cJSON_bool {
    cJSON *child = array->child;
    while ((child != nullptr) && (which > 0)) {
        child = child->next;
        which--;
    }
    if (which > 0) {
        /* item is after the end of the array */
        return 0;
    }
    if (child == nullptr) {
        cJSON_AddItemToArray(array, newitem);
        return 1;
    }

    /* insert into the linked list */
    newitem->next = child;
    newitem->prev = child->prev;
    child->prev = newitem;

    /* was it at the beginning */
    if (child == array->child) {
        array->child = newitem;
    } else {
        newitem->prev->next = newitem;
    }

    return 1;
}

static auto get_object_item(const cJSON *const object, const char *name, const cJSON_bool case_sensitive) -> cJSON * {
    if (case_sensitive != 0) {
        return cJSON_GetObjectItemCaseSensitive(object, name);
    }

    return cJSON_GetObjectItem(object, name);
}

enum patch_operation {
    INVALID,
    ADD,
    REMOVE,
    REPLACE,
    MOVE,
    COPY,
    TEST
};

static auto decode_patch_operation(const cJSON *const patch, const cJSON_bool case_sensitive) -> enum patch_operation {
    cJSON *operation = get_object_item(patch, "op", case_sensitive);
    if (cJSON_IsString(operation) == 0){
            return INVALID;}

if (strcmp(operation->valuestring, "add") == 0) {
    return ADD;
}

if (strcmp(operation->valuestring, "remove") == 0) {
    return REMOVE;
}

if (strcmp(operation->valuestring, "replace") == 0) {
    return REPLACE;
}

if (strcmp(operation->valuestring, "move") == 0) {
    return MOVE;
}

if (strcmp(operation->valuestring, "copy") == 0) {
    return COPY;
}

if (strcmp(operation->valuestring, "test") == 0) {
    return TEST;
}

return INVALID;
}

/* overwrite and existing item with another one and free resources on the way */
static void overwrite_item(cJSON *const root, const cJSON replacement) {
    if (root == nullptr) {
        return;
    }

    if (root->string != nullptr) {
        cJSON_free(root->string);
    }
    if (root->valuestring != nullptr) {
        cJSON_free(root->valuestring);
    }
    if (root->child != nullptr) {
        cJSON_Delete(root->child);
    }

    memcpy(root, &replacement, sizeof(cJSON));
}

static auto apply_patch(cJSON *object, const cJSON *patch, const cJSON_bool case_sensitive) -> int {
    cJSON *path = nullptr;
    cJSON *value = nullptr;
    cJSON *parent = nullptr;
    enum patch_operation opcode = INVALID;
    unsigned char *parent_pointer = nullptr;
    unsigned char *child_pointer = nullptr;
    int status = 0;

    path = get_object_item(patch, "path", case_sensitive);
    if (cJSON_IsString(path) == 0) {
        /* malformed patch. */
        status = 2;
        goto cleanup;
    }

    opcode = decode_patch_operation(patch, case_sensitive);
    if (opcode == INVALID) {
        status = 3;
        goto cleanup;
    } else if (opcode == TEST) {
        /* compare value: {...} with the given path */
        status = static_cast<int>(compare_json(get_item_from_pointer(object, path->valuestring, case_sensitive),
                                               get_object_item(patch, "value", case_sensitive), case_sensitive)) == 0;
        goto cleanup;
    }

    /* special case for replacing the root */
    if (path->valuestring[0] == '\0') {
        if (opcode == REMOVE) {
            static const cJSON invalid = {nullptr, nullptr, nullptr, cJSON_Invalid, nullptr, 0, 0, nullptr};

            overwrite_item(object, invalid);

            status = 0;
            goto cleanup;
        }

        if ((opcode == REPLACE) || (opcode == ADD)) {
            value = get_object_item(patch, "value", case_sensitive);
            if (value == nullptr) {
                /* missing "value" for add/replace. */
                status = 7;
                goto cleanup;
            }

            value = cJSON_Duplicate(value, 1);
            if (value == nullptr) {
                /* out of memory for add/replace. */
                status = 8;
                goto cleanup;
            }

            overwrite_item(object, *value);

            /* delete the duplicated value */
            cJSON_free(value);
            value = nullptr;

            /* the string "value" isn't needed */
            if (object->string != nullptr) {
                cJSON_free(object->string);
                object->string = nullptr;
            }

            status = 0;
            goto cleanup;
        }
    }

    if ((opcode == REMOVE) || (opcode == REPLACE)) {
        /* Get rid of old. */
        cJSON *old_item = detach_path(object, (unsigned char *) path->valuestring, case_sensitive);
        if (old_item == nullptr) {
            status = 13;
            goto cleanup;
        }
        cJSON_Delete(old_item);
        if (opcode == REMOVE) {
            /* For Remove, this job is done. */
            status = 0;
            goto cleanup;
        }
    }

    /* Copy/Move uses "from". */
    if ((opcode == MOVE) || (opcode == COPY)) {
        cJSON *from = get_object_item(patch, "from", case_sensitive);
        if (from == nullptr) {
            /* missing "from" for copy/move. */
            status = 4;
            goto cleanup;
        }

        if (opcode == MOVE) {
            value = detach_path(object, (unsigned char *) from->valuestring, case_sensitive);
        }
        if (opcode == COPY) {
            value = get_item_from_pointer(object, from->valuestring, case_sensitive);
        }
        if (value == nullptr) {
            /* missing "from" for copy/move. */
            status = 5;
            goto cleanup;
        }
        if (opcode == COPY) {
            value = cJSON_Duplicate(value, 1);
        }
        if (value == nullptr) {
            /* out of memory for copy/move. */
            status = 6;
            goto cleanup;
        }
    } else /* Add/Replace uses "value". */
    {
        value = get_object_item(patch, "value", case_sensitive);
        if (value == nullptr) {
            /* missing "value" for add/replace. */
            status = 7;
            goto cleanup;
        }
        value = cJSON_Duplicate(value, 1);
        if (value == nullptr) {
            /* out of memory for add/replace. */
            status = 8;
            goto cleanup;
        }
    }

    /* Now, just add "value" to "path". */

    /* split pointer in parent and child */
    parent_pointer = cJSONUtils_strdup((unsigned char *) path->valuestring);
    if (parent_pointer != nullptr) {
        child_pointer = (unsigned char *) strrchr((char *) parent_pointer, '/');
    }
    if (child_pointer != nullptr) {
        child_pointer[0] = '\0';
        child_pointer++;
    }
    parent = get_item_from_pointer(object, (char *) parent_pointer, case_sensitive);
    decode_pointer_inplace(child_pointer);

    /* add, remove, replace, move, copy, test. */
    if ((parent == nullptr) || (child_pointer == nullptr)) {
        /* Couldn't find object to add to. */
        status = 9;
        goto cleanup;
    } else if (cJSON_IsArray(parent) != 0) {
        if (strcmp((char *) child_pointer, "-") == 0) {
            cJSON_AddItemToArray(parent, value);
            value = nullptr;
        } else {
            size_t index = 0;
            if (decode_array_index_from_pointer(child_pointer, &index) == 0) {
                status = 11;
                goto cleanup;
            }

            if (insert_item_in_array(parent, index, value) == 0) {
                status = 10;
                goto cleanup;
            }
            value = nullptr;
        }
    } else if (cJSON_IsObject(parent) != 0) {
        if (case_sensitive != 0) {
            cJSON_DeleteItemFromObjectCaseSensitive(parent, (char *) child_pointer);
        } else {
            cJSON_DeleteItemFromObject(parent, (char *) child_pointer);
        }
        cJSON_AddItemToObject(parent, (char *) child_pointer, value);
        value = nullptr;
    } else /* parent is not an object */
    {
        /* Couldn't find object to add to. */
        status = 9;
        goto cleanup;
    }

cleanup:
    if (value != nullptr) {
        cJSON_Delete(value);
    }
    if (parent_pointer != nullptr) {
        cJSON_free(parent_pointer);
    }

    return status;
}

auto cJSONUtils_ApplyPatches(cJSON *const object, const cJSON *const patches) -> CJSON_PUBLIC(int) {
    const cJSON *current_patch = nullptr;
    int status = 0;

    if (cJSON_IsArray(patches) == 0) {
        /* malformed patches. */
        return 1;
    }

    if (patches != nullptr) {
        current_patch = patches->child;
    }

    while (current_patch != nullptr) {
        status = apply_patch(object, current_patch, false);
        if (status != 0) {
            return status;
        }
        current_patch = current_patch->next;
    }

    return 0;
}

auto cJSONUtils_ApplyPatchesCaseSensitive(cJSON *const object, const cJSON *const patches) -> CJSON_PUBLIC(int) {
    const cJSON *current_patch = nullptr;
    int status = 0;

    if (cJSON_IsArray(patches) == 0) {
        /* malformed patches. */
        return 1;
    }

    if (patches != nullptr) {
        current_patch = patches->child;
    }

    while (current_patch != nullptr) {
        status = apply_patch(object, current_patch, true);
        if (status != 0) {
            return status;
        }
        current_patch = current_patch->next;
    }

    return 0;
}

static void compose_patch(cJSON *const patches, const unsigned char *const operation, const unsigned char *const path,
                          const unsigned char *suffix, const cJSON *const value) {
    cJSON *patch = nullptr;

    if ((patches == nullptr) || (operation == nullptr) || (path == nullptr)) {
        return;
    }

    patch = cJSON_CreateObject();
    if (patch == nullptr) {
        return;
    }
    cJSON_AddItemToObject(patch, "op", cJSON_CreateString((const char *) operation));

    if (suffix == nullptr) {
        cJSON_AddItemToObject(patch, "path", cJSON_CreateString((const char *) path));
    } else {
        size_t suffix_length = pointer_encoded_length(suffix);
        size_t path_length = strlen((const char *) path);
        auto *full_path = (unsigned char *) cJSON_malloc(path_length + suffix_length + sizeof("/"));

        sprintf((char *) full_path, "%s/", (const char *) path);
        encode_string_as_pointer(full_path + path_length + 1, suffix);

        cJSON_AddItemToObject(patch, "path", cJSON_CreateString((const char *) full_path));
        cJSON_free(full_path);
    }

    if (value != nullptr) {
        cJSON_AddItemToObject(patch, "value", cJSON_Duplicate(value, 1));
    }
    cJSON_AddItemToArray(patches, patch);
}

CJSON_PUBLIC(void)
cJSONUtils_AddPatchToArray(cJSON *const array, const char *const operation, const char *const path,
                           const cJSON *const value) {
    compose_patch(array, (const unsigned char *) operation, (const unsigned char *) path, nullptr, value);
}

static void create_patches(cJSON *const patches, const unsigned char *const path, cJSON *const from, cJSON *const to,
                           const cJSON_bool case_sensitive) {
    if ((from == nullptr) || (to == nullptr)) {
        return;
    }

    if ((from->type & 0xFF) != (to->type & 0xFF)) {
        compose_patch(patches, (const unsigned char *) "replace", path, nullptr, to);
        return;
    }

    switch (from->type & 0xFF) {
        case cJSON_Number:
            if ((from->valueint != to->valueint) || (compare_double(from->valuedouble, to->valuedouble) == 0)) {
                compose_patch(patches, (const unsigned char *) "replace", path, nullptr, to);
            }
            return;

        case cJSON_String:
            if (strcmp(from->valuestring, to->valuestring) != 0) {
                compose_patch(patches, (const unsigned char *) "replace", path, nullptr, to);
            }
            return;

        case cJSON_Array: {
            size_t index = 0;
            cJSON *from_child = from->child;
            cJSON *to_child = to->child;
            auto *new_path = (unsigned char *) cJSON_malloc(
                    strlen((const char *) path) + 20 + sizeof("/")); /* Allow space for 64bit int. log10(2^64) = 20 */

            /* generate patches for all array elements that exist in both "from" and "to" */
            for (index = 0; (from_child != nullptr) && (to_child !=
                                                        nullptr);
                 (void) (from_child = from_child->next), (void) (to_child = to_child->next), index++) {
                /* check if conversion to unsigned long is valid
                 * This should be eliminated at compile time by dead code elimination
                 * if size_t is an alias of unsigned long, or if it is bigger */
                if (index > ULONG_MAX) {
                    cJSON_free(new_path);
                    return;
                }
                sprintf((char *) new_path, "%s/%lu", path,
                        (unsigned long) index); /* path of the current array element */
                create_patches(patches, new_path, from_child, to_child, case_sensitive);
            }

            /* remove leftover elements from 'from' that are not in 'to' */
            for (; (from_child != nullptr); (void) (from_child = from_child->next)) {
                /* check if conversion to unsigned long is valid
                 * This should be eliminated at compile time by dead code elimination
                 * if size_t is an alias of unsigned long, or if it is bigger */
                if (index > ULONG_MAX) {
                    cJSON_free(new_path);
                    return;
                }
                sprintf((char *) new_path, "%lu", (unsigned long) index);
                compose_patch(patches, (const unsigned char *) "remove", path, new_path, nullptr);
            }
            /* add new elements in 'to' that were not in 'from' */
            for (; (to_child != nullptr); (void) (to_child = to_child->next), index++) {
                compose_patch(patches, (const unsigned char *) "add", path, (const unsigned char *) "-", to_child);
            }
            cJSON_free(new_path);
            return;
        }

        case cJSON_Object: {
            cJSON *from_child = nullptr;
            cJSON *to_child = nullptr;
            sort_object(from, case_sensitive);
            sort_object(to, case_sensitive);

            from_child = from->child;
            to_child = to->child;
            /* for all object values in the object with more of them */
            while ((from_child != nullptr) || (to_child != nullptr)) {
                int diff;
                if (from_child == nullptr) {
                    diff = 1;
                } else if (to_child == nullptr) {
                    diff = -1;
                } else {
                    diff = compare_strings((unsigned char *) from_child->string, (unsigned char *) to_child->string,
                                           case_sensitive);
                }

                if (diff == 0) {
                    /* both object keys are the same */
                    size_t path_length = strlen((const char *) path);
                    size_t from_child_name_length = pointer_encoded_length((unsigned char *) from_child->string);
                    auto *new_path = (unsigned char *) cJSON_malloc(
                            path_length + from_child_name_length + sizeof("/"));

                    sprintf((char *) new_path, "%s/", path);
                    encode_string_as_pointer(new_path + path_length + 1, (unsigned char *) from_child->string);

                    /* create a patch for the element */
                    create_patches(patches, new_path, from_child, to_child, case_sensitive);
                    cJSON_free(new_path);

                    from_child = from_child->next;
                    to_child = to_child->next;
                } else if (diff < 0) {
                    /* object element doesn't exist in 'to' --> remove it */
                    compose_patch(patches, (const unsigned char *) "remove", path, (unsigned char *) from_child->string,
                                  nullptr);

                    from_child = from_child->next;
                } else {
                    /* object element doesn't exist in 'from' --> add it */
                    compose_patch(patches, (const unsigned char *) "add", path, (unsigned char *) to_child->string,
                                  to_child);

                    to_child = to_child->next;
                }
            }
            return;
        }

        default:
            break;
    }
}

auto cJSONUtils_GeneratePatches(cJSON *const from, cJSON *const to) -> CJSON_PUBLIC(cJSON *) {
    cJSON *patches = nullptr;

    if ((from == nullptr) || (to == nullptr)) {
        return nullptr;
    }

    patches = cJSON_CreateArray();
    create_patches(patches, (const unsigned char *) "", from, to, false);

    return patches;
}

auto cJSONUtils_GeneratePatchesCaseSensitive(cJSON *const from, cJSON *const to) -> CJSON_PUBLIC(cJSON *) {
    cJSON *patches = nullptr;

    if ((from == nullptr) || (to == nullptr)) {
        return nullptr;
    }

    patches = cJSON_CreateArray();
    create_patches(patches, (const unsigned char *) "", from, to, true);

    return patches;
}

CJSON_PUBLIC(void)
cJSONUtils_SortObject(cJSON *const object) {
    sort_object(object, false);
}

CJSON_PUBLIC(void)
cJSONUtils_SortObjectCaseSensitive(cJSON *const object) {
    sort_object(object, true);
}

static auto merge_patch(cJSON *target, const cJSON *const patch, const cJSON_bool case_sensitive) -> cJSON * {
    cJSON *patch_child = nullptr;

    if (cJSON_IsObject(patch) == 0) {
        /* scalar value, array or NULL, just duplicate */
        cJSON_Delete(target);
        return cJSON_Duplicate(patch, 1);
    }

    if (cJSON_IsObject(target) == 0) {
        cJSON_Delete(target);
        target = cJSON_CreateObject();
    }

    patch_child = patch->child;
    while (patch_child != nullptr) {
        if (cJSON_IsNull(patch_child) != 0) {
            /* NULL is the indicator to remove a value, see RFC7396 */
            if (case_sensitive != 0) {
                cJSON_DeleteItemFromObjectCaseSensitive(target, patch_child->string);
            } else {
                cJSON_DeleteItemFromObject(target, patch_child->string);
            }
        } else {
            cJSON *replace_me = nullptr;
            cJSON *replacement = nullptr;

            if (case_sensitive != 0) {
                replace_me = cJSON_DetachItemFromObjectCaseSensitive(target, patch_child->string);
            } else {
                replace_me = cJSON_DetachItemFromObject(target, patch_child->string);
            }

            replacement = merge_patch(replace_me, patch_child, case_sensitive);
            if (replacement == nullptr) {
                cJSON_Delete(target);
                return nullptr;
            }

            cJSON_AddItemToObject(target, patch_child->string, replacement);
        }
        patch_child = patch_child->next;
    }
    return target;
}

auto cJSONUtils_MergePatch(cJSON *target, const cJSON *const patch) -> CJSON_PUBLIC(cJSON *) {
    return merge_patch(target, patch, false);
}

auto cJSONUtils_MergePatchCaseSensitive(cJSON *target, const cJSON *const patch) -> CJSON_PUBLIC(cJSON *) {
    return merge_patch(target, patch, true);
}

static auto generate_merge_patch(cJSON *const from, cJSON *const to, const cJSON_bool case_sensitive) -> cJSON * {
    cJSON *from_child = nullptr;
    cJSON *to_child = nullptr;
    cJSON *patch = nullptr;
    if (to == nullptr) {
        /* patch to delete everything */
        return cJSON_CreateNull();
    }
    if ((cJSON_IsObject(to) == 0) || (cJSON_IsObject(from) == 0)) {
        return cJSON_Duplicate(to, 1);
    }

    sort_object(from, case_sensitive);
    sort_object(to, case_sensitive);

    from_child = from->child;
    to_child = to->child;
    patch = cJSON_CreateObject();
    if (patch == nullptr) {
        return nullptr;
    }
    while ((from_child != nullptr) || (to_child != nullptr)) {
        int diff;
        if (from_child != nullptr) {
            if (to_child != nullptr) {
                diff = strcmp(from_child->string, to_child->string);
            } else {
                diff = -1;
            }
        } else {
            diff = 1;
        }

        if (diff < 0) {
            /* from has a value that to doesn't have -> remove */
            cJSON_AddItemToObject(patch, from_child->string, cJSON_CreateNull());

            from_child = from_child->next;
        } else if (diff > 0) {
            /* to has a value that from doesn't have -> add to patch */
            cJSON_AddItemToObject(patch, to_child->string, cJSON_Duplicate(to_child, 1));

            to_child = to_child->next;
        } else {
            /* object key exists in both objects */
            if (compare_json(from_child, to_child, case_sensitive) == 0) {
                /* not identical --> generate a patch */
                cJSON_AddItemToObject(patch, to_child->string, cJSONUtils_GenerateMergePatch(from_child, to_child));
            }

            /* next key in the object */
            from_child = from_child->next;
            to_child = to_child->next;
        }
    }
    if (patch->child == nullptr) {
        /* no patch generated */
        cJSON_Delete(patch);
        return nullptr;
    }

    return patch;
}

auto cJSONUtils_GenerateMergePatch(cJSON *const from, cJSON *const to) -> CJSON_PUBLIC(cJSON *) {
    return generate_merge_patch(from, to, false);
}

auto cJSONUtils_GenerateMergePatchCaseSensitive(cJSON *const from, cJSON *const to) -> CJSON_PUBLIC(cJSON *) {
    return generate_merge_patch(from, to, true);
}
