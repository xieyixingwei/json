#include "json.h"
#include <string.h>
#include <stdio.h>

#define PTR_IS_NOT_BLANK(ptr) \
    (*(ptr) != ' ' && *(ptr) != '\r' && *(ptr) != '\n' && *(ptr) != '\t')

#define PTR_IS_BLANK(ptr) \
    (*(ptr) == ' ' || *(ptr) == '\r' || *(ptr) == '\n' || *(ptr) == '\t')

#ifndef MIN
    #define MIN(a, b)   ((a) < (b) ? (a) : (b))
#endif

#ifndef START_WITH
    #define START_WITH(str, dest)   (0 == strncmp(str, dest, MIN(strlen(str), strlen(dest))))
#endif

//======================================================================================================================
// json decode
//======================================================================================================================
static char *skip_blank(char *vptr)
{
    while(PTR_IS_BLANK(vptr)) {
        vptr++;
    }
    return vptr;
}

static char *goto_obj_list_end(char *vptr, char *end, char leftChar, char rightChar)
{
    int flag = 0;
    do {
        if(*vptr == leftChar) {
            flag++;
        }
        else if(*vptr == rightChar) {
            flag--;
            if(0 == flag) {
                return vptr;
            }
        }
        vptr++;
    } while(vptr < end);
    return NULL;
}

static char *goto_value_end(char *vptr, char *end)
{
    if(*vptr == '{') {
        return goto_obj_list_end(vptr, end, '{', '}');
    }
    else if(*vptr == '[') {
        return goto_obj_list_end(vptr, end, '[', ']');
    }
    else if(*vptr == '\"') {
        while(++vptr < end) {
            if(*vptr == '\"' || *vptr == '\0') {
                return vptr;
            }
        }
    }
    else {
        while(++vptr < end) {
            if(*vptr == ',' || *vptr == ']' || *vptr == '}') {
                return vptr - 1;
            }
        }
    }
    return NULL;
}

static char *goto_next_item(char *vptr, char *end)
{
    while(vptr != NULL && vptr < end) {
        /* it must there is a , or ] or } at the back of value */
        if(*vptr == ',') {
            vptr += 1;
            return skip_blank(vptr);
        }
        else if(*vptr == ']' || *vptr == '}') {
            return vptr;
        }
        vptr++;
    }
    return NULL;
}

static bool cmp_key(const char *src, const char *key)
{
    char *ptr = strchr(key, '.');
    size_t len = ptr == NULL ? strlen(key) : (ptr - key);
    return 0 == strncmp(src, key, len);
}
static char *find_object_value(char *vptr, char *end, const char *key, char **keyStart, char **valueEnd)
{
    char *kStart = NULL;
    char *vStart = NULL;
    char *vEnd = NULL;
    /* object must be start with '{' */
    if(vptr == NULL || *vptr++ != '{') {
        return NULL;
    }
    /* skip the front blank at key */
    vptr = skip_blank(vptr);
    /* empty obj */
    if(*vptr == '}') {
        if(valueEnd != NULL) {
            /* valueEnd points to the end of value if don't find */
            *valueEnd = vptr;
        }
        return NULL;
    }
    while(vptr < end) {
        /* object key must start with '\"' */
        kStart = vptr;
        if(*vptr++ != '\"') {
            return NULL;
        }
        /* foreach the key content */
        while(*vptr != '\0') {
            /* object key must end with '\"' and can't contains '\"' char */
            if(*vptr++ == '\"') {
                break;
            }
        }
        vptr = skip_blank(vptr);
        /* the separator ':' must be between key with value */
        if(*vptr++ != ':') {
            return NULL;
        }
        /* skip the front blank of value */
        vptr = skip_blank(vptr);
        vStart = vptr;
        vptr = goto_value_end(vptr, end);
        vEnd = vptr;
        if(key != NULL && cmp_key(kStart + 1, key)) {
            if(keyStart != NULL) {
                *keyStart = kStart;
            }
            if(valueEnd != NULL) {
                *valueEnd = vEnd;
            }
            return vStart;
        }
        vptr = goto_next_item(vptr + 1, end);
        if(*vptr == '}') {
            if(valueEnd != NULL) {
                /* valueEnd points to the end of value if don't find */
                *valueEnd = vptr;
            }
            break;
        }
    }
    return NULL;
}

static char *find_list_value(char *vptr, char *end, const char *key, char **valueEnd)
{
    char *start = vptr;
    const long index = (0 == strlen(key)) ? -1 : strtol(key, NULL, 10);
    /* list must be start with '[' */
    if(vptr == NULL || *vptr++ != '[') {
        return NULL;
    }
    vptr = skip_blank(vptr);
    /* list is empty */
    if(*vptr == ']') {
        if(valueEnd != NULL) {
            /* valueEnd points to the value end if don't find */
            *valueEnd = vptr;
        }
        /* return the list value start if the key is empty */
        return index == -1 ? start : NULL;
    }
    long i = 0;
    while(vptr < end) {
        if(i == index) {
            if(valueEnd != NULL) {
                *valueEnd = goto_value_end(vptr, end);
            }
            return vptr;
        }
        vptr = goto_value_end(vptr, end);
        vptr = goto_next_item(vptr + 1, end);
        i++;
        if(*vptr == ']') {
            if(valueEnd != NULL) {
                /* valueEnd points to the value end if don't find */
                *valueEnd = vptr;
            }
            /* return the list value start if the key is empty */
            return index == -1 ? start : NULL;
        }
    }
    return NULL;
}

char *get_kv(char *vptr, char *end, const char *key, char **keyStart, char **valueEnd)
{
    /* key maybe is "student.name" or "name" or "2" or "subjects.0" */
    /* skip the front blank */
    vptr = skip_blank(vptr);
    while(vptr != NULL && vptr < end) {
        /* find the key in json */
        if(*vptr == '{') {
            vptr = find_object_value(vptr, end, key, keyStart, valueEnd);
        }
        else if(*vptr == '[') {
            vptr = find_list_value(vptr, end, key, valueEnd);
        }
        else {
            return NULL;
        }
        if(NULL == vptr) {
            return NULL;
        }
        char *point = strchr(key, '.');
        if(NULL == point) {
            return vptr;
        }
        else {
            key = point + 1;
        }
    }
}

static bool parse_general_value(char *vptr, char *end, void *value)
{
    if(vptr == NULL) {
        return false;
    }
    else if(START_WITH(vptr, "false") || START_WITH(vptr, "FALSE")) {
        *(bool *)value = false;
    }
    else if(START_WITH(vptr, "true") || START_WITH(vptr, "TRUE")) {
        *(bool *)value = true;
    }
    else if(START_WITH(vptr, "0x") || START_WITH(vptr, "0X")) {
        *(uint32_t *)value = strtoul(vptr, NULL, 16);
    }
    else if(START_WITH(vptr, "\"")) {
        vptr += 1;
        if(*vptr == '\"') {
            *(char **)value = "";
        }
        else {
            *(char **)value = vptr;
            while(*vptr != '\0' && vptr < end) {
                if(*vptr == '\"') {
                    *vptr = '\0';
                    break;
                }
                vptr++;
            }
        }
    }
    else {
       *(long *)value = strtol(vptr, NULL, 10);
    }
    return true;
}

static size_t parse_list_value(char *vptr, char *end, size_t index, void *value)
{
    size_t count = 0;
    /* list must be start with '[' */
    if(vptr == NULL || *vptr++ != '[') {
        return count;
    }
    vptr = skip_blank(vptr);
    /* list is empty */
    if(*vptr == ']') {
        return count; 
    }
    while(vptr != NULL && vptr < end) {
        if(value != NULL && count == index) {
            parse_general_value(vptr, end, value);
            count++;
            break;
        }
        vptr = goto_value_end(vptr, end);
        vptr = goto_next_item(vptr + 1, end);
        count++;
        if(*vptr == ']') {
            break;
        }
    }
    return count;
}

static bool json_decode_value(json_t *json, const char *key, void *value)
{
    char *vptr = get_kv(json->buf, json->end, key, NULL, NULL);
    return parse_general_value(vptr, json->end, value);
}

static long get_long(json_t *json, const char *key, long deft)
{
    long value = deft;
    json_decode_value(json, key, &value);
    return value;
}

static bool get_bool(json_t *json, const char *key, bool deft)
{
    bool value = deft;
    json_decode_value(json, key, &value);
    return value;
}

static const char* get_str(json_t *json, const char *key)
{
    char *value = NULL;
    json_decode_value(json, key, &value);
    return value;
}

static bool is_null(json_t *json, const char *key)
{
    char *vptr = get_kv(json->buf, json->end, key, NULL, NULL);
    if(vptr == NULL || START_WITH(vptr, "null") || START_WITH(vptr, "NULL")) {
        return true;
    }
    return false;
}

static size_t get_list_count(json_t *json, const char *key)
{
    char *vptr = get_kv(json->buf, json->end, key, NULL, NULL);
    return parse_list_value(vptr, json->end, 0, NULL);
}

static size_t get_list_long(json_t *json, const char *key, long *buf, size_t n)
{
    char *vptr = get_kv(json->buf, json->end, key, NULL, NULL);
    size_t i;
    for(i = 0; i < n; i++) {
        if((i + 1) != parse_list_value(vptr, json->end, i, &buf[i])) {
            break;
        }
    }
    return i;
}

static size_t get_list_byte(json_t *json, const char *key, uint8_t *buf, size_t n)
{
    char *vptr = get_kv(json->buf, json->end, key, NULL, NULL);
    size_t i;
    for(i = 0; i < n; i++) {
        if((i + 1) != parse_list_value(vptr, json->end, i, &buf[i])) {
            break;
        }
    }
    return i;
}

static size_t get_list_str(json_t *json, const char *key, const char **buf, size_t n)
{
    char *vptr = get_kv(json->buf, json->end, key, NULL, NULL);
    size_t i;
    for(i = 0; i < n; i++) {
        if((i + 1) != parse_list_value(vptr, json->end, i, &buf[i])) {
            break;
        }
    }
    return i;
}

static long get_list_long_of(json_t *json, const char *key, size_t index)
{
    char *vptr = get_kv(json->buf, json->end, key, NULL, NULL);
    long value = 0;
    parse_list_value(vptr, json->end, index, &value);
    return value;
}

static const char* get_list_str_of(json_t *json, const char *key, size_t index)
{
    char *vptr = get_kv(json->buf, json->end, key, NULL, NULL);
    const char *value = NULL;
    parse_list_value(vptr, json->end, index, &value);
    return value;
}

static const json_decode_op_t sgJsonDecodeOp = {
    .get_long = get_long,
    .get_bool = get_bool,
    .get_str = get_str,
    .is_null = is_null,
    .get_list_count = get_list_count,
    .get_list_long = get_list_long,
    .get_list_byte = get_list_byte,
    .get_list_str = get_list_str,
    .get_list_long_of = get_list_long_of,
    .get_list_str_of = get_list_str_of
};

//======================================================================================================================
// json encode
//======================================================================================================================
#define JSON_TYPE_BOOL      0
#define JSON_TYPE_LONG      1
#define JSON_TYPE_STR       2
#define JSON_TYPE_LIST      3

char* insert_str(char *dst, const char *src, size_t length)
{
    const size_t n = length > 0 ? length : strlen(src);
    const size_t len = strlen(dst);
    char *tmp = ((char *)dst) + len;
    memcpy(tmp, src, n);
    memcpy(tmp + n, dst, len);
    memcpy(dst, tmp, n);
    memcpy(dst + n, tmp + n, len);
    memset(dst + n + len, 0, 1);
    return dst + n;
}

static bool is_index(const char *key)
{
    while(*key != '\0' && *key != '.') {
        if(*key < '0' || *key > '9') {
            return false;
        }
        key++;
    }
    return true;
}

static char *insert_comma(char *vptr)
{
    char *ptr = vptr - 1;
    while(*ptr != '\0') {
        if(*ptr == '{' || *ptr == '[') {
            break;
        }
        else if(PTR_IS_NOT_BLANK(ptr)) {
            vptr = insert_str(vptr, ",", 0);
            break;
        }
        ptr--;
    }
    return vptr;
}

static char *insert_key(char *vptr, const char *key)
{
    vptr = insert_comma(vptr);
    char *ptr = strchr(key, '.');
    const size_t len = ptr != NULL ? (ptr - key) : strlen(key);
    vptr = insert_str(vptr, "\"", 0);
    vptr = insert_str(vptr, key, len);
    return insert_str(vptr, "\":", 0);
}

static char *insert_value(char *vptr, const void *value, uint8_t type)
{
    if(value == NULL) {
        vptr = insert_str(vptr, "null", 0);
    }
    else if(type == JSON_TYPE_LONG) {
        char intStr[16] = { 0 };
        sprintf(intStr, "%ld", *(long *)value);
        vptr = insert_str(vptr, intStr, 0);
    }
    else if(type == JSON_TYPE_BOOL) {
        vptr = insert_str(vptr, *(bool *)value ? "true" : "false", 0);
    }
    else if(type == JSON_TYPE_STR) {
        vptr = insert_str(vptr, "\"", 0);
        vptr = insert_str(vptr, (char *)value, 0);
        vptr = insert_str(vptr, "\"", 0);
    }
    return vptr;
}

static char *insert_kv(char *vptr, const char *key, const void *value, uint8_t type)
{
    if(!is_index(key)) {
        vptr = insert_key(vptr, key);
    }
    char *point = strchr(key, '.');
    if(point != NULL) {
        if(is_index(point + 1)) {
            insert_str(vptr, "[]", 0);
        }
        else {
            insert_str(vptr, "{}", 0);
        }
    }
    else {
        if(type == JSON_TYPE_LIST) {
            insert_str(vptr, "[]", 0);
        }
        else {
            if(is_index(key) && *(vptr - 1) != '[') {
                vptr = insert_str(vptr, ",", 0);
            }
            insert_value(vptr, value, type);
        }
    }
    return vptr;
}

static void set_key_value(json_t *json, const char *key, const void *value, uint8_t type)
{
    /* key maybe is "student.name" or "name" or "2" or "subjects.0" */
    char *keyStart = NULL;
    char *valueEnd = NULL;
    char *vptr = json->buf;
    if(0 == strlen(vptr)) {
        insert_str(vptr, is_index(key) ? "[]" : "{}", 0);
    }
    /* skip the front blank */
    vptr = skip_blank(vptr);
    while(vptr != NULL && vptr < json->end) {
        if(*vptr == '{') {
            vptr = find_object_value(vptr, json->end, key, &keyStart, &valueEnd);
        }
        else if(*vptr == '[') {
            vptr = find_list_value(vptr, json->end, key, &valueEnd);
        }
        else {
            return;
        }
        if(vptr == NULL) {
            vptr = insert_kv(valueEnd, key, value, type);
        }
        const char *point = strchr(key, '.');
        if(point == NULL) {
            return;
        }
        else {
            key = point + 1;
        }
    }
}

static void append_key_value(json_t *json, const char *key, const void *value, uint8_t type)
{
    char *valueEnd = NULL;
    if(0 == strlen(json->buf) && 0 == strlen(key)) {
        insert_str(json->buf, "[]", 0);
    }
    set_key_value(json, key, NULL, JSON_TYPE_LIST);
    char *vptr = get_kv(json->buf, json->end, key, NULL, &valueEnd);
    valueEnd = insert_comma(valueEnd);
    insert_value(valueEnd, value, type);
}

static bool last_is_index(const char *key)
{
    char *point = strrchr(key, '.');
    return  is_index(point != NULL ? point + 1 : key);
}

static void delete_kv(json_t *json, const char *key)
{
    char *keyStart = NULL;
    char *valueEnd = NULL;
    char* vptr = get_kv(json->buf, json->end, key, &keyStart, &valueEnd);
    if(vptr == NULL) {
        return;
    }
    char *start = last_is_index(key) ? vptr : keyStart;
    valueEnd = goto_next_item(valueEnd + 1, json->end);
    if(*(start - 1) == ',' && (*valueEnd == ']' || *valueEnd == '}')) {
        start -= 1;
    }
    memcpy(start, valueEnd, json->end - valueEnd);
}

static void set_long(json_t *json, const char *key, long value)
{
    delete_kv(json, key);
    set_key_value(json, key, &value, JSON_TYPE_LONG);
}

static void set_bool(json_t *json, const char *key, bool value)
{
    delete_kv(json, key);
    set_key_value(json, key, &value, JSON_TYPE_BOOL);
}

static void set_str(json_t *json, const char *key, const char *value)
{
    delete_kv(json, key);
    set_key_value(json, key, value, JSON_TYPE_STR);
}

static void append_list_long(json_t *json, const char *key, long value)
{
    append_key_value(json, key, &value, JSON_TYPE_LONG);
}

static void append_list_bool(json_t *json, const char *key, bool value)
{
    append_key_value(json, key, &value, JSON_TYPE_BOOL);
}

static void append_list_str(json_t *json, const char *key, const char *value)
{
    append_key_value(json, key, value, JSON_TYPE_STR);
}

static const json_encode_op_t sgJsonEncodeOp = {
    .set_long = set_long,
    .set_bool = set_bool,
    .set_str = set_str,
    .delete_key = delete_kv,
    .append_list_long = append_list_long,
    .append_list_bool = append_list_bool,
    .append_list_str = append_list_str
};

void json_init(json_t *json, char *buf, size_t size, bool clearbuf)
{
    if(clearbuf) {
        memset(buf, 0, size);
    }
    json->buf = buf;
    json->end = buf + size;
    json->encode = &sgJsonEncodeOp;
    json->decode = &sgJsonDecodeOp;
}
