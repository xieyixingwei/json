#include "json.h"
#include <check.h>
#include <stdio.h>

#ifndef ARRAY_SIZE
    #define ARRAY_SIZE(array)   (sizeof(array) / sizeof(array[0]))
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE    0
#endif

static char jsonObjString[2000] = " \r {\
\"name\": \"Peter\",\
\"score\": [89,73],\
\"hobby\": [],\
\"mother\": {},\
\"flag\": [\"swim\", true],\
\"brother\": null,\
\"default\": \"\",\
\"classmates\": [\
{\"name\":\"Jack\"},\
{\"Name\":\"Lucy\"}\
], \
\"path\": \"/root/[/]{/},\",\
\"student\": {\
\"name\": \"Tom\",\
\"teacher\": {\
\"age\": 35\
}\
}\
}";

#if 0
int main(void)
{
    char *keyStart;
    char *valueEnd;
    extern char *get_kv(char *vptr, char *end, const char *key, char **keyPtr, char **valueEnd);
    char *vptr = get_kv(tmp, buf + strlen(tmp), "score.1", &keyStart, &valueEnd);

    printf("--- kStart[%0.10s]\r\n", keyStart);
    printf("--- vStart[%0.10s]\r\n", vptr);
    printf("--- vEnd[%0.10s]\r\n", valueEnd);

    return 0;
}
#endif
#if 1

START_TEST(json_test1)
{
    json_t json;
    json_init(&json, jsonObjString, strlen(jsonObjString), false);

    ck_assert_int_eq(NULL == json.decode->get_str(&json, "classmates.path"), 1);
    ck_assert_int_eq(NULL == json.decode->get_str(&json, "mother.path"), 1);
    ck_assert_int_eq(NULL == json.decode->get_str(&json, "mother.default"), 1);


    ck_assert_str_eq(json.decode->get_str(&json, "name"), "Peter");

    ck_assert_int_eq(json.decode->get_long(&json, "score.0", 0), 89);
    ck_assert_int_eq(json.decode->get_long(&json, "score.1", 0), 73);

    ck_assert_int_eq(json.decode->get_list_count(&json, "score"), 2);
    ck_assert_int_eq(json.decode->get_list_count(&json, "hobby"), 0);

    ck_assert_int_eq(json.decode->get_list_count(&json, "flag"), 2);
    ck_assert_str_eq(json.decode->get_str(&json, "flag.0"), "swim");
    ck_assert_int_eq(json.decode->get_bool(&json, "flag.1", FALSE), TRUE);

    ck_assert_int_eq(json.decode->is_null(&json, "brother"), TRUE);

    ck_assert_str_eq(json.decode->get_str(&json, "default"), "");

    ck_assert_int_eq(json.decode->get_list_count(&json, "classmates"), 2);
    ck_assert_str_eq(json.decode->get_str(&json, "classmates.0.name"), "Jack");

    ck_assert_ptr_eq(json.decode->get_str(&json, "classmates.1.name"), NULL);
    ck_assert_str_eq(json.decode->get_str(&json, "classmates.1.Name"), "Lucy");

    ck_assert_str_eq(json.decode->get_str(&json, "path"), "/root/[/]{/},");

    ck_assert_str_eq(json.decode->get_str(&json, "student.name"), "Tom");
    ck_assert_int_eq(json.decode->get_long(&json, "student.teacher.age", 0), 35);

    ck_assert_str_eq(json.decode->get_list_str_of(&json, "flag", 0), "swim");
    ck_assert_int_eq(json.decode->get_list_long_of(&json, "score", 1), 73);

    long vals[2];
    json.decode->get_list_long(&json, "score", vals, 2);
    ck_assert_int_eq(vals[0], 89);
    ck_assert_int_eq(vals[1], 73);
}
END_TEST

static char jsonListString[2000] = " \r [\
\"python\",\
{\
\"name\": \"C++\",\
\"detail\": {\
\"level\": 8\
}\
},\
128,\
null,\
true\
]";
START_TEST(json_test2)
{
    extern void json_init(json_t *json, char *buf, size_t size, bool clearbuf);
    json_t json;
    json_init(&json, jsonListString, strlen(jsonListString), false);

    ck_assert_str_eq(json.decode->get_str(&json, "0"), "python");

    ck_assert_str_eq(json.decode->get_str(&json, "1.name"), "C++");
    ck_assert_int_eq(json.decode->get_long(&json, "1.detail.level", 0), 8);

    ck_assert_int_eq(json.decode->get_list_count(&json, ""), 5);

    ck_assert_int_eq(json.decode->get_long(&json, "2", 0), 128);
    ck_assert_int_eq(json.decode->is_null(&json, "3"), 1);
    ck_assert_int_eq(json.decode->get_bool(&json, "4", false), 1);
}
END_TEST
#endif

#if 1
START_TEST(json_test3)
{
    json_t json;
    char tmp[300] = {0};
    json_init(&json, tmp, ARRAY_SIZE(tmp), true);

    json.encode->set_long(&json, "age", 10);
    ck_assert_str_eq(tmp, "{\"age\":10}");

    json.encode->set_str(&json, "name", "Lucy");
    ck_assert_str_eq(tmp, "{\"age\":10,\"name\":\"Lucy\"}");

    json.encode->set_str(&json, "student.name", "Jack");
    ck_assert_str_eq(tmp, "{\"age\":10,\"name\":\"Lucy\",\"student\":{\"name\":\"Jack\"}}");

    json.encode->set_bool(&json, "student.gender", true);
    ck_assert_str_eq(tmp, "{\"age\":10,\"name\":\"Lucy\",\"student\":{\"name\":\"Jack\",\"gender\":true}}");

    json.encode->set_bool(&json, "student.gender", false);
    ck_assert_str_eq(tmp, "{\"age\":10,\"name\":\"Lucy\",\"student\":{\"name\":\"Jack\",\"gender\":false}}");

    json.encode->set_str(&json, "hobby.0", "english");
    ck_assert_str_eq(tmp, "{\"age\":10,\"name\":\"Lucy\",\"student\":{\"name\":\"Jack\",\"gender\":false},\"hobby\":[\"english\"]}");

    json.encode->set_str(&json, "hobby.0", "python");
    ck_assert_str_eq(tmp, "{\"age\":10,\"name\":\"Lucy\",\"student\":{\"name\":\"Jack\",\"gender\":false},\"hobby\":[\"python\"]}");

    json.encode->set_str(&json, "hobby.1", "java");
    //printf("--- %s\r\n", tmp);
    ck_assert_str_eq(tmp, "{\"age\":10,\"name\":\"Lucy\",\"student\":{\"name\":\"Jack\",\"gender\":false},\"hobby\":[\"python\",\"java\"]}");

    json.encode->set_str(&json, "hobby.1", "C#");
    ck_assert_str_eq(tmp, "{\"age\":10,\"name\":\"Lucy\",\"student\":{\"name\":\"Jack\",\"gender\":false},\"hobby\":[\"python\",\"C#\"]}");
}
END_TEST

START_TEST(json_test4)
{
    char tmp[500] = { 0 };
    json_t json;
    json_init(&json, tmp, ARRAY_SIZE(tmp), true);

    json.encode->set_str(&json, "name", "Jack");
    ck_assert_str_eq(tmp, "{\"name\":\"Jack\"}");

    json.encode->set_str(&json, "name", "Peter");
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\"}");

    json.encode->set_str(&json, "teacher.name", "Zhangli");
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"Zhangli\"}}");

    json.encode->set_str(&json, "teacher.name", "LiLi");
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\"}}");

    json.encode->set_long(&json, "teacher.age", 32);
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\",\"age\":32}}");

    json.encode->set_long(&json, "teacher.age", 28);
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\",\"age\":28}}");

    json.encode->append_list_str(&json, "teacher.subject", "math");
    json.encode->append_list_str(&json, "teacher.subject", "english");
    //printf("--- %s\r\n", tmp);
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\",\"age\":28,\"subject\":[\"math\",\"english\"]}}");

    json.encode->append_list_long(&json, "score", 89);
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\",\"age\":28,\"subject\":[\"math\",\"english\"]},\"score\":[89]}");

    json.encode->set_str(&json, "student.name", "Peter");
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\",\"age\":28,\"subject\":[\"math\",\"english\"]},\"score\":[89],\"student\":{\"name\":\"Peter\"}}");

    json.encode->set_bool(&json, "student.gender", 0);
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\",\"age\":28,\"subject\":[\"math\",\"english\"]},\"score\":[89],\"student\":{\"name\":\"Peter\",\"gender\":false}}");

    json.encode->delete_key(&json, "teacher.name");
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"teacher\":{\"age\":28,\"subject\":[\"math\",\"english\"]},\"score\":[89],\"student\":{\"name\":\"Peter\",\"gender\":false}}");

    json.encode->delete_key(&json, "teacher.subject");
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"teacher\":{\"age\":28},\"score\":[89],\"student\":{\"name\":\"Peter\",\"gender\":false}}");

    json.encode->delete_key(&json, "student.gender");
    
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"teacher\":{\"age\":28},\"score\":[89],\"student\":{\"name\":\"Peter\"}}");

    json.encode->delete_key(&json, "teacher");
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"score\":[89],\"student\":{\"name\":\"Peter\"}}");

    json.encode->delete_key(&json, "student.name");
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"score\":[89],\"student\":{}}");

    json.encode->delete_key(&json, "student");
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"score\":[89]}");

    json.encode->delete_key(&json, "score");
    ck_assert_str_eq(tmp, "{\"name\":\"Peter\"}");

    json.encode->delete_key(&json, "name");
    ck_assert_str_eq(tmp, "{}");
}
END_TEST
#endif


START_TEST(json_test5)
{
    char tmp[500] = { 0 };
    json_t json;
    json_init(&json, tmp, ARRAY_SIZE(tmp), true);

    json.encode->append_list_str(&json, "", "hello");
    ck_assert_str_eq(tmp, "[\"hello\"]");

    json.encode->append_list_long(&json, "", 128);
    //printf("--- %s\r\n", tmp);
    ck_assert_str_eq(tmp, "[\"hello\",128]");

    //json.encode->append_list_str(&json, "student.subjects", "english");
    //printf("--- %s\r\n", tmp);
    //ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\",\"age\":28,\"subject\":[\"math\",\"english\"]}}");
}
END_TEST

START_TEST(json_test6)
{
    char tmp[500] = { 0 };
    json_t json;
    json_init(&json, tmp, ARRAY_SIZE(tmp), true);

    json.encode->append_list_long(&json, "bytes", 12);
    ck_assert_str_eq(tmp, "{\"bytes\":[12]}");

    json.encode->append_list_long(&json, "bytes", 128);
    //printf("--- %s\r\n", tmp);
    ck_assert_str_eq(tmp, "{\"bytes\":[12,128]}");

    //json.encode->append_list_str(&json, "student.subjects", "english");
    //printf("--- %s\r\n", tmp);
    //ck_assert_str_eq(tmp, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\",\"age\":28,\"subject\":[\"math\",\"english\"]}}");
}
END_TEST
#if 1
TCase *json_test_case(void)
{
    TCase *tc = tcase_create("json");
    tcase_add_test(tc, json_test1);
    tcase_add_test(tc, json_test2);
    tcase_add_test(tc, json_test3);
    tcase_add_test(tc, json_test4);
    tcase_add_test(tc, json_test5);
    tcase_add_test(tc, json_test6);
    return tc;
}

int main(void)
{
    Suite *s = suite_create("gnclib");
    suite_add_tcase(s, json_test_case());

    SRunner *runner = srunner_create(s);

    srunner_run_all(runner, CK_NORMAL);
    int n = srunner_ntests_failed(runner);
    srunner_free(runner);

    return 0;
}
#endif


