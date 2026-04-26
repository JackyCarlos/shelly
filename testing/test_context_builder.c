#include <string.h>
#include "../external/unity/src/unity.h"
#include "../parser/parser.h"

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

    token_list = tokenizer("&");
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

    token_list = tokenizer("ls & ls & &");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list == NULL);

    token_list = tokenizer("command | command2 | &");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list == NULL);

    token_list = tokenizer("command & |");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list == NULL);
}

void test_context_builder_no_tokens(void) {
    token_t *token_list;
    execution_context_t *context_list;

    token_list = tokenizer("");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list != NULL);
    TEST_ASSERT_TRUE(context_list[0].type == CTX_END_TYPE);
}

void test_context_builder_command_without_redirection(void) {
    token_t *token_list;
    execution_context_t *context_list;

    token_list = tokenizer("echo aaa bbb ccc ddd");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list != NULL);
    TEST_ASSERT_TRUE(context_list[0].type == CTX_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[1].type == CTX_END_TYPE);

    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[0], "echo") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[1], "aaa") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[2], "bbb") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[3], "ccc") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[4], "ddd") == 0);
    TEST_ASSERT_TRUE(context_list[0].tokens[5] == NULL);

    TEST_ASSERT_TRUE(context_list[0].is_background == 0);
    TEST_ASSERT_TRUE(context_list[0].pipeline_end == 1);
}

void test_context_builder_command_with_redirection_1(void) {
    token_t *token_list;
    execution_context_t *context_list;
    int out_flag;

    token_list = tokenizer("echo aaa > outfile");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list != NULL);
    TEST_ASSERT_TRUE(context_list[0].type == CTX_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[1].type == CTX_END_TYPE);

    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[0], "echo") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[1], "aaa") == 0);
    TEST_ASSERT_TRUE(context_list[0].tokens[2] == NULL);

    TEST_ASSERT_TRUE(context_list[0].is_background == 0);
    TEST_ASSERT_TRUE(context_list[0].pipeline_end == 1);

    out_flag = context_list[0].flags &= REDIR_OUT;
    TEST_ASSERT_TRUE(out_flag == REDIR_OUT);

    TEST_ASSERT_TRUE(strcmp(context_list[0].output_file, "outfile") == 0);
}

void test_context_builder_command_with_redirection_2(void) {
    token_t *token_list;
    execution_context_t *context_list;
    int in_flag, out_flag, append_flag;

    token_list = tokenizer("echo aaa bbb > outfile ccc < infile ddd >> appendfile eee");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list != NULL);
    TEST_ASSERT_TRUE(context_list[0].type == CTX_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[1].type == CTX_END_TYPE);

    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[0], "echo") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[1], "aaa") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[2], "bbb") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[3], "ccc") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[4], "ddd") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[5], "eee") == 0);
    TEST_ASSERT_TRUE(context_list[0].tokens[6] == NULL);

    TEST_ASSERT_TRUE(context_list[0].is_background == 0);
    TEST_ASSERT_TRUE(context_list[0].pipeline_end == 1);

    in_flag = (context_list[0].flags & REDIR_IN);
    TEST_ASSERT_TRUE(in_flag == REDIR_IN); 
    TEST_ASSERT_TRUE(strcmp(context_list[0].input_file, "infile") == 0);

    out_flag = (context_list[0].flags & REDIR_OUT);
    TEST_ASSERT_TRUE(out_flag == REDIR_OUT);
    TEST_ASSERT_TRUE(strcmp(context_list[0].output_file, "outfile") == 0);

    append_flag = context_list[0].flags &= REDIR_APPEND;
    TEST_ASSERT_TRUE(append_flag == REDIR_APPEND);
    TEST_ASSERT_TRUE(strcmp(context_list[0].append_file, "appendfile") == 0); 
}   

void test_context_builder_command_with_pipe_1(void) {
    token_t *token_list;
    execution_context_t *context_list;
    int pipe_output_flag, pipe_input_flag;

    token_list = tokenizer("command | command2");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list != NULL);
    TEST_ASSERT_TRUE(context_list[0].type == CTX_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[1].type == CTX_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[2].type == CTX_END_TYPE);

    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[0], "command") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[1].tokens[0], "command2") == 0);
    TEST_ASSERT_TRUE(context_list[0].tokens[1] == NULL);
    TEST_ASSERT_TRUE(context_list[1].tokens[1] == NULL);

    TEST_ASSERT_TRUE(context_list[0].is_background == 0);
    TEST_ASSERT_TRUE(context_list[0].pipeline_end == 0);
    TEST_ASSERT_TRUE(context_list[1].is_background == 0);
    TEST_ASSERT_TRUE(context_list[1].pipeline_end == 1);

    pipe_input_flag = context_list[0].flags & REDIR_INTO_PIPE;
    TEST_ASSERT_TRUE(pipe_input_flag == REDIR_INTO_PIPE);

    pipe_output_flag = context_list[1].flags & REDIR_OUT_OF_PIPE;
    TEST_ASSERT_TRUE(pipe_output_flag == REDIR_OUT_OF_PIPE);

    TEST_ASSERT_TRUE(context_list[0].flags == 8);
    TEST_ASSERT_TRUE(context_list[1].flags == 16);
}   

void test_context_builder_command_with_pipe_2(void) {
    token_t *token_list;
    execution_context_t *context_list;
    int pipe_output_flag, pipe_input_flag, out_flag, in_flag;

    token_list = tokenizer("command < input > output aaa | command2 | command3 -v >> output2 -B < input2 | command4 bbb");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list != NULL);
    TEST_ASSERT_TRUE(context_list[0].type == CTX_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[1].type == CTX_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[2].type == CTX_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[3].type == CTX_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[4].type == CTX_END_TYPE);


    // first command
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[0], "command") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[1], "aaa") == 0);
    TEST_ASSERT_TRUE(context_list[0].tokens[2] == NULL);

    TEST_ASSERT_TRUE(context_list[0].is_background == 0);
    TEST_ASSERT_TRUE(context_list[0].pipeline_end == 0);

    pipe_input_flag = context_list[0].flags & REDIR_INTO_PIPE;
    pipe_output_flag = context_list[0].flags & REDIR_OUT_OF_PIPE;
    in_flag = (context_list[0].flags & REDIR_IN);
    out_flag = (context_list[0].flags & REDIR_OUT);

    TEST_ASSERT_TRUE(pipe_input_flag == REDIR_INTO_PIPE);
    TEST_ASSERT_FALSE(pipe_output_flag == REDIR_OUT_OF_PIPE);
    TEST_ASSERT_TRUE(in_flag == REDIR_IN); 
    TEST_ASSERT_TRUE(out_flag == REDIR_OUT);

    TEST_ASSERT_TRUE(strcmp(context_list[0].input_file, "input") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[0].output_file, "output") == 0);


    // second command
    TEST_ASSERT_TRUE(strcmp(context_list[1].tokens[0], "command2") == 0);
    TEST_ASSERT_TRUE(context_list[1].tokens[1] == NULL);

    TEST_ASSERT_TRUE(context_list[1].is_background == 0);
    TEST_ASSERT_TRUE(context_list[1].pipeline_end == 0);

    pipe_input_flag = context_list[1].flags & REDIR_INTO_PIPE;
    pipe_output_flag = context_list[1].flags & REDIR_OUT_OF_PIPE;

    TEST_ASSERT_TRUE(pipe_input_flag == REDIR_INTO_PIPE);
    TEST_ASSERT_TRUE(pipe_output_flag == REDIR_OUT_OF_PIPE);


    // third command
    TEST_ASSERT_TRUE(strcmp(context_list[2].tokens[0], "command3") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[2].tokens[1], "-v") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[2].tokens[2], "-B") == 0);
    TEST_ASSERT_TRUE(context_list[2].tokens[3] == NULL);

    TEST_ASSERT_TRUE(context_list[2].is_background == 0);
    TEST_ASSERT_TRUE(context_list[2].pipeline_end == 0);

    pipe_input_flag = context_list[2].flags & REDIR_INTO_PIPE;
    pipe_output_flag = context_list[2].flags & REDIR_OUT_OF_PIPE;

    TEST_ASSERT_TRUE(pipe_input_flag == REDIR_INTO_PIPE);
    TEST_ASSERT_TRUE(pipe_output_flag == REDIR_OUT_OF_PIPE);


    // fourth command
    TEST_ASSERT_TRUE(strcmp(context_list[3].tokens[0], "command4") == 0);
    TEST_ASSERT_TRUE(strcmp(context_list[3].tokens[1], "bbb") == 0);
    TEST_ASSERT_TRUE(context_list[3].tokens[2] == NULL);

    TEST_ASSERT_TRUE(context_list[3].is_background == 0);
    TEST_ASSERT_TRUE(context_list[3].pipeline_end == 1);

    pipe_input_flag = context_list[3].flags & REDIR_INTO_PIPE;
    pipe_output_flag = context_list[3].flags & REDIR_OUT_OF_PIPE;

    TEST_ASSERT_FALSE(pipe_input_flag == REDIR_INTO_PIPE);
    TEST_ASSERT_TRUE(pipe_output_flag == REDIR_OUT_OF_PIPE);
}

void test_context_builder_command_with_ampersand_1(void) {
    token_t *token_list;
    execution_context_t *context_list;

    token_list = tokenizer("command &");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list != NULL);
    TEST_ASSERT_TRUE(context_list[0].type == CTX_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[1].type == CTX_END_TYPE);

    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[0], "command") == 0);
    TEST_ASSERT_TRUE(context_list[0].tokens[1] == NULL);
    
    TEST_ASSERT_TRUE(context_list[0].is_background == 1);
    TEST_ASSERT_TRUE(context_list[0].pipeline_end == 1);
}

void test_context_builder_command_with_ampersand_2(void) {
    token_t *token_list;
    execution_context_t *context_list;

    token_list = tokenizer("ls & ls & ls");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list != NULL);
    TEST_ASSERT_TRUE(context_list[0].type == CTX_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[1].type == CTX_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[2].type == CTX_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[3].type == CTX_END_TYPE);

    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[0], "ls") == 0);
    TEST_ASSERT_TRUE(context_list[0].tokens[1] == NULL);
    TEST_ASSERT_TRUE(strcmp(context_list[1].tokens[0], "ls") == 0);
    TEST_ASSERT_TRUE(context_list[1].tokens[1] == NULL);
    TEST_ASSERT_TRUE(strcmp(context_list[2].tokens[0], "ls") == 0);
    TEST_ASSERT_TRUE(context_list[2].tokens[1] == NULL);

    TEST_ASSERT_TRUE(context_list[0].is_background == 1);
    TEST_ASSERT_TRUE(context_list[0].pipeline_end == 1);
    TEST_ASSERT_TRUE(context_list[1].is_background == 1);
    TEST_ASSERT_TRUE(context_list[1].pipeline_end == 1);
    TEST_ASSERT_TRUE(context_list[2].is_background == 0);
    TEST_ASSERT_TRUE(context_list[2].pipeline_end == 1);
}

void test_context_builder_command_with_ampersand_3(void) {
    token_t *token_list;
    execution_context_t *context_list;

    token_list = tokenizer("command | command2 &");
    context_list = get_context(token_list);
    TEST_ASSERT_TRUE(context_list != NULL);
    TEST_ASSERT_TRUE(context_list[0].type == CTX_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[1].type == CTX_COMMAND_TYPE);
    TEST_ASSERT_TRUE(context_list[2].type == CTX_END_TYPE);

    TEST_ASSERT_TRUE(strcmp(context_list[0].tokens[0], "command") == 0);
    TEST_ASSERT_TRUE(context_list[0].tokens[1] == NULL);
    TEST_ASSERT_TRUE(strcmp(context_list[1].tokens[0], "command2") == 0);
    TEST_ASSERT_TRUE(context_list[1].tokens[1] == NULL);

    TEST_ASSERT_TRUE(context_list[0].is_background == 1);
    TEST_ASSERT_TRUE(context_list[0].pipeline_end == 0);
    TEST_ASSERT_TRUE(context_list[1].is_background == 1);
    TEST_ASSERT_TRUE(context_list[1].pipeline_end == 1);
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
    RUN_TEST(test_context_builder_command_with_pipe_2);
    RUN_TEST(test_context_builder_command_with_ampersand_1);
    RUN_TEST(test_context_builder_command_with_ampersand_2);
    RUN_TEST(test_context_builder_command_with_ampersand_3);

    return UNITY_END();
}