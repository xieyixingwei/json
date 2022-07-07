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

static char buf[2000] = " \r {\
\"name\": \"Peter\",\
\"score\": [89,73],\
\"hobby\": [],\
\"mother\": {},\
\"flag\": [\"swim\", true],\
\"brother\": null,\
\"default\": \" \",\
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

#if 1
START_TEST(json_test1)
{
    json_value_t values[100];
    extern void json_decode(json_decode_t *json, json_value_t *values, size_t n, char *jsonstr);
    json_decode_t json;
    json_decode(&json, values, ARRAY_SIZE(values), buf);

    //extern void json_decode_print(json_decode_t *json);
    //json_decode_print(&json);

    ck_assert_str_eq(json.op->get_str(&json, "name"), "Peter");

    ck_assert_int_eq(json.op->get_int(&json, "score.0", 0), 89);
    ck_assert_int_eq(json.op->get_int(&json, "score.1", 0), 73);
#if 1
    ck_assert_int_eq(json.op->get_list_count(&json, "score"), 2);
    ck_assert_int_eq(json.op->get_list_count(&json, "hobby"), 0);

    ck_assert_int_eq(json.op->get_list_count(&json, "flag"), 2);
    ck_assert_str_eq(json.op->get_str(&json, "flag.0"), "swim");
    ck_assert_int_eq(json.op->get_bool(&json, "flag.1", FALSE), TRUE);

    ck_assert_int_eq(json.op->is_null(&json, "brother"), TRUE);

    ck_assert_str_eq(json.op->get_str(&json, "default"), " ");

    ck_assert_int_eq(json.op->get_list_count(&json, "classmates"), 2);
    ck_assert_str_eq(json.op->get_str(&json, "classmates.0.name"), "Jack");

    ck_assert_ptr_eq(json.op->get_str(&json, "classmates.1.name"), NULL);
    ck_assert_str_eq(json.op->get_str(&json, "classmates.1.Name"), "Lucy");

    ck_assert_str_eq(json.op->get_str(&json, "path"), "/root/[/]{/},");

    ck_assert_str_eq(json.op->get_str(&json, "student.name"), "Tom");
    ck_assert_int_eq(json.op->get_int(&json, "student.teacher.age", 0), 35);

    ck_assert_str_eq(json.op->get_list_str_of(&json, "flag", 0), "swim");
    ck_assert_int_eq(json.op->get_list_int_of(&json, "score", 1), 73);

    int vals[2];
    json.op->get_list_int(&json, "score", vals, 2);
    ck_assert_int_eq(vals[0], 89);
    ck_assert_int_eq(vals[1], 73);
#endif
}
END_TEST
#endif
    
START_TEST(json_test2)
{
    json_encode_t jsonSet;
    char tmp[30] = {0};
    json_encode_init(&jsonSet, tmp, sizeof(tmp));

    jsonSet.op->set_int(&jsonSet, "age", 10);
    ck_assert_str_eq(tmp, "{\"age\":10}");

    jsonSet.op->set_str(&jsonSet, "name", "Lucy");
    ck_assert_str_eq(tmp, "{\"age\":10,\"name\":\"Lucy\"}");

    jsonSet.op->set_str(&jsonSet, "student.name", "Jack");
    ck_assert_str_eq(tmp, "{\"age\":10,\"name\":\"Lucy\"}");

/*
    jsonSet.op->set_bool(&jsonSet, "student.gender", 1);
    printf("%s\n", tmp);
    jsonSet.op->set_bool(&jsonSet, "student.gender", 0);
    printf("%s\n", tmp);
*/
/*
    jsonSet.op->set_str(&jsonSet, "hobby.0", "english");
    printf("%s\n", tmp);
    jsonSet.op->set_str(&jsonSet, "hobby.0", "python");
    printf("%s\n", tmp);
*/
}
END_TEST

START_TEST(json_test3)
{
    char buf[500] = { 0 };
    json_encode_t json;
    json_encode_init(&json, buf, ARRAY_SIZE(buf));

    json.op->set_str(&json, "name", "Jack");
    ck_assert_str_eq(buf, "{\"name\":\"Jack\"}");

    json.op->set_str(&json, "name", "Peter");
    ck_assert_str_eq(buf, "{\"name\":\"Peter\"}");

    json.op->set_str(&json, "teacher.name", "Zhangli");
    ck_assert_str_eq(buf, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"Zhangli\"}}");

    json.op->set_str(&json, "teacher.name", "LiLi");
    ck_assert_str_eq(buf, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\"}}");

    json.op->set_int(&json, "teacher.age", 32);
    ck_assert_str_eq(buf, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\",\"age\":32}}");

    json.op->set_int(&json, "teacher.age", 28);
    ck_assert_str_eq(buf, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\",\"age\":28}}");

    json.op->append_list_str(&json, "teacher.subject", "math");
    json.op->append_list_str(&json, "teacher.subject", "english");
    ck_assert_str_eq(buf, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\",\"age\":28,\"subject\":[\"math\",\"english\"]}}");

    json.op->append_list_int(&json, "score", 89);
    ck_assert_str_eq(buf, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\",\"age\":28,\"subject\":[\"math\",\"english\"]},\"score\":[89]}");

    json.op->set_str(&json, "student.name", "Peter");
    ck_assert_str_eq(buf, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\",\"age\":28,\"subject\":[\"math\",\"english\"]},\"score\":[89],\"student\":{\"name\":\"Peter\"}}");

    json.op->set_bool(&json, "student.gender", 0);
    ck_assert_str_eq(buf, "{\"name\":\"Peter\",\"teacher\":{\"name\":\"LiLi\",\"age\":28,\"subject\":[\"math\",\"english\"]},\"score\":[89],\"student\":{\"name\":\"Peter\",\"gender\":false}}");

    json.op->delete_key(&json, "teacher.name");
    ck_assert_str_eq(buf, "{\"name\":\"Peter\",\"teacher\":{\"age\":28,\"subject\":[\"math\",\"english\"]},\"score\":[89],\"student\":{\"name\":\"Peter\",\"gender\":false}}");

    json.op->delete_key(&json, "teacher.subject");
    ck_assert_str_eq(buf, "{\"name\":\"Peter\",\"teacher\":{\"age\":28},\"score\":[89],\"student\":{\"name\":\"Peter\",\"gender\":false}}");

    json.op->delete_key(&json, "student.gender");
    ck_assert_str_eq(buf, "{\"name\":\"Peter\",\"teacher\":{\"age\":28},\"score\":[89],\"student\":{\"name\":\"Peter\"}}");

    json.op->delete_key(&json, "teacher");
    ck_assert_str_eq(buf, "{\"name\":\"Peter\",\"score\":[89],\"student\":{\"name\":\"Peter\"}}");

    json.op->delete_key(&json, "student.name");
    ck_assert_str_eq(buf, "{\"name\":\"Peter\",\"score\":[89],\"student\":{}}");

    json.op->delete_key(&json, "student");
    ck_assert_str_eq(buf, "{\"name\":\"Peter\",\"score\":[89]}");

    json.op->delete_key(&json, "score");
    ck_assert_str_eq(buf, "{\"name\":\"Peter\"}");

    json.op->delete_key(&json, "name");
    ck_assert_str_eq(buf, "{}");
}
END_TEST

TCase *json_test_case(void)
{
    TCase *tc = tcase_create("json");
    tcase_add_test(tc, json_test1);
    tcase_add_test(tc, json_test2);
    tcase_add_test(tc, json_test3);
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
