CC = gcc

CFLAGS = -I.
CFLAGS_TEST = -I. -I./external/unity/src

SRC_UNITY = ./external/unity/src/unity.c
SRC_LINENOISE = ./external/linenoise/linenoise.c

SRC_CORE = \
	main.c \
	shelly.c

SRC_BUILTINS = \
	builtins/builtins.c \
	builtins/builtins_registry.c

SRC_PARSER = \
	parser/tokenizer.c \
	parser/contexter.c

SRC_EXECUTOR = \
	executor/executor.c

SRC_JOB_CTRL = \
	job-control/jobs.c \
	job-control/jobs_control_builtins.c \
	job-control/job-display/job-display.c

SRC_LN_INPUT = \
	input/input.c

SRCS = \
	$(SRC_CORE) \
	$(SRC_BUILTINS) \
	$(SRC_PARSER) \
	$(SRC_EXECUTOR) \
	$(SRC_JOB_CTRL) \
	$(SRC_LN_INPUT)

SRCS_TEST_TOKENIZER = \
	parser/tokenizer.c \
	testing/test_tokenizer.c \
	executor/executor.c \
	$(SRC_UNITY)

SRCS_TEST_CONTEXT_BUILDER = \
	parser/tokenizer.c \
	parser/contexter.c \
	testing/test_context_builder.c \
	executor/executor.c \
	$(SRC_UNITY)

default: $(SRCS)
	$(CC) $(CFLAGS) -o bin/shelly $(SRCS) $(SRC_LINENOISE)

test-tokenizer: $(SRCS_TEST_TOKENIZER)
	$(CC) $(CFLAGS_TEST) -o bin/test_tokenizer $(SRCS_TEST_TOKENIZER)

test-contextbuilder: $(SRCS_TEST_CONTEXT_BUILDER)
	$(CC) $(CFLAGS_TEST) -o bin/test_context_builder $(SRCS_TEST_CONTEXT_BUILDER)