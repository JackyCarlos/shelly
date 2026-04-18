CC = gcc

CFLAGS = -I.
CFLAGS_TEST = -I./external/unity/src  

OBJS = shelly.c builtins/builtins.c builtins/builtins_registry.c tokenizer.c contexter.c main.c 
OBJS_TEST_TOKENIZER = tokenizer.c testing/test_tokenizer.c ./external/unity/src/unity.c
OBJS_TEST_CONTEXT_BUILDER = tokenizer.c contexter.c testing/test_context_builder.c \
                                   ./external/unity/src/unity.c

default: $(OBJS)
	$(CC) $(CFLAGS) -o bin/shelly $(OBJS)

test-tokenizer: $(OBJS_TEST_TOKENIZER)
	$(CC) $(CFLAGS_TEST) -o bin/test_tokenizer $(OBJS_TEST_TOKENIZER) 

test-contextbuilder: $(OBJS_TEST_CONTEXT_BUILDER)
	$(CC) $(CFLAGS_TEST) -o bin/test_context_builder $(OBJS_TEST_CONTEXT_BUILDER) 
