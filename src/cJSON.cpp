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

/* cJSON */
/* JSON parser in C. */

/* disable warnings about old C89 functions in MSVC */
#if !defined(_CRT_SECURE_NO_DEPRECATE) && defined(_MSC_VER)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#ifdef __GNUC__
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

#ifdef ENABLE_LOCALES
#include <locale.h>
#endif

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#ifdef __GNUC__
#pragma GCC visibility pop
#endif

#include "cJSON.h"

/* define our own boolean type */
#ifdef true
#undef true
#endif
#define true ((cJSON_bool) 1)

#ifdef false
#undef false
#endif
#define false ((cJSON_bool) 0)

/* define isnan and isinf for ANSI C, if in C99 or above, isnan and isinf has been defined in math.h */
#ifndef isinf
#define isinf(d) (isnan((d - d)) && !isnan(d))
#endif
#ifndef isnan
#define isnan(d) (d != d)
#endif

#ifndef NAN
#ifdef _WIN32
#define NAN sqrt(-1.0)
#else
#define NAN 0.0 / 0.0
#endif
#endif

using error = struct {
    const unsigned char *json;
    size_t position;
};
static error global_error = {nullptr, 0};

CJSON_PUBLIC(const char *)
cJSON_GetErrorPtr(void) {
    return (const char *) (global_error.json + global_error.position);
}

auto
cJSON_GetStringValue(const cJSON *const item) -> CJSON_PUBLIC(char *) {
    if (!cJSON_IsString(item)) {
        return nullptr;
    }

    return item->valuestring;
}

auto
cJSON_GetNumberValue(const cJSON *const item) -> CJSON_PUBLIC(double) {
    if (!cJSON_IsNumber(item)) {
        return (double) NAN;
    }

    return item->valuedouble;
}

/* This is a safeguard to prevent copy-pasters from using incompatible C and header files */
#if (CJSON_VERSION_MAJOR != 1) || (CJSON_VERSION_MINOR != 7) || (CJSON_VERSION_PATCH != 15)
#error cJSON.h and cJSON.c have different versions. Make sure that both have the same.
#endif

CJSON_PUBLIC(const char *)
cJSON_Version(void) {
    static char version[15];
    sprintf(version, "%i.%i.%i", CJSON_VERSION_MAJOR, CJSON_VERSION_MINOR, CJSON_VERSION_PATCH);

    return version;
}

/* Case insensitive string comparison, doesn't consider two NULL pointers equal though */
static auto case_insensitive_strcmp(const unsigned char *string1, const unsigned char *string2) -> int {
    if ((string1 == nullptr) || (string2 == nullptr)) {
        return 1;
    }

    if (string1 == string2) {
        return 0;
    }

    for (; tolower(*string1) == tolower(*string2); (void) string1++, string2++) {
        if (*string1 == '\0') {
            return 0;
        }
    }

    return tolower(*string1) - tolower(*string2);
}

using internal_hooks = struct internal_hooks {
    void *(CJSON_CDECL *allocate)(size_t size);

    void (CJSON_CDECL *deallocate)(void *pointer);

    void *(CJSON_CDECL *reallocate)(void *pointer, size_t size);
};

#if defined(_MSC_VER)
/* work around MSVC error C2322: '...' address of dllimport '...' is not static */
static void *CJSON_CDECL internal_malloc(size_t size) {
    return malloc(size);
}
static void CJSON_CDECL internal_free(void *pointer) {
    free(pointer);
}
static void *CJSON_CDECL internal_realloc(void *pointer, size_t size) {
    return realloc(pointer, size);
}
#else
#define internal_malloc  malloc
#define internal_free    free
#define internal_realloc realloc
#endif

/* strlen of character literals resolved at compile time */
#define static_strlen(string_literal) (sizeof(string_literal) - sizeof(""))

static internal_hooks global_hooks = {internal_malloc, internal_free, internal_realloc};

static auto cJSON_strdup(const unsigned char *string, const internal_hooks *const hooks) -> unsigned char * {
    size_t length = 0;
    unsigned char *copy = nullptr;

    if (string == nullptr) {
        return nullptr;
    }

    length = strlen((const char *) string) + sizeof("");
    copy = (unsigned char *) hooks->allocate(length);
    if (copy == nullptr) {
        return nullptr;
    }
    memcpy(copy, string, length);

    return copy;
}

CJSON_PUBLIC(void)
cJSON_InitHooks(cJSON_Hooks *hooks) {
    if (hooks == nullptr) {
        /* Reset hooks */
        global_hooks.allocate = malloc;
        global_hooks.deallocate = free;
        global_hooks.reallocate = realloc;
        return;
    }

    global_hooks.allocate = malloc;
    if (hooks->malloc_fn != nullptr) {
        global_hooks.allocate = hooks->malloc_fn;
    }

    global_hooks.deallocate = free;
    if (hooks->free_fn != nullptr) {
        global_hooks.deallocate = hooks->free_fn;
    }

    /* use realloc only if both free and malloc are used */
    global_hooks.reallocate = nullptr;
    if ((global_hooks.allocate == malloc) && (global_hooks.deallocate == free)) {
        global_hooks.reallocate = realloc;
    }
}

/* Internal constructor. */
static auto cJSON_New_Item(const internal_hooks *const hooks) -> cJSON * {
    auto *node = (cJSON *) hooks->allocate(sizeof(cJSON));
    if (node) {
        memset(node, '\0', sizeof(cJSON));
    }

    return node;
}

/* Delete a cJSON structure. */
CJSON_PUBLIC(void)
cJSON_Delete(cJSON *item) {
    cJSON *next = nullptr;
    while (item != nullptr) {
        next = item->next;
        if (!(item->type & cJSON_IsReference) && (item->child != nullptr)) {
            cJSON_Delete(item->child);
        }
        if (!(item->type & cJSON_IsReference) && (item->valuestring != nullptr)) {
            global_hooks.deallocate(item->valuestring);
        }
        if (!(item->type & cJSON_StringIsConst) && (item->string != nullptr)) {
            global_hooks.deallocate(item->string);
        }
        global_hooks.deallocate(item);
        item = next;
    }
}

/* get the decimal point character of the current locale */
static auto get_decimal_point() -> unsigned char {
#ifdef ENABLE_LOCALES
    struct lconv *lconv = localeconv();
    return (unsigned char) lconv->decimal_point[0];
#else
    return '.';
#endif
}

using parse_buffer = struct {
    const unsigned char *content;
    size_t length;
    size_t offset;
    size_t depth; /* How deeply nested (in arrays/objects) is the input at the current offset. */
    internal_hooks hooks;
};

/* check if the given size is left to read in a given parse buffer (starting with 1) */
#define can_read(buffer, size)                ((buffer != NULL) && (((buffer)->offset + size) <= (buffer)->length))
/* check if the buffer can be accessed at the given index (starting with 0) */
#define can_access_at_index(buffer, index)    ((buffer != NULL) && (((buffer)->offset + index) < (buffer)->length))
#define cannot_access_at_index(buffer, index) (!can_access_at_index(buffer, index))
/* get a pointer to the buffer at the position */
#define buffer_at_offset(buffer)              ((buffer)->content + (buffer)->offset)

/* Parse the input text to generate a number, and populate the result into item. */
static auto parse_number(cJSON *const item, parse_buffer *const input_buffer) -> cJSON_bool {
    double number = 0;
    unsigned char *after_end = nullptr;
    unsigned char number_c_string[64];
    unsigned char decimal_point = get_decimal_point();
    size_t i = 0;

    if ((input_buffer == nullptr) || (input_buffer->content == nullptr)) {
        return false;
    }

    /* copy the number into a temporary buffer and replace '.' with the decimal point
     * of the current locale (for strtod)
     * This also takes care of '\0' not necessarily being available for marking the end of the input */
    for (i = 0; (i < (sizeof(number_c_string) - 1)) && can_access_at_index(input_buffer, i); i++) {
        switch (buffer_at_offset(input_buffer)[i]) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '+':
            case '-':
            case 'e':
            case 'E':
                number_c_string[i] = buffer_at_offset(input_buffer)[i];
                break;

            case '.':
                number_c_string[i] = decimal_point;
                break;

            default:
                goto loop_end;
        }
    }
    loop_end:
    number_c_string[i] = '\0';

    number = strtod((const char *) number_c_string, (char **) &after_end);
    if (number_c_string == after_end) {
        return false; /* parse_error */
    }

    item->valuedouble = number;

    /* use saturation in case of overflow */
    if (number >= INT_MAX) {
        item->valueint = INT_MAX;
    } else if (number <= (double) INT_MIN) {
        item->valueint = INT_MIN;
    } else {
        item->valueint = (int) number;
    }

    item->type = cJSON_Number;

    input_buffer->offset += (size_t)(after_end - number_c_string);
    return true;
}

/* don't ask me, but the original cJSON_SetNumberValue returns an integer or double */
auto
cJSON_SetNumberHelper(cJSON *object, double number) -> CJSON_PUBLIC(double) {
    if (number >= INT_MAX) {
        object->valueint = INT_MAX;
    } else if (number <= (double) INT_MIN) {
        object->valueint = INT_MIN;
    } else {
        object->valueint = (int) number;
    }

    return object->valuedouble = number;
}

auto
cJSON_SetValuestring(cJSON *object, const char *valuestring) -> CJSON_PUBLIC(char *) {
    char *copy = nullptr;
    /* if object's type is not cJSON_String or is cJSON_IsReference, it should not set valuestring */
    if (!(object->type & cJSON_String) || (object->type & cJSON_IsReference)) {
        return nullptr;
    }
    if (strlen(valuestring) <= strlen(object->valuestring)) {
        strcpy(object->valuestring, valuestring);
        return object->valuestring;
    }
    copy = (char *) cJSON_strdup((const unsigned char *) valuestring, &global_hooks);
    if (copy == nullptr) {
        return nullptr;
    }
    if (object->valuestring != nullptr) {
        cJSON_free(object->valuestring);
    }
    object->valuestring = copy;

    return copy;
}

using printbuffer = struct {
    unsigned char *buffer;
    size_t length;
    size_t offset;
    size_t depth; /* current nesting depth (for formatted printing) */
    cJSON_bool noalloc;
    cJSON_bool format; /* is this print a formatted print */
    internal_hooks hooks;
};

/* realloc printbuffer if necessary to have at least "needed" bytes more */
static auto ensure(printbuffer *const p, size_t needed) -> unsigned char * {
    unsigned char *newbuffer = nullptr;
    size_t newsize = 0;

    if ((p == nullptr) || (p->buffer == nullptr)) {
        return nullptr;
    }

    if ((p->length > 0) && (p->offset >= p->length)) {
        /* make sure that offset is valid */
        return nullptr;
    }

    if (needed > INT_MAX) {
        /* sizes bigger than INT_MAX are currently not supported */
        return nullptr;
    }

    needed += p->offset + 1;
    if (needed <= p->length) {
        return p->buffer + p->offset;
    }

    if (p->noalloc) {
        return nullptr;
    }

    /* calculate new buffer size */
    if (needed > (INT_MAX / 2)) {
        /* overflow of int, use INT_MAX if possible */
        if (needed <= INT_MAX) {
            newsize = INT_MAX;
        } else {
            return nullptr;
        }
    } else {
        newsize = needed * 2;
    }

    if (p->hooks.reallocate != nullptr) {
        /* reallocate with realloc if available */
        newbuffer = (unsigned char *) p->hooks.reallocate(p->buffer, newsize);
        if (newbuffer == nullptr) {
            p->hooks.deallocate(p->buffer);
            p->length = 0;
            p->buffer = nullptr;

            return nullptr;
        }
    } else {
        /* otherwise reallocate manually */
        newbuffer = (unsigned char *) p->hooks.allocate(newsize);
        if (!newbuffer) {
            p->hooks.deallocate(p->buffer);
            p->length = 0;
            p->buffer = nullptr;

            return nullptr;
        }

        memcpy(newbuffer, p->buffer, p->offset + 1);
        p->hooks.deallocate(p->buffer);
    }
    p->length = newsize;
    p->buffer = newbuffer;

    return newbuffer + p->offset;
}

/* calculate the new length of the string in a printbuffer and update the offset */
static void update_offset(printbuffer *const buffer) {
    const unsigned char *buffer_pointer = nullptr;
    if ((buffer == nullptr) || (buffer->buffer == nullptr)) {
        return;
    }
    buffer_pointer = buffer->buffer + buffer->offset;

    buffer->offset += strlen((const char *) buffer_pointer);
}

/* securely comparison of floating-point variables */
static auto compare_double(double a, double b) -> cJSON_bool {
    double maxVal = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return (fabs(a - b) <= maxVal * DBL_EPSILON);
}

/* Render the number nicely from the given item into a string. */
static auto print_number(const cJSON *const item, printbuffer *const output_buffer) -> cJSON_bool {
    unsigned char *output_pointer = nullptr;
    double d = item->valuedouble;
    int length = 0;
    size_t i = 0;
    unsigned char number_buffer[26] = {0}; /* temporary buffer to print the number into */
    unsigned char decimal_point = get_decimal_point();
    double test = 0.0;

    if (output_buffer == nullptr) {
        return false;
    }

    /* This checks for NaN and Infinity */
    if (isnan(d) || isinf(d)) {
        length = sprintf((char *) number_buffer, "null");
    } else {
        /* Try 15 decimal places of precision to avoid nonsignificant nonzero digits */
        length = sprintf((char *) number_buffer, "%1.15g", d);

        /* Check whether the original double can be recovered */
        if ((sscanf((char *) number_buffer, "%lg", &test) != 1) || !compare_double((double) test, d)) {
            /* If not, print with 17 decimal places of precision */
            length = sprintf((char *) number_buffer, "%1.17g", d);
        }
    }

    /* sprintf failed or buffer overrun occurred */
    if ((length < 0) || (length > (int) (sizeof(number_buffer) - 1))) {
        return false;
    }

    /* reserve appropriate space in the output */
    output_pointer = ensure(output_buffer, (size_t) length + sizeof(""));
    if (output_pointer == nullptr) {
        return false;
    }

    /* copy the printed number to the output and replace locale
     * dependent decimal point with '.' */
    for (i = 0; i < ((size_t) length); i++) {
        if (number_buffer[i] == decimal_point) {
            output_pointer[i] = '.';
            continue;
        }

        output_pointer[i] = number_buffer[i];
    }
    output_pointer[i] = '\0';

    output_buffer->offset += (size_t) length;

    return true;
}

/* parse 4 digit hexadecimal number */
static auto parse_hex4(const unsigned char *const input) -> unsigned {
    unsigned int h = 0;
    size_t i = 0;

    for (i = 0; i < 4; i++) {
        /* parse digit */
        if ((input[i] >= '0') && (input[i] <= '9')) {
            h += (unsigned int) input[i] - '0';
        } else if ((input[i] >= 'A') && (input[i] <= 'F')) {
            h += (unsigned int) 10 + input[i] - 'A';
        } else if ((input[i] >= 'a') && (input[i] <= 'f')) {
            h += (unsigned int) 10 + input[i] - 'a';
        } else /* invalid */
        {
            return 0;
        }

        if (i < 3) {
            /* shift left to make place for the next nibble */
            h = h << 4;
        }
    }

    return h;
}

/* converts a UTF-16 literal to UTF-8
 * A literal can be one or two sequences of the form \uXXXX */
static auto
utf16_literal_to_utf8(const unsigned char *const input_pointer, const unsigned char *const input_end,
                      unsigned char **output_pointer) -> unsigned char {
    long unsigned int codepoint = 0;
    unsigned int first_code = 0;
    const unsigned char *first_sequence = input_pointer;
    unsigned char utf8_length = 0;
    unsigned char utf8_position = 0;
    unsigned char sequence_length = 0;
    unsigned char first_byte_mark = 0;

    if ((input_end - first_sequence) < 6) {
        /* input ends unexpectedly */
        goto fail;
    }

    /* get the first utf16 sequence */
    first_code = parse_hex4(first_sequence + 2);

    /* check that the code is valid */
    if (((first_code >= 0xDC00) && (first_code <= 0xDFFF))) {
        goto fail;
    }

    /* UTF16 surrogate pair */
    if ((first_code >= 0xD800) && (first_code <= 0xDBFF)) {
        const unsigned char *second_sequence = first_sequence + 6;
        unsigned int second_code = 0;
        sequence_length = 12; /* \uXXXX\uXXXX */

        if ((input_end - second_sequence) < 6) {
            /* input ends unexpectedly */
            goto fail;
        }

        if ((second_sequence[0] != '\\') || (second_sequence[1] != 'u')) {
            /* missing second half of the surrogate pair */
            goto fail;
        }

        /* get the second utf16 sequence */
        second_code = parse_hex4(second_sequence + 2);
        /* check that the code is valid */
        if ((second_code < 0xDC00) || (second_code > 0xDFFF)) {
            /* invalid second half of the surrogate pair */
            goto fail;
        }


        /* calculate the unicode codepoint from the surrogate pair */
        codepoint = 0x10000 + (((first_code & 0x3FF) << 10) | (second_code & 0x3FF));
    } else {
        sequence_length = 6; /* \uXXXX */
        codepoint = first_code;
    }

    /* encode as UTF-8
     * takes at maximum 4 bytes to encode:
     * 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
    if (codepoint < 0x80) {
        /* normal ascii, encoding 0xxxxxxx */
        utf8_length = 1;
    } else if (codepoint < 0x800) {
        /* two bytes, encoding 110xxxxx 10xxxxxx */
        utf8_length = 2;
        first_byte_mark = 0xC0; /* 11000000 */
    } else if (codepoint < 0x10000) {
        /* three bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx */
        utf8_length = 3;
        first_byte_mark = 0xE0; /* 11100000 */
    } else if (codepoint <= 0x10FFFF) {
        /* four bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx 10xxxxxx */
        utf8_length = 4;
        first_byte_mark = 0xF0; /* 11110000 */
    } else {
        /* invalid unicode codepoint */
        goto fail;
    }

    /* encode as utf8 */
    for (utf8_position = (unsigned char) (utf8_length - 1); utf8_position > 0; utf8_position--) {
        /* 10xxxxxx */
        (*output_pointer)[utf8_position] = (unsigned char) ((codepoint | 0x80) & 0xBF);
        codepoint >>= 6;
    }
    /* encode first byte */
    if (utf8_length > 1) {
        (*output_pointer)[0] = (unsigned char) ((codepoint | first_byte_mark) & 0xFF);
    } else {
        (*output_pointer)[0] = (unsigned char) (codepoint & 0x7F);
    }

    *output_pointer += utf8_length;

    return sequence_length;

    fail:
    return 0;
}

/* Parse the input text into an unescaped cinput, and populate item. */
static auto parse_string(cJSON *const item, parse_buffer *const input_buffer) -> cJSON_bool {
    const unsigned char *input_pointer = buffer_at_offset(input_buffer) + 1;
    const unsigned char *input_end = buffer_at_offset(input_buffer) + 1;
    unsigned char *output_pointer = nullptr;
    unsigned char *output = nullptr;

    /* not a string */
    if (buffer_at_offset(input_buffer)[0] != '\"') {
        goto fail;
    }

    {
        /* calculate approximate size of the output (overestimate) */
        size_t allocation_length = 0;
        size_t skipped_bytes = 0;
        while (((size_t)(input_end - input_buffer->content) < input_buffer->length) && (*input_end != '\"')) {
            /* is escape sequence */
            if (input_end[0] == '\\') {
                if ((size_t)(input_end + 1 - input_buffer->content) >= input_buffer->length) {
                    /* prevent buffer overflow when last input character is a backslash */
                    goto fail;
                }
                skipped_bytes++;
                input_end++;
            }
            input_end++;
        }
        if (((size_t)(input_end - input_buffer->content) >= input_buffer->length) || (*input_end != '\"')) {
            goto fail; /* string ended unexpectedly */
        }

        /* This is at most how much we need for the output */
        allocation_length = (size_t)(input_end - buffer_at_offset(input_buffer)) - skipped_bytes;
        output = (unsigned char *) input_buffer->hooks.allocate(allocation_length + sizeof(""));
        if (output == nullptr) {
            goto fail; /* allocation failure */
        }
    }

    output_pointer = output;
    /* loop through the string literal */
    while (input_pointer < input_end) {
        if (*input_pointer != '\\') {
            *output_pointer++ = *input_pointer++;
        }
            /* escape sequence */
        else {
            unsigned char sequence_length = 2;
            if ((input_end - input_pointer) < 1) {
                goto fail;
            }

            switch (input_pointer[1]) {
                case 'b':
                    *output_pointer++ = '\b';
                    break;
                case 'f':
                    *output_pointer++ = '\f';
                    break;
                case 'n':
                    *output_pointer++ = '\n';
                    break;
                case 'r':
                    *output_pointer++ = '\r';
                    break;
                case 't':
                    *output_pointer++ = '\t';
                    break;
                case '\"':
                case '\\':
                case '/':
                    *output_pointer++ = input_pointer[1];
                    break;

                    /* UTF-16 literal */
                case 'u':
                    sequence_length = utf16_literal_to_utf8(input_pointer, input_end, &output_pointer);
                    if (sequence_length == 0) {
                        /* failed to convert UTF16-literal to UTF-8 */
                        goto fail;
                    }
                    break;

                default:
                    goto fail;
            }
            input_pointer += sequence_length;
        }
    }

    /* zero terminate the output */
    *output_pointer = '\0';

    item->type = cJSON_String;
    item->valuestring = (char *) output;

    input_buffer->offset = (size_t)(input_end - input_buffer->content);
    input_buffer->offset++;

    return true;

    fail:
    if (output != nullptr) {
        input_buffer->hooks.deallocate(output);
    }

    if (input_pointer != nullptr) {
        input_buffer->offset = (size_t)(input_pointer - input_buffer->content);
    }

    return false;
}

/* Render the cstring provided to an escaped version that can be printed. */
static auto print_string_ptr(const unsigned char *const input, printbuffer *const output_buffer) -> cJSON_bool {
    const unsigned char *input_pointer = nullptr;
    unsigned char *output = nullptr;
    unsigned char *output_pointer = nullptr;
    size_t output_length = 0;
    /* numbers of additional characters needed for escaping */
    size_t escape_characters = 0;

    if (output_buffer == nullptr) {
        return false;
    }

    /* empty string */
    if (input == nullptr) {
        output = ensure(output_buffer, sizeof("\"\""));
        if (output == nullptr) {
            return false;
        }
        strcpy((char *) output, "\"\"");

        return true;
    }

    /* set "flag" to 1 if something needs to be escaped */
    for (input_pointer = input; *input_pointer; input_pointer++) {
        switch (*input_pointer) {
            case '\"':
            case '\\':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                /* one character escape sequence */
                escape_characters++;
                break;
            default:
                if (*input_pointer < 32) {
                    /* UTF-16 escape sequence uXXXX */
                    escape_characters += 5;
                }
                break;
        }
    }
    output_length = (size_t)(input_pointer - input) + escape_characters;

    output = ensure(output_buffer, output_length + sizeof("\"\""));
    if (output == nullptr) {
        return false;
    }

    /* no characters have to be escaped */
    if (escape_characters == 0) {
        output[0] = '\"';
        memcpy(output + 1, input, output_length);
        output[output_length + 1] = '\"';
        output[output_length + 2] = '\0';

        return true;
    }

    output[0] = '\"';
    output_pointer = output + 1;
    /* copy the string */
    for (input_pointer = input; *input_pointer != '\0'; (void) input_pointer++, output_pointer++) {
        if ((*input_pointer > 31) && (*input_pointer != '\"') && (*input_pointer != '\\')) {
            /* normal character, copy */
            *output_pointer = *input_pointer;
        } else {
            /* character needs to be escaped */
            *output_pointer++ = '\\';
            switch (*input_pointer) {
                case '\\':
                    *output_pointer = '\\';
                    break;
                case '\"':
                    *output_pointer = '\"';
                    break;
                case '\b':
                    *output_pointer = 'b';
                    break;
                case '\f':
                    *output_pointer = 'f';
                    break;
                case '\n':
                    *output_pointer = 'n';
                    break;
                case '\r':
                    *output_pointer = 'r';
                    break;
                case '\t':
                    *output_pointer = 't';
                    break;
                default:
                    /* escape and print as unicode codepoint */
                    sprintf((char *) output_pointer, "u%04x", *input_pointer);
                    output_pointer += 4;
                    break;
            }
        }
    }
    output[output_length + 1] = '\"';
    output[output_length + 2] = '\0';

    return true;
}

/* Invoke print_string_ptr (which is useful) on an item. */
static auto print_string(const cJSON *const item, printbuffer *const p) -> cJSON_bool {
    return print_string_ptr((unsigned char *) item->valuestring, p);
}

/* Predeclare these prototypes. */
static auto parse_value(cJSON *const item, parse_buffer *const input_buffer) -> cJSON_bool;

static auto print_value(const cJSON *const item, printbuffer *const output_buffer) -> cJSON_bool;

static auto parse_array(cJSON *const item, parse_buffer *const input_buffer) -> cJSON_bool;

static auto print_array(const cJSON *const item, printbuffer *const output_buffer) -> cJSON_bool;

static auto parse_object(cJSON *const item, parse_buffer *const input_buffer) -> cJSON_bool;

static auto print_object(const cJSON *const item, printbuffer *const output_buffer) -> cJSON_bool;

/* Utility to jump whitespace and cr/lf */
static auto buffer_skip_whitespace(parse_buffer *const buffer) -> parse_buffer * {
    if ((buffer == nullptr) || (buffer->content == nullptr)) {
        return nullptr;
    }

    if (cannot_access_at_index(buffer, 0)) {
        return buffer;
    }

    while (can_access_at_index(buffer, 0) && (buffer_at_offset(buffer)[0] <= 32)) {
        buffer->offset++;
    }

    if (buffer->offset == buffer->length) {
        buffer->offset--;
    }

    return buffer;
}

/* skip the UTF-8 BOM (byte order mark) if it is at the beginning of a buffer */
static auto skip_utf8_bom(parse_buffer *const buffer) -> parse_buffer * {
    if ((buffer == nullptr) || (buffer->content == nullptr) || (buffer->offset != 0)) {
        return nullptr;
    }

    if (can_access_at_index(buffer, 4) && (strncmp((const char *) buffer_at_offset(buffer), "\xEF\xBB\xBF", 3) == 0)) {
        buffer->offset += 3;
    }

    return buffer;
}

auto
cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated) -> CJSON_PUBLIC(cJSON *) {
    size_t buffer_length;

    if (nullptr == value) {
        return nullptr;
    }

    /* Adding null character size due to require_null_terminated. */
    buffer_length = strlen(value) + sizeof("");

    return cJSON_ParseWithLengthOpts(value, buffer_length, return_parse_end, require_null_terminated);
}

/* Parse an object - create a new root, and populate. */
auto
cJSON_ParseWithLengthOpts(const char *value, size_t buffer_length, const char **return_parse_end,
                          cJSON_bool require_null_terminated) -> CJSON_PUBLIC(cJSON *) {
    parse_buffer buffer = {nullptr, 0, 0, 0, {nullptr, nullptr, nullptr}};
    cJSON *item = nullptr;

    /* reset error position */
    global_error.json = nullptr;
    global_error.position = 0;

    if (value == nullptr || 0 == buffer_length) {
        goto fail;
    }

    buffer.content = (const unsigned char *) value;
    buffer.length = buffer_length;
    buffer.offset = 0;
    buffer.hooks = global_hooks;

    item = cJSON_New_Item(&global_hooks);
    if (item == nullptr) /* memory fail */
    {
        goto fail;
    }

    if (!parse_value(item, buffer_skip_whitespace(skip_utf8_bom(&buffer)))) {
        /* parse failure. ep is set. */
        goto fail;
    }

    /* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
    if (require_null_terminated) {
        buffer_skip_whitespace(&buffer);
        if ((buffer.offset >= buffer.length) || buffer_at_offset(&buffer)[0] != '\0') {
            goto fail;
        }
    }
    if (return_parse_end) {
        *return_parse_end = (const char *) buffer_at_offset(&buffer);
    }

    return item;

    fail:
    if (item != nullptr) {
        cJSON_Delete(item);
    }

    if (value != nullptr) {
        error local_error;
        local_error.json = (const unsigned char *) value;
        local_error.position = 0;

        if (buffer.offset < buffer.length) {
            local_error.position = buffer.offset;
        } else if (buffer.length > 0) {
            local_error.position = buffer.length - 1;
        }

        if (return_parse_end != nullptr) {
            *return_parse_end = (const char *) local_error.json + local_error.position;
        }

        global_error = local_error;
    }

    return nullptr;
}

/* Default options for cJSON_Parse */
auto
cJSON_Parse(const char *value) -> CJSON_PUBLIC(cJSON *) {
    return cJSON_ParseWithOpts(value, nullptr, 0);
}

auto
cJSON_ParseWithLength(const char *value, size_t buffer_length) -> CJSON_PUBLIC(cJSON *) {
    return cJSON_ParseWithLengthOpts(value, buffer_length, nullptr, 0);
}

#define cjson_min(a, b) (((a) < (b)) ? (a) : (b))

static auto print(const cJSON *const item, cJSON_bool format, const internal_hooks *const hooks) -> unsigned char * {
    static const size_t default_buffer_size = 256;
    printbuffer buffer[1];
    unsigned char *printed = nullptr;

    memset(buffer, 0, sizeof(buffer));

    /* create buffer */
    buffer->buffer = (unsigned char *) hooks->allocate(default_buffer_size);
    buffer->length = default_buffer_size;
    buffer->format = format;
    buffer->hooks = *hooks;
    if (buffer->buffer == nullptr) {
        goto fail;
    }

    /* print the value */
    if (!print_value(item, buffer)) {
        goto fail;
    }
    update_offset(buffer);

    /* check if reallocate is available */
    if (hooks->reallocate != nullptr) {
        printed = (unsigned char *) hooks->reallocate(buffer->buffer, buffer->offset + 1);
        if (printed == nullptr) {
            goto fail;
        }
        buffer->buffer = nullptr;
    } else /* otherwise copy the JSON over to a new buffer */
    {
        printed = (unsigned char *) hooks->allocate(buffer->offset + 1);
        if (printed == nullptr) {
            goto fail;
        }
        memcpy(printed, buffer->buffer, cjson_min(buffer->length, buffer->offset + 1));
        printed[buffer->offset] = '\0'; /* just to be sure */

        /* free the buffer */
        hooks->deallocate(buffer->buffer);
    }

    return printed;

    fail:
    if (buffer->buffer != nullptr) {
        hooks->deallocate(buffer->buffer);
    }

    if (printed != nullptr) {
        hooks->deallocate(printed);
    }

    return nullptr;
}

/* Render a cJSON item/entity/structure to text. */
auto
cJSON_Print(const cJSON *item) -> CJSON_PUBLIC(char *) {
    return (char *) print(item, true, &global_hooks);
}

auto
cJSON_PrintUnformatted(const cJSON *item) -> CJSON_PUBLIC(char *) {
    return (char *) print(item, false, &global_hooks);
}

auto
cJSON_PrintBuffered(const cJSON *item, int prebuffer, cJSON_bool fmt) -> CJSON_PUBLIC(char *) {
    printbuffer p = {nullptr, 0, 0, 0, 0, 0, {nullptr, nullptr, nullptr}};

    if (prebuffer < 0) {
        return nullptr;
    }

    p.buffer = (unsigned char *) global_hooks.allocate((size_t) prebuffer);
    if (!p.buffer) {
        return nullptr;
    }

    p.length = (size_t) prebuffer;
    p.offset = 0;
    p.noalloc = false;
    p.format = fmt;
    p.hooks = global_hooks;

    if (!print_value(item, &p)) {
        global_hooks.deallocate(p.buffer);
        return nullptr;
    }

    return (char *) p.buffer;
}

auto
cJSON_PrintPreallocated(cJSON *item, char *buffer, const int length, const cJSON_bool format) -> CJSON_PUBLIC(cJSON_bool) {
    printbuffer p = {nullptr, 0, 0, 0, 0, 0, {nullptr, nullptr, nullptr}};

    if ((length < 0) || (buffer == nullptr)) {
        return false;
    }

    p.buffer = (unsigned char *) buffer;
    p.length = (size_t) length;
    p.offset = 0;
    p.noalloc = true;
    p.format = format;
    p.hooks = global_hooks;

    return print_value(item, &p);
}

/* Parser core - when encountering text, process appropriately. */
static auto parse_value(cJSON *const item, parse_buffer *const input_buffer) -> cJSON_bool {
    if ((input_buffer == nullptr) || (input_buffer->content == nullptr)) {
        return false; /* no input */
    }

    /* parse the different types of values */
    /* null */
    if (can_read(input_buffer, 4) && (strncmp((const char *) buffer_at_offset(input_buffer), "null", 4) == 0)) {
        item->type = cJSON_NULL;
        input_buffer->offset += 4;
        return true;
    }
    /* false */
    if (can_read(input_buffer, 5) && (strncmp((const char *) buffer_at_offset(input_buffer), "false", 5) == 0)) {
        item->type = cJSON_False;
        input_buffer->offset += 5;
        return true;
    }
    /* true */
    if (can_read(input_buffer, 4) && (strncmp((const char *) buffer_at_offset(input_buffer), "true", 4) == 0)) {
        item->type = cJSON_True;
        item->valueint = 1;
        input_buffer->offset += 4;
        return true;
    }
    /* string */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '\"')) {
        return parse_string(item, input_buffer);
    }
    /* number */
    if (can_access_at_index(input_buffer, 0) && ((buffer_at_offset(input_buffer)[0] == '-') ||
                                                 ((buffer_at_offset(input_buffer)[0] >= '0') &&
                                                  (buffer_at_offset(input_buffer)[0] <= '9')))) {
        return parse_number(item, input_buffer);
    }
    /* array */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '[')) {
        return parse_array(item, input_buffer);
    }
    /* object */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '{')) {
        return parse_object(item, input_buffer);
    }

    return false;
}

/* Render a value to text. */
static auto print_value(const cJSON *const item, printbuffer *const output_buffer) -> cJSON_bool {
    unsigned char *output = nullptr;

    if ((item == nullptr) || (output_buffer == nullptr)) {
        return false;
    }

    switch ((item->type) & 0xFF) {
        case cJSON_NULL:
            output = ensure(output_buffer, 5);
            if (output == nullptr) {
                return false;
            }
            strcpy((char *) output, "null");
            return true;

        case cJSON_False:
            output = ensure(output_buffer, 6);
            if (output == nullptr) {
                return false;
            }
            strcpy((char *) output, "false");
            return true;

        case cJSON_True:
            output = ensure(output_buffer, 5);
            if (output == nullptr) {
                return false;
            }
            strcpy((char *) output, "true");
            return true;

        case cJSON_Number:
            return print_number(item, output_buffer);

        case cJSON_Raw: {
            size_t raw_length = 0;
            if (item->valuestring == nullptr) {
                return false;
            }

            raw_length = strlen(item->valuestring) + sizeof("");
            output = ensure(output_buffer, raw_length);
            if (output == nullptr) {
                return false;
            }
            memcpy(output, item->valuestring, raw_length);
            return true;
        }

        case cJSON_String:
            return print_string(item, output_buffer);

        case cJSON_Array:
            return print_array(item, output_buffer);

        case cJSON_Object:
            return print_object(item, output_buffer);

        default:
            return false;
    }
}

/* Build an array from input text. */
static auto parse_array(cJSON *const item, parse_buffer *const input_buffer) -> cJSON_bool {
    cJSON *head = nullptr; /* head of the linked list */
    cJSON *current_item = nullptr;

    if (input_buffer->depth >= CJSON_NESTING_LIMIT) {
        return false; /* to deeply nested */
    }
    input_buffer->depth++;

    if (buffer_at_offset(input_buffer)[0] != '[') {
        /* not an array */
        goto fail;
    }

    input_buffer->offset++;
    buffer_skip_whitespace(input_buffer);
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ']')) {
        /* empty array */
        goto success;
    }

    /* check if we skipped to the end of the buffer */
    if (cannot_access_at_index(input_buffer, 0)) {
        input_buffer->offset--;
        goto fail;
    }

    /* step back to character in front of the first element */
    input_buffer->offset--;
    /* loop through the comma separated array elements */
    do {
        /* allocate next item */
        cJSON *new_item = cJSON_New_Item(&(input_buffer->hooks));
        if (new_item == nullptr) {
            goto fail; /* allocation failure */
        }

        /* attach next item to list */
        if (head == nullptr) {
            /* start the linked list */
            current_item = head = new_item;
        } else {
            /* add to the end and advance */
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }

        /* parse next value */
        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_value(current_item, input_buffer)) {
            goto fail; /* failed to parse value */
        }
        buffer_skip_whitespace(input_buffer);
    } while (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ','));

    if (cannot_access_at_index(input_buffer, 0) || buffer_at_offset(input_buffer)[0] != ']') {
        goto fail; /* expected end of array */
    }

    success:
    input_buffer->depth--;

    if (head != nullptr) {
        head->prev = current_item;
    }

    item->type = cJSON_Array;
    item->child = head;

    input_buffer->offset++;

    return true;

    fail:
    if (head != nullptr) {
        cJSON_Delete(head);
    }

    return false;
}

/* Render an array to text */
static auto print_array(const cJSON *const item, printbuffer *const output_buffer) -> cJSON_bool {
    unsigned char *output_pointer = nullptr;
    size_t length = 0;
    cJSON *current_element = item->child;

    if (output_buffer == nullptr) {
        return false;
    }

    /* Compose the output array. */
    /* opening square bracket */
    output_pointer = ensure(output_buffer, 1);
    if (output_pointer == nullptr) {
        return false;
    }

    *output_pointer = '[';
    output_buffer->offset++;
    output_buffer->depth++;

    while (current_element != nullptr) {
        if (!print_value(current_element, output_buffer)) {
            return false;
        }
        update_offset(output_buffer);
        if (current_element->next) {
            length = (size_t)(output_buffer->format ? 2 : 1);
            output_pointer = ensure(output_buffer, length + 1);
            if (output_pointer == nullptr) {
                return false;
            }
            *output_pointer++ = ',';
            if (output_buffer->format) {
                *output_pointer++ = ' ';
            }
            *output_pointer = '\0';
            output_buffer->offset += length;
        }
        current_element = current_element->next;
    }

    output_pointer = ensure(output_buffer, 2);
    if (output_pointer == nullptr) {
        return false;
    }
    *output_pointer++ = ']';
    *output_pointer = '\0';
    output_buffer->depth--;

    return true;
}

/* Build an object from the text. */
static auto parse_object(cJSON *const item, parse_buffer *const input_buffer) -> cJSON_bool {
    cJSON *head = nullptr; /* linked list head */
    cJSON *current_item = nullptr;

    if (input_buffer->depth >= CJSON_NESTING_LIMIT) {
        return false; /* to deeply nested */
    }
    input_buffer->depth++;

    if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != '{')) {
        goto fail; /* not an object */
    }

    input_buffer->offset++;
    buffer_skip_whitespace(input_buffer);
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '}')) {
        goto success; /* empty object */
    }

    /* check if we skipped to the end of the buffer */
    if (cannot_access_at_index(input_buffer, 0)) {
        input_buffer->offset--;
        goto fail;
    }

    /* step back to character in front of the first element */
    input_buffer->offset--;
    /* loop through the comma separated array elements */
    do {
        /* allocate next item */
        cJSON *new_item = cJSON_New_Item(&(input_buffer->hooks));
        if (new_item == nullptr) {
            goto fail; /* allocation failure */
        }

        /* attach next item to list */
        if (head == nullptr) {
            /* start the linked list */
            current_item = head = new_item;
        } else {
            /* add to the end and advance */
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }

        /* parse the name of the child */
        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_string(current_item, input_buffer)) {
            goto fail; /* failed to parse name */
        }
        buffer_skip_whitespace(input_buffer);

        /* swap valuestring and string, because we parsed the name */
        current_item->string = current_item->valuestring;
        current_item->valuestring = nullptr;

        if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != ':')) {
            goto fail; /* invalid object */
        }

        /* parse the value */
        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_value(current_item, input_buffer)) {
            goto fail; /* failed to parse value */
        }
        buffer_skip_whitespace(input_buffer);
    } while (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ','));

    if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != '}')) {
        goto fail; /* expected end of object */
    }

    success:
    input_buffer->depth--;

    if (head != nullptr) {
        head->prev = current_item;
    }

    item->type = cJSON_Object;
    item->child = head;

    input_buffer->offset++;
    return true;

    fail:
    if (head != nullptr) {
        cJSON_Delete(head);
    }

    return false;
}

/* Render an object to text. */
static auto print_object(const cJSON *const item, printbuffer *const output_buffer) -> cJSON_bool {
    unsigned char *output_pointer = nullptr;
    size_t length = 0;
    cJSON *current_item = item->child;

    if (output_buffer == nullptr) {
        return false;
    }

    /* Compose the output: */
    length = (size_t)(output_buffer->format ? 2 : 1); /* fmt: {\n */
    output_pointer = ensure(output_buffer, length + 1);
    if (output_pointer == nullptr) {
        return false;
    }

    *output_pointer++ = '{';
    output_buffer->depth++;
    if (output_buffer->format) {
        *output_pointer++ = '\n';
    }
    output_buffer->offset += length;

    while (current_item) {
        if (output_buffer->format) {
            size_t i;
            output_pointer = ensure(output_buffer, output_buffer->depth);
            if (output_pointer == nullptr) {
                return false;
            }
            for (i = 0; i < output_buffer->depth; i++) {
                *output_pointer++ = '\t';
            }
            output_buffer->offset += output_buffer->depth;
        }

        /* print key */
        if (!print_string_ptr((unsigned char *) current_item->string, output_buffer)) {
            return false;
        }
        update_offset(output_buffer);

        length = (size_t)(output_buffer->format ? 2 : 1);
        output_pointer = ensure(output_buffer, length);
        if (output_pointer == nullptr) {
            return false;
        }
        *output_pointer++ = ':';
        if (output_buffer->format) {
            *output_pointer++ = '\t';
        }
        output_buffer->offset += length;

        /* print value */
        if (!print_value(current_item, output_buffer)) {
            return false;
        }
        update_offset(output_buffer);

        /* print comma if not last */
        length = ((size_t)(output_buffer->format ? 1 : 0) + (size_t)(current_item->next ? 1 : 0));
        output_pointer = ensure(output_buffer, length + 1);
        if (output_pointer == nullptr) {
            return false;
        }
        if (current_item->next) {
            *output_pointer++ = ',';
        }

        if (output_buffer->format) {
            *output_pointer++ = '\n';
        }
        *output_pointer = '\0';
        output_buffer->offset += length;

        current_item = current_item->next;
    }

    output_pointer = ensure(output_buffer, output_buffer->format ? (output_buffer->depth + 1) : 2);
    if (output_pointer == nullptr) {
        return false;
    }
    if (output_buffer->format) {
        size_t i;
        for (i = 0; i < (output_buffer->depth - 1); i++) {
            *output_pointer++ = '\t';
        }
    }
    *output_pointer++ = '}';
    *output_pointer = '\0';
    output_buffer->depth--;

    return true;
}

/* Get Array size/item / object item. */
auto
cJSON_GetArraySize(const cJSON *array) -> CJSON_PUBLIC(int) {
    cJSON *child = nullptr;
    size_t size = 0;

    if (array == nullptr) {
        return 0;
    }

    child = array->child;

    while (child != nullptr) {
        size++;
        child = child->next;
    }

    /* FIXME: Can overflow here. Cannot be fixed without breaking the API */

    return (int) size;
}

static auto get_array_item(const cJSON *array, size_t index) -> cJSON * {
    cJSON *current_child = nullptr;

    if (array == nullptr) {
        return nullptr;
    }

    current_child = array->child;
    while ((current_child != nullptr) && (index > 0)) {
        index--;
        current_child = current_child->next;
    }

    return current_child;
}

auto
cJSON_GetArrayItem(const cJSON *array, int index) -> CJSON_PUBLIC(cJSON *) {
    if (index < 0) {
        return nullptr;
    }

    return get_array_item(array, (size_t) index);
}

static auto get_object_item(const cJSON *const object, const char *const name, const cJSON_bool case_sensitive) -> cJSON * {
    cJSON *current_element = nullptr;

    if ((object == nullptr) || (name == nullptr)) {
        return nullptr;
    }

    current_element = object->child;
    if (case_sensitive) {
        while ((current_element != nullptr) && (current_element->string != nullptr) &&
               (strcmp(name, current_element->string) != 0)) {
            current_element = current_element->next;
        }
    } else {
        while ((current_element != nullptr) && (case_insensitive_strcmp((const unsigned char *) name,
                                                                     (const unsigned char *) (current_element->string)) !=
                                             0)) {
            current_element = current_element->next;
        }
    }

    if ((current_element == nullptr) || (current_element->string == nullptr)) {
        return nullptr;
    }

    return current_element;
}

auto
cJSON_GetObjectItem(const cJSON *const object, const char *const string) -> CJSON_PUBLIC(cJSON *) {
    return get_object_item(object, string, false);
}

auto
cJSON_GetObjectItemCaseSensitive(const cJSON *const object, const char *const string) -> CJSON_PUBLIC(cJSON *) {
    return get_object_item(object, string, true);
}

auto
cJSON_HasObjectItem(const cJSON *object, const char *string) -> CJSON_PUBLIC(cJSON_bool) {
    return cJSON_GetObjectItem(object, string) ? 1 : 0;
}

/* Utility for array list handling. */
static void suffix_object(cJSON *prev, cJSON *item) {
    prev->next = item;
    item->prev = prev;
}

/* Utility for handling references. */
static auto create_reference(const cJSON *item, const internal_hooks *const hooks) -> cJSON * {
    cJSON *reference = nullptr;
    if (item == nullptr) {
        return nullptr;
    }

    reference = cJSON_New_Item(hooks);
    if (reference == nullptr) {
        return nullptr;
    }

    memcpy(reference, item, sizeof(cJSON));
    reference->string = nullptr;
    reference->type |= cJSON_IsReference;
    reference->next = reference->prev = nullptr;
    return reference;
}

static auto add_item_to_array(cJSON *array, cJSON *item) -> cJSON_bool {
    cJSON *child = nullptr;

    if ((item == nullptr) || (array == nullptr) || (array == item)) {
        return false;
    }

    child = array->child;
    /*
     * To find the last item in array quickly, we use prev in array
     */
    if (child == nullptr) {
        /* list is empty, start new one */
        array->child = item;
        item->prev = item;
        item->next = nullptr;
    } else {
        /* append to the end */
        if (child->prev) {
            suffix_object(child->prev, item);
            array->child->prev = item;
        }
    }

    return true;
}

/* Add item to array/object. */
auto
cJSON_AddItemToArray(cJSON *array, cJSON *item) -> CJSON_PUBLIC(cJSON_bool) {
    return add_item_to_array(array, item);
}

#if defined(__clang__) || (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif

/* helper function to cast away const */
static auto cast_away_const(const void *string) -> void * {
    return (void *) string;
}

#if defined(__clang__) || (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
#pragma GCC diagnostic pop
#endif


static auto
add_item_to_object(cJSON *const object, const char *const string, cJSON *const item, const internal_hooks *const hooks,
                   const cJSON_bool constant_key) -> cJSON_bool {
    char *new_key = nullptr;
    int new_type = cJSON_Invalid;

    if ((object == nullptr) || (string == nullptr) || (item == nullptr) || (object == item)) {
        return false;
    }

    if (constant_key) {
        new_key = (char *) cast_away_const(string);
        new_type = item->type | cJSON_StringIsConst;
    } else {
        new_key = (char *) cJSON_strdup((const unsigned char *) string, hooks);
        if (new_key == nullptr) {
            return false;
        }

        new_type = item->type & ~cJSON_StringIsConst;
    }

    if (!(item->type & cJSON_StringIsConst) && (item->string != nullptr)) {
        hooks->deallocate(item->string);
    }

    item->string = new_key;
    item->type = new_type;

    return add_item_to_array(object, item);
}

auto
cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item) -> CJSON_PUBLIC(cJSON_bool) {
    return add_item_to_object(object, string, item, &global_hooks, false);
}

/* Add an item to an object with constant string as key */
auto
cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item) -> CJSON_PUBLIC(cJSON_bool) {
    return add_item_to_object(object, string, item, &global_hooks, true);
}

auto
cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item) -> CJSON_PUBLIC(cJSON_bool) {
    if (array == nullptr) {
        return false;
    }

    return add_item_to_array(array, create_reference(item, &global_hooks));
}

auto
cJSON_AddItemReferenceToObject(cJSON *object, const char *string, cJSON *item) -> CJSON_PUBLIC(cJSON_bool) {
    if ((object == nullptr) || (string == nullptr)) {
        return false;
    }

    return add_item_to_object(object, string, create_reference(item, &global_hooks), &global_hooks, false);
}

auto
cJSON_AddNullToObject(cJSON *const object, const char *const name) -> CJSON_PUBLIC(cJSON *) {
    cJSON *null = cJSON_CreateNull();
    if (add_item_to_object(object, name, null, &global_hooks, false)) {
        return null;
    }

    cJSON_Delete(null);
    return nullptr;
}

auto
cJSON_AddTrueToObject(cJSON *const object, const char *const name) -> CJSON_PUBLIC(cJSON *) {
    cJSON *true_item = cJSON_CreateTrue();
    if (add_item_to_object(object, name, true_item, &global_hooks, false)) {
        return true_item;
    }

    cJSON_Delete(true_item);
    return nullptr;
}

auto
cJSON_AddFalseToObject(cJSON *const object, const char *const name) -> CJSON_PUBLIC(cJSON *) {
    cJSON *false_item = cJSON_CreateFalse();
    if (add_item_to_object(object, name, false_item, &global_hooks, false)) {
        return false_item;
    }

    cJSON_Delete(false_item);
    return nullptr;
}

auto
cJSON_AddBoolToObject(cJSON *const object, const char *const name, const cJSON_bool boolean) -> CJSON_PUBLIC(cJSON *) {
    cJSON *bool_item = cJSON_CreateBool(boolean);
    if (add_item_to_object(object, name, bool_item, &global_hooks, false)) {
        return bool_item;
    }

    cJSON_Delete(bool_item);
    return nullptr;
}

auto
cJSON_AddNumberToObject(cJSON *const object, const char *const name, const double number) -> CJSON_PUBLIC(cJSON *) {
    cJSON *number_item = cJSON_CreateNumber(number);
    if (add_item_to_object(object, name, number_item, &global_hooks, false)) {
        return number_item;
    }

    cJSON_Delete(number_item);
    return nullptr;
}

auto
cJSON_AddStringToObject(cJSON *const object, const char *const name, const char *const string) -> CJSON_PUBLIC(cJSON *) {
    cJSON *string_item = cJSON_CreateString(string);
    if (add_item_to_object(object, name, string_item, &global_hooks, false)) {
        return string_item;
    }

    cJSON_Delete(string_item);
    return nullptr;
}

auto
cJSON_AddRawToObject(cJSON *const object, const char *const name, const char *const raw) -> CJSON_PUBLIC(cJSON *) {
    cJSON *raw_item = cJSON_CreateRaw(raw);
    if (add_item_to_object(object, name, raw_item, &global_hooks, false)) {
        return raw_item;
    }

    cJSON_Delete(raw_item);
    return nullptr;
}

auto
cJSON_AddObjectToObject(cJSON *const object, const char *const name) -> CJSON_PUBLIC(cJSON *) {
    cJSON *object_item = cJSON_CreateObject();
    if (add_item_to_object(object, name, object_item, &global_hooks, false)) {
        return object_item;
    }

    cJSON_Delete(object_item);
    return nullptr;
}

auto
cJSON_AddArrayToObject(cJSON *const object, const char *const name) -> CJSON_PUBLIC(cJSON *) {
    cJSON *array = cJSON_CreateArray();
    if (add_item_to_object(object, name, array, &global_hooks, false)) {
        return array;
    }

    cJSON_Delete(array);
    return nullptr;
}

auto
cJSON_DetachItemViaPointer(cJSON *parent, cJSON *const item) -> CJSON_PUBLIC(cJSON *) {
    if ((parent == nullptr) || (item == nullptr)) {
        return nullptr;
    }

    if (item != parent->child) {
        /* not the first element */
        item->prev->next = item->next;
    }
    if (item->next != nullptr) {
        /* not the last element */
        item->next->prev = item->prev;
    }

    if (item == parent->child) {
        /* first element */
        parent->child = item->next;
    } else if (item->next == nullptr) {
        /* last element */
        parent->child->prev = item->prev;
    }

    /* make sure the detached item doesn't point anywhere anymore */
    item->prev = nullptr;
    item->next = nullptr;

    return item;
}

auto
cJSON_DetachItemFromArray(cJSON *array, int which) -> CJSON_PUBLIC(cJSON *) {
    if (which < 0) {
        return nullptr;
    }

    return cJSON_DetachItemViaPointer(array, get_array_item(array, (size_t) which));
}

CJSON_PUBLIC(void)
cJSON_DeleteItemFromArray(cJSON *array, int which) {
    cJSON_Delete(cJSON_DetachItemFromArray(array, which));
}

auto
cJSON_DetachItemFromObject(cJSON *object, const char *string) -> CJSON_PUBLIC(cJSON *) {
    cJSON *to_detach = cJSON_GetObjectItem(object, string);

    return cJSON_DetachItemViaPointer(object, to_detach);
}

auto
cJSON_DetachItemFromObjectCaseSensitive(cJSON *object, const char *string) -> CJSON_PUBLIC(cJSON *) {
    cJSON *to_detach = cJSON_GetObjectItemCaseSensitive(object, string);

    return cJSON_DetachItemViaPointer(object, to_detach);
}

CJSON_PUBLIC(void)
cJSON_DeleteItemFromObject(cJSON *object, const char *string) {
    cJSON_Delete(cJSON_DetachItemFromObject(object, string));
}

CJSON_PUBLIC(void)
cJSON_DeleteItemFromObjectCaseSensitive(cJSON *object, const char *string) {
    cJSON_Delete(cJSON_DetachItemFromObjectCaseSensitive(object, string));
}

/* Replace array/object items with new ones. */
auto
cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem) -> CJSON_PUBLIC(cJSON_bool) {
    cJSON *after_inserted = nullptr;

    if (which < 0) {
        return false;
    }

    after_inserted = get_array_item(array, (size_t) which);
    if (after_inserted == nullptr) {
        return add_item_to_array(array, newitem);
    }

    newitem->next = after_inserted;
    newitem->prev = after_inserted->prev;
    after_inserted->prev = newitem;
    if (after_inserted == array->child) {
        array->child = newitem;
    } else {
        newitem->prev->next = newitem;
    }
    return true;
}

auto
cJSON_ReplaceItemViaPointer(cJSON *const parent, cJSON *const item, cJSON *replacement) -> CJSON_PUBLIC(cJSON_bool) {
    if ((parent == nullptr) || (replacement == nullptr) || (item == nullptr)) {
        return false;
    }

    if (replacement == item) {
        return true;
    }

    replacement->next = item->next;
    replacement->prev = item->prev;

    if (replacement->next != nullptr) {
        replacement->next->prev = replacement;
    }
    if (parent->child == item) {
        if (parent->child->prev == parent->child) {
            replacement->prev = replacement;
        }
        parent->child = replacement;
    } else { /*
         * To find the last item in array quickly, we use prev in array.
         * We can't modify the last item's next pointer where this item was the parent's child
         */
        if (replacement->prev != nullptr) {
            replacement->prev->next = replacement;
        }
        if (replacement->next == nullptr) {
            parent->child->prev = replacement;
        }
    }

    item->next = nullptr;
    item->prev = nullptr;
    cJSON_Delete(item);

    return true;
}

auto
cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem) -> CJSON_PUBLIC(cJSON_bool) {
    if (which < 0) {
        return false;
    }

    return cJSON_ReplaceItemViaPointer(array, get_array_item(array, (size_t) which), newitem);
}

static auto
replace_item_in_object(cJSON *object, const char *string, cJSON *replacement, cJSON_bool case_sensitive) -> cJSON_bool {
    if ((replacement == nullptr) || (string == nullptr)) {
        return false;
    }

    /* replace the name in the replacement */
    if (!(replacement->type & cJSON_StringIsConst) && (replacement->string != nullptr)) {
        cJSON_free(replacement->string);
    }
    replacement->string = (char *) cJSON_strdup((const unsigned char *) string, &global_hooks);
    replacement->type &= ~cJSON_StringIsConst;

    return cJSON_ReplaceItemViaPointer(object, get_object_item(object, string, case_sensitive), replacement);
}

auto
cJSON_ReplaceItemInObject(cJSON *object, const char *string, cJSON *newitem) -> CJSON_PUBLIC(cJSON_bool) {
    return replace_item_in_object(object, string, newitem, false);
}

auto
cJSON_ReplaceItemInObjectCaseSensitive(cJSON *object, const char *string, cJSON *newitem) -> CJSON_PUBLIC(cJSON_bool) {
    return replace_item_in_object(object, string, newitem, true);
}

/* Create basic types: */
auto
cJSON_CreateNull(void) -> CJSON_PUBLIC(cJSON *) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_NULL;
    }

    return item;
}

auto
cJSON_CreateTrue(void) -> CJSON_PUBLIC(cJSON *) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_True;
    }

    return item;
}

auto
cJSON_CreateFalse(void) -> CJSON_PUBLIC(cJSON *) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_False;
    }

    return item;
}

auto
cJSON_CreateBool(cJSON_bool boolean) -> CJSON_PUBLIC(cJSON *) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = boolean ? cJSON_True : cJSON_False;
    }

    return item;
}

auto
cJSON_CreateNumber(double num) -> CJSON_PUBLIC(cJSON *) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_Number;
        item->valuedouble = num;

        /* use saturation in case of overflow */
        if (num >= INT_MAX) {
            item->valueint = INT_MAX;
        } else if (num <= (double) INT_MIN) {
            item->valueint = INT_MIN;
        } else {
            item->valueint = (int) num;
        }
    }

    return item;
}

auto
cJSON_CreateString(const char *string) -> CJSON_PUBLIC(cJSON *) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_String;
        item->valuestring = (char *) cJSON_strdup((const unsigned char *) string, &global_hooks);
        if (!item->valuestring) {
            cJSON_Delete(item);
            return nullptr;
        }
    }

    return item;
}

auto
cJSON_CreateStringReference(const char *string) -> CJSON_PUBLIC(cJSON *) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item != nullptr) {
        item->type = cJSON_String | cJSON_IsReference;
        item->valuestring = (char *) cast_away_const(string);
    }

    return item;
}

auto
cJSON_CreateObjectReference(const cJSON *child) -> CJSON_PUBLIC(cJSON *) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item != nullptr) {
        item->type = cJSON_Object | cJSON_IsReference;
        item->child = (cJSON *) cast_away_const(child);
    }

    return item;
}

auto
cJSON_CreateArrayReference(const cJSON *child) -> CJSON_PUBLIC(cJSON *) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item != nullptr) {
        item->type = cJSON_Array | cJSON_IsReference;
        item->child = (cJSON *) cast_away_const(child);
    }

    return item;
}

auto
cJSON_CreateRaw(const char *raw) -> CJSON_PUBLIC(cJSON *) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_Raw;
        item->valuestring = (char *) cJSON_strdup((const unsigned char *) raw, &global_hooks);
        if (!item->valuestring) {
            cJSON_Delete(item);
            return nullptr;
        }
    }

    return item;
}

auto
cJSON_CreateArray(void) -> CJSON_PUBLIC(cJSON *) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_Array;
    }

    return item;
}

auto
cJSON_CreateObject(void) -> CJSON_PUBLIC(cJSON *) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_Object;
    }

    return item;
}

/* Create Arrays: */
auto
cJSON_CreateIntArray(const int *numbers, int count) -> CJSON_PUBLIC(cJSON *) {
    size_t i = 0;
    cJSON *n = nullptr;
    cJSON *p = nullptr;
    cJSON *a = nullptr;

    if ((count < 0) || (numbers == nullptr)) {
        return nullptr;
    }

    a = cJSON_CreateArray();

    for (i = 0; a && (i < (size_t) count); i++) {
        n = cJSON_CreateNumber(numbers[i]);
        if (!n) {
            cJSON_Delete(a);
            return nullptr;
        }
        if (!i) {
            a->child = n;
        } else {
            suffix_object(p, n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

auto
cJSON_CreateFloatArray(const float *numbers, int count) -> CJSON_PUBLIC(cJSON *) {
    size_t i = 0;
    cJSON *n = nullptr;
    cJSON *p = nullptr;
    cJSON *a = nullptr;

    if ((count < 0) || (numbers == nullptr)) {
        return nullptr;
    }

    a = cJSON_CreateArray();

    for (i = 0; a && (i < (size_t) count); i++) {
        n = cJSON_CreateNumber((double) numbers[i]);
        if (!n) {
            cJSON_Delete(a);
            return nullptr;
        }
        if (!i) {
            a->child = n;
        } else {
            suffix_object(p, n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

auto
cJSON_CreateDoubleArray(const double *numbers, int count) -> CJSON_PUBLIC(cJSON *) {
    size_t i = 0;
    cJSON *n = nullptr;
    cJSON *p = nullptr;
    cJSON *a = nullptr;

    if ((count < 0) || (numbers == nullptr)) {
        return nullptr;
    }

    a = cJSON_CreateArray();

    for (i = 0; a && (i < (size_t) count); i++) {
        n = cJSON_CreateNumber(numbers[i]);
        if (!n) {
            cJSON_Delete(a);
            return nullptr;
        }
        if (!i) {
            a->child = n;
        } else {
            suffix_object(p, n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

auto
cJSON_CreateStringArray(const char *const *strings, int count) -> CJSON_PUBLIC(cJSON *) {
    size_t i = 0;
    cJSON *n = nullptr;
    cJSON *p = nullptr;
    cJSON *a = nullptr;

    if ((count < 0) || (strings == nullptr)) {
        return nullptr;
    }

    a = cJSON_CreateArray();

    for (i = 0; a && (i < (size_t) count); i++) {
        n = cJSON_CreateString(strings[i]);
        if (!n) {
            cJSON_Delete(a);
            return nullptr;
        }
        if (!i) {
            a->child = n;
        } else {
            suffix_object(p, n);
        }
        p = n;
    }

    if (a && a->child) {
        a->child->prev = n;
    }

    return a;
}

/* Duplication */
auto
cJSON_Duplicate(const cJSON *item, cJSON_bool recurse) -> CJSON_PUBLIC(cJSON *) {
    cJSON *newitem = nullptr;
    cJSON *child = nullptr;
    cJSON *next = nullptr;
    cJSON *newchild = nullptr;

    /* Bail on bad ptr */
    if (!item) {
        goto fail;
    }
    /* Create new item */
    newitem = cJSON_New_Item(&global_hooks);
    if (!newitem) {
        goto fail;
    }
    /* Copy over all vars */
    newitem->type = item->type & (~cJSON_IsReference);
    newitem->valueint = item->valueint;
    newitem->valuedouble = item->valuedouble;
    if (item->valuestring) {
        newitem->valuestring = (char *) cJSON_strdup((unsigned char *) item->valuestring, &global_hooks);
        if (!newitem->valuestring) {
            goto fail;
        }
    }
    if (item->string) {
        newitem->string = (item->type & cJSON_StringIsConst) ? item->string : (char *) cJSON_strdup(
                (unsigned char *) item->string, &global_hooks);
        if (!newitem->string) {
            goto fail;
        }
    }
    /* If non-recursive, then we're done! */
    if (!recurse) {
        return newitem;
    }
    /* Walk the ->next chain for the child. */
    child = item->child;
    while (child != nullptr) {
        newchild = cJSON_Duplicate(child, true); /* Duplicate (with recurse) each item in the ->next chain */
        if (!newchild) {
            goto fail;
        }
        if (next != nullptr) {
            /* If newitem->child already set, then crosswire ->prev and ->next and move on */
            next->next = newchild;
            newchild->prev = next;
            next = newchild;
        } else {
            /* Set newitem->child and move to it */
            newitem->child = newchild;
            next = newchild;
        }
        child = child->next;
    }
    if (newitem && newitem->child) {
        newitem->child->prev = newchild;
    }

    return newitem;

    fail:
    if (newitem != nullptr) {
        cJSON_Delete(newitem);
    }

    return nullptr;
}

static void skip_oneline_comment(char **input) {
    *input += static_strlen("//");

    for (; (*input)[0] != '\0'; ++(*input)) {
        if ((*input)[0] == '\n') {
            *input += static_strlen("\n");
            return;
        }
    }
}

static void skip_multiline_comment(char **input) {
    *input += static_strlen("/*");

    for (; (*input)[0] != '\0'; ++(*input)) {
        if (((*input)[0] == '*') && ((*input)[1] == '/')) {
            *input += static_strlen("*/");
            return;
        }
    }
}

static void minify_string(char **input, char **output) {
    (*output)[0] = (*input)[0];
    *input += static_strlen("\"");
    *output += static_strlen("\"");


    for (; (*input)[0] != '\0'; (void) ++(*input), ++(*output)) {
        (*output)[0] = (*input)[0];

        if ((*input)[0] == '\"') {
            (*output)[0] = '\"';
            *input += static_strlen("\"");
            *output += static_strlen("\"");
            return;
        } else if (((*input)[0] == '\\') && ((*input)[1] == '\"')) {
            (*output)[1] = (*input)[1];
            *input += static_strlen("\"");
            *output += static_strlen("\"");
        }
    }
}

CJSON_PUBLIC(void)
cJSON_Minify(char *json) {
    char *into = json;

    if (json == nullptr) {
        return;
    }

    while (json[0] != '\0') {
        switch (json[0]) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                json++;
                break;

            case '/':
                if (json[1] == '/') {
                    skip_oneline_comment(&json);
                } else if (json[1] == '*') {
                    skip_multiline_comment(&json);
                } else {
                    json++;
                }
                break;

            case '\"':
                minify_string(&json, (char **) &into);
                break;

            default:
                into[0] = json[0];
                json++;
                into++;
        }
    }

    /* and null-terminate. */
    *into = '\0';
}

auto
cJSON_IsInvalid(const cJSON *const item) -> CJSON_PUBLIC(cJSON_bool) {
    if (item == nullptr) {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Invalid;
}

auto
cJSON_IsFalse(const cJSON *const item) -> CJSON_PUBLIC(cJSON_bool) {
    if (item == nullptr) {
        return false;
    }

    return (item->type & 0xFF) == cJSON_False;
}

auto
cJSON_IsTrue(const cJSON *const item) -> CJSON_PUBLIC(cJSON_bool) {
    if (item == nullptr) {
        return false;
    }

    return (item->type & 0xff) == cJSON_True;
}


auto
cJSON_IsBool(const cJSON *const item) -> CJSON_PUBLIC(cJSON_bool) {
    if (item == nullptr) {
        return false;
    }

    return (item->type & (cJSON_True | cJSON_False)) != 0;
}

auto
cJSON_IsNull(const cJSON *const item) -> CJSON_PUBLIC(cJSON_bool) {
    if (item == nullptr) {
        return false;
    }

    return (item->type & 0xFF) == cJSON_NULL;
}

auto
cJSON_IsNumber(const cJSON *const item) -> CJSON_PUBLIC(cJSON_bool) {
    if (item == nullptr) {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Number;
}

auto
cJSON_IsString(const cJSON *const item) -> CJSON_PUBLIC(cJSON_bool) {
    if (item == nullptr) {
        return false;
    }

    return (item->type & 0xFF) == cJSON_String;
}

auto
cJSON_IsArray(const cJSON *const item) -> CJSON_PUBLIC(cJSON_bool) {
    if (item == nullptr) {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Array;
}

auto
cJSON_IsObject(const cJSON *const item) -> CJSON_PUBLIC(cJSON_bool) {
    if (item == nullptr) {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Object;
}

auto
cJSON_IsRaw(const cJSON *const item) -> CJSON_PUBLIC(cJSON_bool) {
    if (item == nullptr) {
        return false;
    }

    return (item->type & 0xFF) == cJSON_Raw;
}

auto
cJSON_Compare(const cJSON *const a, const cJSON *const b, const cJSON_bool case_sensitive) -> CJSON_PUBLIC(cJSON_bool) {
    if ((a == nullptr) || (b == nullptr) || ((a->type & 0xFF) != (b->type & 0xFF))) {
        return false;
    }

    /* check if type is valid */
    switch (a->type & 0xFF) {
        case cJSON_False:
        case cJSON_True:
        case cJSON_NULL:
        case cJSON_Number:
        case cJSON_String:
        case cJSON_Raw:
        case cJSON_Array:
        case cJSON_Object:
            break;

        default:
            return false;
    }

    /* identical objects are equal */
    if (a == b) {
        return true;
    }

    switch (a->type & 0xFF) {
        /* in these cases and equal type is enough */
        case cJSON_False:
        case cJSON_True:
        case cJSON_NULL:
            return true;

        case cJSON_Number:
            if (compare_double(a->valuedouble, b->valuedouble)) {
                return true;
            }
            return false;

        case cJSON_String:
        case cJSON_Raw:
            if ((a->valuestring == nullptr) || (b->valuestring == nullptr)) {
                return false;
            }
            if (strcmp(a->valuestring, b->valuestring) == 0) {
                return true;
            }

            return false;

        case cJSON_Array: {
            cJSON *a_element = a->child;
            cJSON *b_element = b->child;

            for (; (a_element != nullptr) && (b_element != nullptr);) {
                if (!cJSON_Compare(a_element, b_element, case_sensitive)) {
                    return false;
                }

                a_element = a_element->next;
                b_element = b_element->next;
            }

            /* one of the arrays is longer than the other */
            if (a_element != b_element) {
                return false;
            }

            return true;
        }

        case cJSON_Object: {
            cJSON *a_element = nullptr;
            cJSON *b_element = nullptr;
            cJSON_ArrayForEach(a_element, a) {
                /* TODO This has O(n^2) runtime, which is horrible! */
                b_element = get_object_item(b, a_element->string, case_sensitive);
                if (b_element == nullptr) {
                    return false;
                }

                if (!cJSON_Compare(a_element, b_element, case_sensitive)) {
                    return false;
                }
            }

            /* doing this twice, once on a and b to prevent true comparison if a subset of b
             * TODO: Do this the proper way, this is just a fix for now */
            cJSON_ArrayForEach(b_element, b) {
                a_element = get_object_item(a, b_element->string, case_sensitive);
                if (a_element == nullptr) {
                    return false;
                }

                if (!cJSON_Compare(b_element, a_element, case_sensitive)) {
                    return false;
                }
            }

            return true;
        }

        default:
            return false;
    }
}

auto
cJSON_malloc(size_t size) -> CJSON_PUBLIC(void *) {
    return global_hooks.allocate(size);
}

CJSON_PUBLIC(void)
cJSON_free(void *object) {
    global_hooks.deallocate(object);
}
