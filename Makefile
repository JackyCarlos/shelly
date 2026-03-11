CC = gcc
CFLAGS = -I../external/Unity/src  

OBJS = shelly.c 
OBJS_TEST = shelly.c builtins.c testing/test_tokenizer.c ../external/Unity/src/unity.c

default: $(OBJS)
	$(CC) $(CFLAGS) -o bin/shelly $(OBJS)

test: $(OBJS_TEST)
	$(CC) $(CFLAGS) -o bin/test_tokenizer $(OBJS_TEST) 
