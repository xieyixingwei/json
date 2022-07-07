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

#define JSON_DECODE_IS_VALID(json)         ((json) != NULL && (json)->count < ((json)->values_size))

#ifndef START_WITH
    #define START_WITH(str, dest)   (0 == strncmp(str, dest, MIN(strlen(str), strlen(dest))))
#endif

/**
 * @brief insert the str in ptr
 * 
 * @param ptr the insert point
 * @param str the inserted str
 * @return char* return the pointer: (ptr + strlen(str))
 */
static char* insert_str(char *ptr, const char *str)
{
    const size_t SIZE = strlen(str);
    const size_t SHIFT_SIZE = strlen(ptr);
    size_t i = 0, k = 0;
    char tmpA = 0;
    char tmpB = 0;
    char *tmpPtr = NULL;

    for(i = 0; i < SIZE; i++) {
        tmpPtr = ptr + i;
        tmpA = *tmpPtr;
        *tmpPtr = str[i];
        for(k = 1; k <= SHIFT_SIZE; k++) {
            tmpB = *(tmpPtr + k);
            *(tmpPtr + k) = tmpA;
            tmpA = tmpB;
        }
    }

    return ptr + i;
}

static char* insert_str_by_len(char *ptr, const char *str, size_t len)
{
    const size_t SHIFT_SIZE = strlen(ptr);
    size_t i = 0, k = 0;
    char tmpA = 0;
    char tmpB = 0;
    char *tmpPtr = NULL;

    for(i = 0; i < len; i++) {
        tmpPtr = ptr + i;
        tmpA = *tmpPtr;
        *tmpPtr = str[i];
        for(k = 1; k <= SHIFT_SIZE; k++) {
            tmpB = *(tmpPtr + k);
            *(tmpPtr + k) = tmpA;
            tmpA = tmpB;
        }
    }

    return ptr + i;
}

/**
 * @brief delete the str from start(include) to end(include)
 * 
 * @param start the start str
 * @param end the end str
 * @return char* return the start pointer
 */
static char* delete_str(char *start, char *end)
{
    char *ptrA = start;
    char *ptrB = end + 1;

    while(*ptrA != '\0') {
        *ptrA++ = *ptrB;
        if(*ptrB != '\0') {
            ptrB++;
        }
    }
    return start;
}

static uint32_t hash(const char* str, size_t len)
{
    #define HASH_SIZE   0xFFFF
    uint32_t h = 0;
    if(len == 0) {
        len = strlen(str);
    }
    for(size_t i = 0; i < len; i++) {
        h = str[i] + h*31;
    }
    return h % HASH_SIZE;
}

static uint32_t key_hash(const char* key)
{
    uint32_t keyhash = 0;
    char *dot = NULL;
    do {
        dot = strchr(key, '.');
        if(dot != NULL) {
            keyhash += hash(key, dot - key);
            key = dot + 1;
        }
        else {
            keyhash += hash(key, 0);
        }
    } while(dot != NULL);
    return keyhash;
}

static char* parse_int_bool_null_value(json_decode_t *json, uint32_t keyhash, char *vptr)
{
    if(JSON_DECODE_IS_VALID(json)) {
        json->values[json->count].type = JSON_TYPE_INT;
        json->values[json->count].keyhash = CUT_KEYHASH(keyhash);

        if(START_WITH(vptr, "false") || START_WITH(vptr, "FALSE")) {
            json->values[json->count].type = JSON_TYPE_BOOL;
            json->values[json->count++].value.vbool = false;
        }
        else if(START_WITH(vptr, "true") || START_WITH(vptr, "TRUE")) {
            json->values[json->count].type = JSON_TYPE_BOOL;
            json->values[json->count++].value.vbool = true;
        }
        else if(START_WITH(vptr, "null") || START_WITH(vptr, "NULL")) {
            json->values[json->count].type = JSON_TYPE_NULL;
            json->values[json->count++].value.vnull = 0;
        }
        else if(START_WITH(vptr, "0x") || START_WITH(vptr, "0X")) {
            json->values[json->count++].value.vint = (int)strtoul(vptr, NULL, 16);
        }
        else if (START_WITH(vptr, "-")) {
            vptr += 1;
            json->values[json->count++].value.vint = (int)(0 - (int)strtoul(vptr, NULL, 10));
        }
        else {
            json->values[json->count++].value.vint = (int)strtoul(vptr, NULL, 10);
        }
    }
    while(*(++vptr) != '\0') {
        /* the int or bool or null value must be end with , or ] or } */
        if(*vptr == ',') {
            return vptr + 1;
        }
        else if(*vptr == ']' || *vptr == '}') {
            return vptr;
        }
        else if(*vptr == '\"' || *vptr == '\0') {
            break;
        }
    }
    return NULL;
}

static char *goto_value_end(char *vptr)
{
    while(*(++vptr) != '\0') {
        /* it must there is a , or ] or } at the back of value */
        if(*vptr == ',') {
            return vptr + 1;
        }
        else if(*vptr == ']' || *vptr == '}') {
            return vptr;
        }
        else if(PTR_IS_NOT_BLANK(vptr)) {
            break;
        }
    }
    return NULL;
}

static char* parse_str_value(json_decode_t *json, uint32_t keyhash, char *vptr)
{
    /* str must be start with '\"' */
    if(*vptr++ != '\"') {
        return NULL;
    }
    if(JSON_DECODE_IS_VALID(json)) {
        json->values[json->count].keyhash = CUT_KEYHASH(keyhash);
        json->values[json->count].type = JSON_TYPE_STR;
        json->values[json->count++].value.vstr = vptr;
    }
    /* foreach the str value content */
    while(*vptr != '\0') {
        /* str must be end with '\"' and can't contains '\"' character */
        if(*vptr == '\"') {
            if(json != NULL) {
                *vptr = '\0';
            }
            break;
        }
        vptr++;
    }
    /* find the end of str value */
    return goto_value_end(vptr);
}

static char *skip_blank(char *vptr)
{
    while(PTR_IS_BLANK(vptr)) {
        vptr++;
    }
    return vptr;
}

static char *parse_value(json_decode_t *json, uint32_t keyhash, char *vptr);

static char *parse_list(json_decode_t *json, uint32_t keyhash, char *vptr)
{
    #define LIST_INDEX_SIZE    16
    char index[LIST_INDEX_SIZE] = {0};
    /* list must be start with '[' */
    if(*vptr++ != '[') {
        return NULL;
    }
    uint32_t list_count = 0;
    while(vptr != NULL && *vptr != '\0') {
        /* skip the blank at the front */
        vptr = skip_blank(vptr);
        /* list is not empty */
        if(*vptr != ']') {
            memset(index, 0, LIST_INDEX_SIZE);
            sprintf(index, "%u", list_count);
            vptr = parse_value(json, keyhash + hash(index, 0), vptr);
            list_count++;
        }
        if(*vptr == ']') {
            if(JSON_DECODE_IS_VALID(json)) {
                json->values[json->count].type = JSON_TYPE_LIST;
                json->values[json->count].keyhash = CUT_KEYHASH(keyhash);
                json->values[json->count++].value.vlist_count = list_count;
            }
            return goto_value_end(vptr);
        }
    }
    return NULL;
}

static char* parse_object(json_decode_t *json, uint32_t keyhash, char *vptr)
{
    /* object must be start with '{' */
    if(*vptr++ != '{') {
        return NULL;
    }
    /* skip the front blank at key */
    vptr = skip_blank(vptr);
    /* empty obj */
    if(*vptr == '}') {
        return goto_value_end(vptr);
    }
    while(vptr != NULL && *vptr != '\0') {
        /* skip the front blank at key */
        vptr = skip_blank(vptr);
        /* object key must start with '\"' */
        if(*vptr++ != '\"') {
            return NULL;
        }
        char *key = vptr;
        size_t keyLen = 0;
        /* foreach the key content */
        while(*vptr != '\0') {
            /* object key must end with '\"' and can't contains '\"' char */
            if(*vptr++ == '\"') {
                break;
            }
            keyLen++;
        }
        /* find the separator ':' between key with value */
        while(*vptr != '\0') {
            if(*vptr++ == ':') {
                break;
            }
            else if(PTR_IS_NOT_BLANK(vptr)) {
                return NULL;
            }
        }
        /* skip the front blank of value */
        vptr = skip_blank(vptr);
        /* object is not empty */
        if(*vptr != '}') {
            vptr = parse_value(json, keyhash + hash(key, keyLen), vptr);
        }
        if(*vptr == '}') {
            /* find the end of obj value */
            return goto_value_end(vptr);
        }
    }
    return NULL;
}

static char *parse_value(json_decode_t *json, uint32_t keyhash, char *vptr)
{
    if(*vptr == '\0') {
        return NULL;
    }
    else if(*vptr == '{') {
        vptr = parse_object(json, keyhash, vptr);
    }
    else if(*vptr == '[') {
        vptr = parse_list(json, keyhash, vptr);
    }
    else if(*vptr == '\"') {
        vptr = parse_str_value(json, keyhash, vptr);
    }
    else {
        vptr = parse_int_bool_null_value(json, keyhash, vptr);
    }
    return vptr;
}
#if 1
void json_decode_print(json_decode_t *json)
{
    for(uint32_t i = 0; i < json->count; i++) {
        if(json->values[i].type == JSON_TYPE_INT) {
            printf("type(int) keyhash[0x%x] = %d\r\n", CUT_KEYHASH(json->values[i].keyhash), json->values[i].value.vint);
        }
        if(json->values[i].type == JSON_TYPE_BOOL) {
            printf("type(bool) keyhash[0x%x] = %d\r\n", CUT_KEYHASH(json->values[i].keyhash), json->values[i].value.vbool);
        }
        else if(json->values[i].type == JSON_TYPE_STR) {
            printf("type(str) keyhash[0x%x] = %s\r\n", CUT_KEYHASH(json->values[i].keyhash), json->values[i].value.vstr);
        }
        else if(json->values[i].type == JSON_TYPE_NULL) {
            printf("type(null) keyhash[0x%x] = null\r\n", CUT_KEYHASH(json->values[i].keyhash));
        }
        else if(json->values[i].type == JSON_TYPE_LIST) {
            printf("type(list) keyhash[0x%x] count = %u\r\n", CUT_KEYHASH(json->values[i].keyhash), json->values[i].value.vlist_count);
        }
    }
}
#endif
static json_value_t* get_value(json_decode_t *json, const char *key)
{
    const uint32_t keyhash = key_hash(key);
    for(uint32_t i = 0; i < json->count; i++) {
        if(keyhash == CUT_KEYHASH(json->values[i].keyhash)) {
            return &(json->values[i]);
        }
    }
    return NULL;
}

static json_value_t* get_list_value(json_decode_t *json, const char *key, size_t index)
{
    #define INDEX_SIZE    12
    char indexStr[INDEX_SIZE] = {0};
    snprintf(indexStr, INDEX_SIZE, "%lu", index);
    const uint32_t keyhash = key_hash(key) + key_hash(indexStr);
    for(uint32_t i = 0; i < json->count; i++) {
        if(keyhash == CUT_KEYHASH(json->values[i].keyhash)) {
            return &(json->values[i]);
        }
    }
    return NULL;
}

static int get_int(json_decode_t *json, const char *key, int def)
{
    json_value_t *jv = get_value(json, key);
    if(jv == NULL || jv->type != JSON_TYPE_INT) {
        return def;
    }
    return jv->value.vint;
}

static int get_bool(json_decode_t *json, const char *key, int def)
{
    json_value_t *jv = get_value(json, key);
    if(jv == NULL || jv->type != JSON_TYPE_BOOL) {
        return def;
    }
    return jv->value.vbool;
}

static const char* get_str(json_decode_t *json, const char *key)
{
    json_value_t *jv = get_value(json, key);
    if(jv == NULL || jv->type != JSON_TYPE_STR) {
        return NULL;
    }
    return jv->value.vstr;
}

static bool is_null(json_decode_t *json, const char *key)
{
    json_value_t *jv = get_value(json, key);
    if(jv == NULL || jv->type != JSON_TYPE_NULL) {
        return false;
    }
    return true;
}

static size_t get_list_count(json_decode_t *json, const char *key)
{
    json_value_t *jv = get_value(json, key);
    if(jv == NULL || jv->type != JSON_TYPE_LIST) {
        return 0;
    }
    return jv->value.vlist_count;
}

static size_t get_list_int(json_decode_t *json, const char *key, int *buf, size_t n)
{
    const size_t count = MIN(n, get_list_count(json, key));
    size_t i = 0;
    for(i = 0; i < count; i++) {
        json_value_t *jv = get_list_value(json, key, i);
        if(jv == NULL || jv->type != JSON_TYPE_INT) {
            return 0;
        }
        buf[i] = jv->value.vint;
    }
    return i;
}

static size_t get_list_byte(json_decode_t *json, const char *key, uint8_t *buf, size_t n)
{
    const size_t count = MIN(n, get_list_count(json, key));
    size_t i = 0;
    for(i = 0; i < count; i++) {
        json_value_t *jv = get_list_value(json, key, i);
        if(jv == NULL || jv->type != JSON_TYPE_INT) {
            return 0;
        }
        buf[i] = (uint8_t)jv->value.vint;
    }
    return i;
}

static size_t get_list_str(json_decode_t *json, const char *key, const char **buf, size_t n)
{
    const size_t count = MIN(n, get_list_count(json, key));
    size_t i = 0;
    for(i = 0; i < count; i++) {
        json_value_t *jv = get_list_value(json, key, i);
        if(jv == NULL || jv->type != JSON_TYPE_STR) {
            return 0;
        }
        buf[i] = jv->value.vstr;
    }
    return i;
}

static int get_list_int_of(json_decode_t *json, const char *key, size_t index)
{
    json_value_t *jv = get_list_value(json, key, index);
    if(jv == NULL || jv->type != JSON_TYPE_INT) {
        return 0;
    }
    return jv->value.vint;
}

static const char* get_list_str_of(json_decode_t *json, const char *key, size_t index)
{
    json_value_t *jv = get_list_value(json, key, index);
    if(jv == NULL || jv->type != JSON_TYPE_STR) {
        return 0;
    }
    return jv->value.vstr;
}

static const json_decode_op_t gJsonDecodeOp = {
    .get_int = get_int,
    .get_bool = get_bool,
    .get_str = get_str,
    .is_null = is_null,
    .get_list_count = get_list_count,
    .get_list_int = get_list_int,
    .get_list_byte = get_list_byte,
    .get_list_str = get_list_str,
    .get_list_int_of = get_list_int_of,
    .get_list_str_of = get_list_str_of
};

void json_decode(json_decode_t *json, json_value_t *values, size_t n, char *jsonstr)
{
    char *vptr = jsonstr;
    memset(json, 0, sizeof(json_decode_t));
    json->op = &gJsonDecodeOp;
    json->values = values;
    json->values_size = n;
    /* skip the front blank */
    vptr = skip_blank(vptr);
    while(vptr != NULL && *vptr != '\0') {
        /* key maybe is class.student.name or class.student or class or student */
        /* find the key in json */
        if(*vptr == '{') {
            vptr = parse_object(json, 0, vptr);
        }
        else if(*vptr == '[') {
            vptr = parse_list(json, 0, vptr);
        }
        else {
            vptr++;
        }
    }
}
//======================================================================================================================
// json encode
//======================================================================================================================
static bool keycmp(const char *key, const char *objKey, size_t keyLen)
{
    for(size_t i = 0; i < keyLen; i++) {
        if(*key == '.' || *key == '\0') {
            return false;
        }
        if(*key++ != *objKey++) {
            return false;
        }
    }
    return true;
}

static char *find_list_index(char *vptr, uint32_t index, char **valueStart, char **valueEnd)
{
    /* list must be start with '[' */
    if(*vptr++ != '[') {
        return NULL;
    }
    uint32_t list_count = 0;
    while(vptr != NULL && *vptr != '\0') {
        /* skip the blank at the front */
        vptr = skip_blank(vptr);
        *valueStart = vptr;
        /* list is not empty */
        if(*vptr != ']') {
            vptr = parse_value(NULL, 0, vptr);
            *valueEnd = vptr;
        }
        else {
            *valueEnd = vptr;
        }
        if(index == list_count) {
            return *valueStart;
        }
        list_count++;
        if(*vptr == ']') {
            break;
        }
    }
    return NULL;
}

/**
 * return key start and value start and value end
*/
static char* find_object_key(char *vptr, const char *key, char **valueStart, char **valueEnd)
{
    char *keyStart = NULL;
    /* object must be start with '{' */
    if(*vptr++ != '{') {
        return NULL;
    }
    /* skip the front blank at key */
    vptr = skip_blank(vptr);
    /* empty obj */
    if(*vptr == '}') {
        *valueEnd = vptr;
        return NULL;
    }
    while(vptr != NULL && *vptr != '\0') {
        /* skip the front blank at key */
        vptr = skip_blank(vptr);
        keyStart = vptr;
        /* object key must start with '\"' */
        if(*vptr++ != '\"') {
            return NULL;
        }
        char *objKey = vptr;
        size_t keyLen = 0;
        /* foreach the key content */
        while(*vptr != '\0') {
            /* object key must end with '\"' and can't contains '\"' char */
            if(*vptr++ == '\"') {
                break;
            }
            keyLen++;
        }
        /* find the separator ':' between key with value */
        while(*vptr != '\0') {
            if(*vptr++ == ':') {
                break;
            }
            else if(PTR_IS_NOT_BLANK(vptr)) {
                return NULL;
            }
        }
        /* skip the front blank of value */
        vptr = skip_blank(vptr);
        *valueStart = vptr;
        /* object is not empty */
        if(*vptr != '}') {
            vptr = parse_value(NULL, 0, vptr);
            *valueEnd = vptr;
        }
        else {
            *valueEnd = vptr;
        }
        if(keycmp(key, objKey, keyLen)) {
            return keyStart;
        }
        if(*vptr == '}') {
            break;
        }
    }
    return NULL;
}

/**
 * return key start and value start and value end
*/
static char* find_key(char *vptr, const char *key, char **valueStart, char **valueEnd)
{
    char *keyStart = NULL;
    if(*vptr == '{') {
        keyStart = find_object_key(vptr, key, valueStart, valueEnd);
    }
    else if(*vptr == '[' && (key[0] >= '0' && key[0] <= '9')) {
        uint32_t index = strtoul(key, NULL, 10);
        keyStart = find_list_index(vptr, index, valueStart, valueEnd);
    }
    return keyStart;
}

static char *insert_front_comma(char *vptr)
{
    char *ptr = vptr;
    while(*(--ptr) != '\0') {
        if(*ptr == ',' || *ptr == '[' || *ptr == '{') {
            break;
        }
        else if(PTR_IS_NOT_BLANK(ptr)) {
            vptr = insert_str(vptr, ",");
            break;
        }
    }
    return vptr;
}

static void delete_front_comma(char *vptr)
{
    char *ptr = vptr;
    while(*(--ptr) != '\0') {
        if(*ptr == ',' && (*vptr == '}' || *vptr == ']')) {
            delete_str(ptr, ptr);
            return;
        }
        else if(PTR_IS_NOT_BLANK(vptr)) {
            break;
        }
    }
}

static char *goto_value_trail(char *valueEnd)
{
    char *ptr = valueEnd;
    while(*(--ptr) != '\0') {
        if(*ptr != ',' && PTR_IS_NOT_BLANK(ptr)) {
            return ptr;
        }
    }
    return valueEnd;
}

static char* insert_key(char *vptr, const char *key)
{
    char *ptr = strchr(key, '.');
    size_t keyLen = ptr != NULL ? ptr - key : strlen(key);
    vptr = insert_front_comma(vptr);
    vptr = insert_str(vptr, "\"");
    vptr = insert_str_by_len(vptr, key, keyLen);
    return insert_str(vptr, "\":");
}

static void insert_value(char *vptr, const void *value, uint8_t type)
{
    char intStr[16] = { 0 };
    if(value == NULL) {
        insert_str(vptr, "null");
    }
    else if(type == JSON_TYPE_INT) {
        sprintf(intStr, "%d", *(int *)value);
        insert_str(vptr, intStr);
    }
    else if(type == JSON_TYPE_BOOL) {
        insert_str(vptr, *(int *)value ? "true" : "false");
    }
    else if(type == JSON_TYPE_STR) {
        vptr = insert_str(vptr, "\"");
        vptr = insert_str(vptr, (char *)value);
        vptr = insert_str(vptr, "\"");
    }
}

static char *set_key(json_encode_t *json, const char *key, char **valueStart, char **valueEnd)
{
    char* keyStart = NULL;
    const char *kptr = key;
    char *vptr = json->jsonstr;
    if(strlen(vptr) == 0) {
        if(key[0] >= '0' &&  key[0] <= '9') {
            vptr[0] = '[';
            vptr[1] = ']';
        }
        else {
            vptr[0] = '{';
            vptr[1] = '}';
        }
    }
    while(kptr != NULL) {
        keyStart = find_key(vptr, kptr, valueStart, valueEnd);
        if(keyStart == NULL) {
            vptr = insert_key(*valueEnd, kptr);
            *valueStart = vptr;
            *valueEnd = vptr;
            if(strchr(kptr, '.') != NULL) {
                char c = *(strchr(kptr, '.') + 1);
                if(c >= '0' && c <= '9') {
                    *valueEnd = insert_str(vptr, "[]");
                }
                else {
                    *valueEnd = insert_str(vptr, "{}");
                }
            }
        }
        else {
            vptr = *valueStart;
        }
        kptr = strchr(kptr, '.');
        if(kptr == NULL) {
            return keyStart;
        }
        else {
            kptr += + 1;
        }
    }
    return NULL;
}

static int space_is_valid(json_encode_t *json, const char *key, const void *value, uint8_t type)
{
    size_t dataLen = strlen(json->jsonstr) + strlen(key) + strlen("\"\"") + strlen(",");
    if(value == NULL) {
        dataLen += strlen("null");
    }
    else if(type == JSON_TYPE_STR) {
        dataLen += strlen((char *) value) + strlen("\"\"");
    }
    else if(type == JSON_TYPE_INT) {
        dataLen += 16;
    }
    else if(type == JSON_TYPE_BOOL) {
        dataLen += strlen("false");
    }
    return (dataLen < json->size);
}

static void set_key_value(json_encode_t *json, const char *key, const void *value, uint8_t type)
{
    if(!space_is_valid(json, key, value, type)) {
        return;
    }
    char *valueStart = NULL;
    char *valueEnd = NULL;
    char* keyStart = set_key(json, key, &valueStart, &valueEnd);
    if(keyStart != NULL) {
        delete_str(valueStart, goto_value_trail(valueEnd));
    }
    insert_value(valueStart, value, type);
}

static void append_key_value(json_encode_t *json, const char *key, const void *value, uint8_t type)
{
    if(!space_is_valid(json, key, value, type)) {
        return;
    }
    char *valueStart = NULL;
    char *valueEnd = NULL;
    char* keyStart = set_key(json, key, &valueStart, &valueEnd);
    if(keyStart == NULL) {
        valueEnd = insert_str(valueStart, "[]") - 1;
    }
    else {
        valueEnd = insert_str(valueEnd - 1, ",");
    }
    insert_value(valueEnd, value, type);
}

static void delete_key(json_encode_t *json, const char *key)
{
    char* keyStart = NULL;
    char *valueStart = NULL;
    char *valueEnd = NULL;
    const char *kptr = key;
    char *vptr = json->jsonstr;
    while(kptr != NULL) {
        keyStart = find_key(vptr, kptr, &valueStart, &valueEnd);
        kptr = strchr(kptr, '.');
        if(keyStart == NULL) {
            return;
        }
        else if(kptr == NULL) {
            delete_front_comma(delete_str(keyStart, valueEnd - 1));
            return;
        }
        else {
            vptr = valueStart;
            kptr += 1;
        }
    }
}

static void set_int(json_encode_t *json, const char *key, int value)
{
    set_key_value(json, key, &value, JSON_TYPE_INT);
}

static void set_bool(json_encode_t *json, const char *key, int value)
{
    set_key_value(json, key, &value, JSON_TYPE_BOOL);
}

static void set_str(json_encode_t *json, const char *key, const char *value)
{
    set_key_value(json, key, value, JSON_TYPE_STR);
}

static void append_list_int(json_encode_t *json, const char *key, int value)
{
    append_key_value(json, key, &value, JSON_TYPE_INT);
}

static void append_list_bool(json_encode_t *json, const char *key, int value)
{
    append_key_value(json, key, &value, JSON_TYPE_BOOL);
}

static void append_list_str(json_encode_t *json, const char *key, const char *value)
{
    append_key_value(json, key, value, JSON_TYPE_STR);
}

static const json_encode_op_t gJsonEncodeOp = {
    .set_int = set_int,
    .set_bool = set_bool,
    .set_str = set_str,
    .delete_key = delete_key,
    .append_list_int = append_list_int,
    .append_list_bool = append_list_bool,
    .append_list_str = append_list_str
};

void json_encode_init(json_encode_t *json, char *buf, size_t size)
{
    memset(json, 0, sizeof(json_encode_t));
    memset(buf, 0, size);
    json->jsonstr = buf;
    json->size = size;
    json->op = &gJsonEncodeOp;
}
