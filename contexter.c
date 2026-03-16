#include <stddef.h>
#include <stdlib.h>

#include "shelly.h"

static void initiate_contexts(execution_context_t *contexts);

static void initiate_contexts(execution_context_t *contexts) {
    int i;

    for (i = 0; i < 32; ++i) {
        contexts[i].tokens_index    = 0;
        contexts[i].flags           = 0;
    }
}

execution_context_t *get_context(token_t *token_list) {
    execution_context_t *contexts;
    int i;
    context_status status;

    int context_array_size;

    int tokens_index;   // index in the token array of a single context 
    int tokens_size;    // size of the char * array of a single context

    context_array_size = 32;
    contexts = (execution_context_t *) malloc(sizeof(execution_context_t) * context_array_size);    // CONTEXT_ARRAY_SIZE
    initiate_contexts(contexts);
    
    i = 0;

    contexts[i].type = CONTEXT_END_TYPE;
    status = STATUS_CONTEXT_INIT;

    while (token_list->type != NULL_TYPE) {
        if (i == context_array_size - 1) {
            context_array_size += 32;
            contexts = realloc(contexts, sizeof(execution_context_t) * context_array_size);
            initiate_contexts(contexts + i + 1);
        }

        switch (token_list->type) {
            case REDIRECT_OUT_TYPE:
                if (status != STATUS_CONTEXT_WORD) {
                    return NULL;
                }

                status = STATUS_CONTEXT_REDIRECT_OUT;
                break;

            case REDIRECT_IN_TYPE:
                if (status != STATUS_CONTEXT_WORD) {
                    return NULL;
                }

                status = STATUS_CONTEXT_REDIRECT_IN;
                break;

            case REDIRECT_PIPE_TYPE: 
                if (status != STATUS_CONTEXT_WORD) {
                    return NULL;
                }

                contexts[i++].flags |= INTO_PIPE;
                status = STATUS_CONTEXT_PIPE;
                break;

            case REDIRECT_APPEND_TYPE: 
                if (status != STATUS_CONTEXT_WORD) {
                    return NULL;
                }

                status = STATUS_CONTEXT_REDIRECT_APPEND;
                break;

            default:
                if (status == STATUS_CONTEXT_REDIRECT_OUT) {
                    contexts[i].output_file = token_list->str;
                    contexts[i].flags |= OUT;

                } else if (status == STATUS_CONTEXT_REDIRECT_IN) {
                    contexts[i].input_file = token_list->str;
                    contexts[i].flags |= IN;

                } else if (status == STATUS_CONTEXT_REDIRECT_APPEND) {
                    contexts[i].append_file = token_list->str;
                    contexts[i].flags |= APPEND;

                } else {
                    if (status == STATUS_CONTEXT_PIPE) {
                        contexts[i].flags |= OUT_OF_PIPE;
                    }

                    tokens_index = contexts[i].tokens_index;

                    if (tokens_index == 0) {
                        contexts[i].type = CONTEXT_COMMAND_TYPE;
                        contexts[i + 1].type = CONTEXT_END_TYPE;

                        tokens_size = 32;
                        contexts[i].tokens = malloc(sizeof(char *) * tokens_size);

                    } else if (tokens_index - 1 == tokens_size) {
                        tokens_size += 32;
                        contexts[i].tokens = realloc(contexts[i].tokens, tokens_size);
                    } 
                    
                    // left to do check for tokens_index - 1 == tokens_size and reallocate if necessary

                    contexts[i].tokens[tokens_index++] = token_list->str;
                    contexts[i].tokens[tokens_index] = NULL;
                    contexts[i].tokens_index++;
                }

                status = STATUS_CONTEXT_WORD;
        }
        
        token_list++;
    } 

    if (status != STATUS_CONTEXT_WORD && contexts[0].type != CONTEXT_END_TYPE) {
        return NULL;
    }

    return contexts;
}
