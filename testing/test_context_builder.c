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

void test_context_builder_command_without_redirection(void) {
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

void test_context_builder_command_with_redirection_1(void) {
    token_t *token_list;
    execution_context_t *context_list;
    int out_flag;

    token_list = tokenizer("echo aaa > outfile");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list != NULL);
    TEST_ASSERT_TRUE(context_list[0].type == CONTEXT_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[1].type == CONTEXT_END_TYPE);

    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[0], "echo") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[1], "aaa") == 0);
    TEST_ASSERT_TRUE(context_list[0].tokens[2] == NULL);

    out_flag = context_list[0].flags &= OUT;
    TEST_ASSERT_TRUE(out_flag == OUT);

    TEST_ASSERT_TRUE(strcmp(context_list[0].output_file, "outfile") == 0);
}

void test_context_builder_command_with_redirection_2(void) {
    token_t *token_list;
    execution_context_t *context_list;
    int in_flag, out_flag, append_flag;

    token_list = tokenizer("echo aaa bbb > outfile ccc < infile ddd >> appendfile eee");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list != NULL);
    TEST_ASSERT_TRUE(context_list[0].type == CONTEXT_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[1].type == CONTEXT_END_TYPE);

    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[0], "echo") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[1], "aaa") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[2], "bbb") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[3], "ccc") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[4], "ddd") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[5], "eee") == 0);
    TEST_ASSERT_TRUE(context_list[0].tokens[6] == NULL);

    in_flag = (context_list[0].flags & IN);
    TEST_ASSERT_TRUE(in_flag == IN); 
    TEST_ASSERT_TRUE(strcmp(context_list[0].input_file, "infile") == 0);

    out_flag = (context_list[0].flags & OUT);
    TEST_ASSERT_TRUE(out_flag == OUT);
    TEST_ASSERT_TRUE(strcmp(context_list[0].output_file, "outfile") == 0);

    append_flag = context_list[0].flags &= APPEND;
    TEST_ASSERT_TRUE(append_flag == APPEND);
    TEST_ASSERT_TRUE(strcmp(context_list[0].append_file, "appendfile") == 0); 
}   

void test_context_builder_command_with_pipe_1(void) {
    token_t *token_list;
    execution_context_t *context_list;
    int pipe_output_flag, pipe_input_flag;

    token_list = tokenizer("command | command2");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list != NULL);
    TEST_ASSERT_TRUE(context_list[0].type == CONTEXT_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[1].type == CONTEXT_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[2].type == CONTEXT_END_TYPE);

    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[0], "command") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[1].tokens[0], "command2") == 0);
    TEST_ASSERT_TRUE(context_list[0].tokens[1] == NULL);
    TEST_ASSERT_TRUE(context_list[1].tokens[1] == NULL);

    pipe_input_flag = context_list[0].flags & INTO_PIPE;
    TEST_ASSERT_TRUE(pipe_input_flag == INTO_PIPE);

    pipe_output_flag = context_list[1].flags & OUT_OF_PIPE;
    TEST_ASSERT_TRUE(pipe_output_flag == OUT_OF_PIPE);

    TEST_ASSERT_TRUE(context_list[0].flags == 8);
    TEST_ASSERT_TRUE(context_list[1].flags == 16);
}   

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_context_builder_bad_syntax_1);
    RUN_TEST(test_context_builder_bad_syntax_2);
    RUN_TEST(test_context_builder_no_tokens);
    RUN_TEST(test_context_builder_command_without_redirection);
    RUN_TEST(test_context_builder_command_with_redirection_1);
    RUN_TEST(test_context_builder_command_with_redirection_2);
    RUN_TEST(test_context_builder_command_with_pipe_1);

    return UNITY_END();
}