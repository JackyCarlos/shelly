CC = gcc
CFLAGS = -I../external/Unity/src  

OBJS = shelly.c main.c  
OBJS_TEST_TOKENIZER = shelly.c builtins.c testing/test_tokenizer.c ../external/Unity/src/unity.c
OBJS_TEST_CONTEXT_BUILDER = shelly.c builtins.c testing/test_context_builder.c \
                                   ../external/Unity/src/unity.c

default: $(OBJS)
	$(CC) $(CFLAGS) -o bin/shelly $(OBJS)

test-tokenizer: $(OBJS_TEST_TOKENIZER)
	$(CC) $(CFLAGS) -o bin/test_tokenizer $(OBJS_TEST_TOKENIZER) 

test-contextbuilder: $(OBJS_TEST_CONTEXT_BUILDER)
	$(CC) $(CFLAGS) -o bin/test_context_builder $(OBJS_TEST_CONTEXT_BUILDER) 
