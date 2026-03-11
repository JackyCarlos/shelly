#include "../../external/Unity/src/unity.h"
#include "../shelly.h"
#include <string.h>

void setUp(void) {

}

void tearDown(void) {

}

token_t *tokenizer(char *line);

void test_tokenizer_ignore_blank_characters(void) {
    token_t *token_list;

    token_list = tokenizer("");
    TEST_ASSERT_TRUE(token_list[0].type == NULL_TYPE);

    token_list = tokenizer("\r\t\t\r");
    TEST_ASSERT_TRUE(token_list[0].type == NULL_TYPE);

    token_list = tokenizer("              \r     \t           \r");
    TEST_ASSERT_TRUE(token_list[0].type == NULL_TYPE);    
}

void test_tokenizer_word_tokens_only(void) {
    token_t *token_list, *token_list2;
 
    token_list = token_list2 = tokenizer("echo aaa bbb ccc 444 !!!");
    while (token_list2->type != NULL_TYPE) {
        TEST_ASSERT_TRUE(token_list2->type == WORD_TYPE);  
        token_list2++;  
    }
    
    TEST_ASSERT_TRUE(strcmp(token_list[0].str, "echo") == 0);
    TEST_ASSERT_TRUE(strcmp(token_list[1].str, "aaa") == 0);
    TEST_ASSERT_TRUE(strcmp(token_list[2].str, "bbb") == 0);
    TEST_ASSERT_TRUE(strcmp(token_list[3].str, "ccc") == 0);
    TEST_ASSERT_TRUE(strcmp(token_list[4].str, "444") == 0);
    TEST_ASSERT_TRUE(strcmp(token_list[5].str, "!!!") == 0);
}

void test_tokenizer_redirect_tokens_only(void) {
    token_t *token_list, *token_list2;
 
    token_list = tokenizer(">>>>|<>>> > |");
    
    TEST_ASSERT_TRUE(token_list[0].type == REDIRECT_APPEND_TYPE);
    TEST_ASSERT_TRUE(token_list[1].type == REDIRECT_APPEND_TYPE);
    TEST_ASSERT_TRUE(token_list[2].type == REDIRECT_PIPE_TYPE);
    TEST_ASSERT_TRUE(token_list[3].type == REDIRECT_IN_TYPE);
    TEST_ASSERT_TRUE(token_list[4].type == REDIRECT_APPEND_TYPE);
    TEST_ASSERT_TRUE(token_list[5].type == REDIRECT_OUT_TYPE);
    TEST_ASSERT_TRUE(token_list[6].type == REDIRECT_OUT_TYPE);
    TEST_ASSERT_TRUE(token_list[7].type == REDIRECT_PIPE_TYPE);
    TEST_ASSERT_TRUE(token_list[8].type == NULL_TYPE);
}

void test_tokenizer_mixed_tokens1(void) {
    token_t *token_list, *token_list2;
 
    token_list = tokenizer("base64 -d file | grep -v regex > outfile");
    
    TEST_ASSERT_TRUE(token_list[0].type == WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[1].type == WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[2].type == WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[3].type == REDIRECT_PIPE_TYPE);
    TEST_ASSERT_TRUE(token_list[4].type == WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[5].type == WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[6].type == WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[7].type == REDIRECT_OUT_TYPE);
    TEST_ASSERT_TRUE(token_list[8].type == WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[9].type == NULL_TYPE);
}

void test_tokenizer_mixed_tokens2(void) {
    token_t *token_list, *token_list2;
 
    token_list = token_list2 = tokenizer("xxd<file|grep -v regex|grep regex2>>outfile");
    
    TEST_ASSERT_TRUE(token_list[0].type == WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[1].type == REDIRECT_IN_TYPE);
    TEST_ASSERT_TRUE(token_list[2].type == WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[3].type == REDIRECT_PIPE_TYPE);
    TEST_ASSERT_TRUE(token_list[4].type == WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[5].type == WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[6].type == WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[7].type == REDIRECT_PIPE_TYPE);
    TEST_ASSERT_TRUE(token_list[8].type == WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[9].type == WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[10].type == REDIRECT_APPEND_TYPE);
    TEST_ASSERT_TRUE(token_list[11].type == WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[12].type == NULL_TYPE);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_tokenizer_ignore_blank_characters);
    RUN_TEST(test_tokenizer_word_tokens_only);
    RUN_TEST(test_tokenizer_redirect_tokens_only);
    RUN_TEST(test_tokenizer_mixed_tokens1); 
    RUN_TEST(test_tokenizer_mixed_tokens2); 

    return UNITY_END();
}