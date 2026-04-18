#include <string.h>
#include "../external/unity/src/unity.h"
#include "../parser/parser.h"

void setUp(void) {

}

void tearDown(void) {

}

token_t *tokenizer(char *line);

void test_tokenizer_ignore_blank_characters(void) {
    token_t *token_list;

    token_list = tokenizer("");
    TEST_ASSERT_TRUE(token_list[0].type == TOK_NULL_TYPE);

    token_list = tokenizer("\r\t\t\r");
    TEST_ASSERT_TRUE(token_list[0].type == TOK_NULL_TYPE);

    token_list = tokenizer("              \r     \t           \r");
    TEST_ASSERT_TRUE(token_list[0].type == TOK_NULL_TYPE);    
}

void test_tokenizer_word_tokens_only(void) {
    token_t *token_list, *token_list2;
 
    token_list = token_list2 = tokenizer("echo aaa bbb ccc 444 !!!");
    while (token_list2->type != TOK_NULL_TYPE) {
        TEST_ASSERT_TRUE(token_list2->type == TOK_WORD_TYPE);  
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
    token_t *token_list;
 
    token_list = tokenizer(">>>>|<>>> > |");
    
    TEST_ASSERT_TRUE(token_list[0].type == TOK_REDIRECT_APPEND_TYPE);
    TEST_ASSERT_TRUE(token_list[1].type == TOK_REDIRECT_APPEND_TYPE);
    TEST_ASSERT_TRUE(token_list[2].type == TOK_REDIRECT_PIPE_TYPE);
    TEST_ASSERT_TRUE(token_list[3].type == TOK_REDIRECT_IN_TYPE);
    TEST_ASSERT_TRUE(token_list[4].type == TOK_REDIRECT_APPEND_TYPE);
    TEST_ASSERT_TRUE(token_list[5].type == TOK_REDIRECT_OUT_TYPE);
    TEST_ASSERT_TRUE(token_list[6].type == TOK_REDIRECT_OUT_TYPE);
    TEST_ASSERT_TRUE(token_list[7].type == TOK_REDIRECT_PIPE_TYPE);
    TEST_ASSERT_TRUE(token_list[8].type == TOK_NULL_TYPE);
}

void test_tokenizer_mixed_tokens1(void) {
    token_t *token_list;
 
    token_list = tokenizer("base64 -d file | grep -v regex > outfile");
    
    TEST_ASSERT_TRUE(token_list[0].type == TOK_WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[1].type == TOK_WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[2].type == TOK_WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[3].type == TOK_REDIRECT_PIPE_TYPE);
    TEST_ASSERT_TRUE(token_list[4].type == TOK_WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[5].type == TOK_WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[6].type == TOK_WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[7].type == TOK_REDIRECT_OUT_TYPE);
    TEST_ASSERT_TRUE(token_list[8].type == TOK_WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[9].type == TOK_NULL_TYPE);
}

void test_tokenizer_mixed_tokens2(void) {
    token_t *token_list;
 
    token_list = tokenizer("xxd<file|grep -v regex|grep regex2>>outfile");
    
    TEST_ASSERT_TRUE(token_list[0].type == TOK_WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[1].type == TOK_REDIRECT_IN_TYPE);
    TEST_ASSERT_TRUE(token_list[2].type == TOK_WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[3].type == TOK_REDIRECT_PIPE_TYPE);
    TEST_ASSERT_TRUE(token_list[4].type == TOK_WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[5].type == TOK_WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[6].type == TOK_WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[7].type == TOK_REDIRECT_PIPE_TYPE);
    TEST_ASSERT_TRUE(token_list[8].type == TOK_WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[9].type == TOK_WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[10].type == TOK_REDIRECT_APPEND_TYPE);
    TEST_ASSERT_TRUE(token_list[11].type == TOK_WORD_TYPE);
    TEST_ASSERT_TRUE(token_list[12].type == TOK_NULL_TYPE);
}

void test_tokenizer_token_array_realloc(void) {
    token_t *token_list;
 
    token_list = tokenizer("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29");
    
    TEST_ASSERT_FALSE(token_list[29].type == TOK_NULL_TYPE);
    TEST_ASSERT_TRUE(token_list[30].type == TOK_NULL_TYPE);
    TEST_ASSERT_FALSE(token_list[31].type == TOK_NULL_TYPE);

    token_list = tokenizer("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33");
    TEST_ASSERT_FALSE(token_list[29].type == TOK_NULL_TYPE);
    TEST_ASSERT_FALSE(token_list[30].type == TOK_NULL_TYPE);
    TEST_ASSERT_FALSE(token_list[31].type == TOK_NULL_TYPE);
    TEST_ASSERT_FALSE(token_list[32].type == TOK_NULL_TYPE);
    TEST_ASSERT_FALSE(token_list[33].type == TOK_NULL_TYPE);
    TEST_ASSERT_TRUE(token_list[34].type == TOK_NULL_TYPE);
}

void test_tokenizer_long_word_tokens(void) {
    token_t *token_list;
 
    char *a31 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    char *b32 = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    char *c33 = "ccccccccccccccccccccccccccccccccc";

    token_list = tokenizer(a31);
    TEST_ASSERT_TRUE(strcmp(token_list[0].str, a31) == 0);

    token_list = tokenizer(b32);
    TEST_ASSERT_TRUE(strcmp(token_list[0].str, b32) == 0);

    token_list = tokenizer(c33);
    TEST_ASSERT_TRUE(strcmp(token_list[0].str, c33) == 0);
}



int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_tokenizer_ignore_blank_characters);
    RUN_TEST(test_tokenizer_word_tokens_only);
    RUN_TEST(test_tokenizer_redirect_tokens_only);
    RUN_TEST(test_tokenizer_mixed_tokens1); 
    RUN_TEST(test_tokenizer_mixed_tokens2); 
    RUN_TEST(test_tokenizer_token_array_realloc);
    RUN_TEST(test_tokenizer_long_word_tokens);

    return UNITY_END();
}