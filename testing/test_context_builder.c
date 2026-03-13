#include "../../external/Unity/src/unity.h"
#include "../shelly.h"
#include <string.h>

token_t *tokenizer(char *line);
execution_context_t *get_context(token_t *token_list);

void setUp(void) {

}

void tearDown(void) {

}

// test for null return value in case statement starts with redirection token
void test_context_builder_bad_syntax_1(void) {
    token_t *token_list;
    execution_context_t *context_list;

    token_list = tokenizer("|");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list == NULL);

    token_list = tokenizer("<");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list == NULL);

    token_list = tokenizer(">");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list == NULL);

    token_list = tokenizer(">>");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list == NULL);
}

void test_context_builder_bad_syntax_2(void) {
    token_t *token_list;
    execution_context_t *context_list;

    token_list = tokenizer("echo test >> file >");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list == NULL);

    token_list = tokenizer("echo test |");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list == NULL);

    token_list = tokenizer("echo test | < file");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list == NULL);

    token_list = tokenizer("echo test1 test2 test3 > > output");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list == NULL);

    token_list = tokenizer("echo test | grep -v a | grep -vE b | ");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list == NULL);
}

void test_context_builder_no_tokens(void) {
    token_t *token_list;
    execution_context_t *context_list;

    token_list = tokenizer("");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list != NULL);
    TEST_ASSERT_TRUE(context_list[0].type == CONTEXT_END_TYPE);
}

void test_context_builder_statements_without_redirection(void) {
    token_t *token_list;
    execution_context_t *context_list;

    token_list = tokenizer("echo aaa bbb ccc ddd");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list != NULL);
    TEST_ASSERT_TRUE(context_list[0].type == CONTEXT_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[1].type == CONTEXT_END_TYPE);

    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[0], "echo") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[1], "aaa") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[2], "bbb") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[3], "ccc") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[4], "ddd") == 0);
    TEST_ASSERT_TRUE(context_list[0].tokens[5] == NULL);
}


int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_context_builder_bad_syntax_1);
    RUN_TEST(test_context_builder_bad_syntax_2);
    RUN_TEST(test_context_builder_no_tokens);
    RUN_TEST(test_context_builder_statements_without_redirection);

    return UNITY_END();
}