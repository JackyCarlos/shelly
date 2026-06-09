# shelly - a definetely not POSIX compliant shell

This shell is a fun project for learning some linux operating system principles - mostly about processes, process groups and signaling. This writeup summarizes the things I learned and can be viewed like a short documentation for me.

## Motivation

When looking for something to program I stumbled accross the [blog entry](https://brennan.io/2015/01/16/write-a-shell-in-c/) of Stephen Brennan on how to write a shell in C. He presents a very basic shell which is capable of printing a small cli, executing typed commands and wait for the spawned process termination. After reprogramming the shell I decided to improove it a little bit. And from there I continuously extended shelly to make it a bit more professionel shell.

## General overview

In general shelly repeatedly does the following: It reads input from the command line, parses the input, executes commands and puts commands in the fore- or background of the terminal. Each iteration is divided into five stages:

1. input stage. shelly reads input via [linenoise](https://github.com/antirez/linenoise) and returns bytes to the main loop. In this stage some signal handling is also performed.
2. tokenizing stage. The read input is splitted into tokens which have a certain token type.
3. contexting stage. The list of previously created tokens is translated into execution contexts. Here the syntax of the input is checked.
4. execution stage. shelly launches all provided commands and handles input/output redirection as well as pipelining. Along the way executed commands are turned into jobs.
5. job control stage. The started jobs are finally delivered to the job control. shelly waits for jobs to finish independent wether they are in fore- or background.

In the following we are going to have a closer look at each stage.

## Input stage

On each iteration shelly prints a colorful cli string consisting of the username, the host name and the current working directory. From there it waits for input. In the tutorial I followed in the beginning the classic `getchar` function from the standard IO library was used to read input. The downside with this approach is the fact that `getchar` simply does not have built-in mechanisms to handle asynchronous events like signals, which interrupt the underlying `read` system call the function is using. So I first switched to using the raw `read` system call realizing soon that this solution was not suitable either. Since I wanted some features like reprinting the already typed in characters before handling them in my own code I decided to switch to already existing ways of handling input. Here my first choice was the `readline` function which I could not make friends with. With linenoise I finally found a library to handle signals and print as well as reprint strings in the terminal. Inside my own function `shelly_linenoise` linenoise library functions are called to read lines and return them without the trailing `\n` byte at the end. Additionally an `error` value is set for each call to the function. In case the user hits `CTRL + C` the error is set to indicate the occurence of the `SIGINT` signal so shelly discards the read input, prints a fresh cli line and calls `shelly_linenoise` again. In case the user hits `CTRL + D` (which writes `EOF` to stdin) and no input has been read the error is set to indicate the will to terminate shelly - shelly then prints exit and terminates. Finally if everything goes well `shelly_linenoise` just returns a `char *` ready to be processed in the next stage. To inform users about finished background jobs the function also handles occurences of the `SIGCHLD` signal and calls a job control api function. This is further described in the job control section.

## Tokenizing stage

The task of the tokenizer is to analyze the lexical structure of the input and turn a raw stream of characters into a list of tokens. Think of a token as a meaningful grouping of characters into a unit. This facilitates the job of the parser in the next stage so it does not have to deal with single characters but rather tokens of the language we want to parse. Let's have a look at an example. We want to tokenize the string `cmd param < input > output | cmd2 &`. This results in the following list of tokens:

```
WORD("cmd")
WORD("param")
REDIRECT_IN
WORD("input")
REDIRECT_OUT
WORD("output")
PIPE
WORD("cmd2")
AMPERSAND
```

The tokenizer strips all space like bytes. In my code I created two data types to represent a token. `token_type` is an enum with members representing the single tokens. A `token_type` variable is used inside of the `token_t` struct which represents an actual token.

```c
typedef enum {
    TOK_WORD_TYPE,
    TOK_REDIRECT_OUT_TYPE,
    TOK_REDIRECT_IN_TYPE,
    TOK_REDIRECT_PIPE_TYPE,
    TOK_REDIRECT_APPEND_TYPE,
    TOK_AMPS_TYPE,
    TOK_NULL_TYPE
} token_type;

typedef struct {
    token_type type;
    char *str;
    int index;
} token_t;
```

Only `WORD` tokens need the `str` field of the `token_t` struct. For a token of this kind the tokenizer copies input characters to the memory area `str` points to until it encounters a new token or a space like byte. For the other token types the characters which make up the token are not needed and are discarded. The pure existence of the token is enough information for the next stage.
The tokenizer is implemented over the function `token_t *tokenizer(char *line)` which receives the raw input line and returns a pointer to the list of `token_t` structs. The last token in the list has a `token_type` of NULL which marks the end of the list.

## Contexting stage or parsing stage

This stage has two main goals. First every input the user provides must be checked wether it makes up a valid shell command according to the features shelly supports. In other words: It must be checked wether it follows the syntax of the language shelly works with. Since I wasn't interrested in implementing features like subshells (`$(..)`) or higher level control structures like if statements or for/while loops, a regular language was fine for my needs. The following grammar in 'Extended Backus Naur Form' (EBNF) defines the language $L$:

```
word          = [a-zA-Z0-9]+

redirect_op   = "<" | ">" | ">>"
redirect      = redirect_op word

command_name  = word
argument      = word

command       = command_name (argument | redirect)*

pipeline_op   = "|" | "&"

pipeline      = command (pipeline_op command)* "&"?

input         = pipeline | ε
```

Elements of $L$ follow the classic shell syntax every POSIX compliant shell supports. For shelly a command is made up of at least one word and may include an arbitrary amount of parameters and redirection operators followed by a filename in any order. Commands may be connected via pipelines and may have a trailing ampersand. In case the input is not element of $L$, shelly informs the user, performs a cleanup and initiates a fresh iteration of shelly. The second goal of this stage is to turn the token list from the last stage into execution information the next stage operates on. It therefore interpretes the tokens and groups together all information each command is made of into so called execution contexts. One could say an execution context is the static semantics of a command. The implementation does not operate sequentially on the two goals. It rather simultaniously checks the syntax and updates the current execution context.

To parse $L$ the code mimics a deterministic finite automat via a state machine. The possible statuses of this machine are:

```c
typedef enum {
    STATUS_CONTEXT_INIT,
    STATUS_CONTEXT_WORD,
    STATUS_CONTEXT_REDIRECT_IN,
    STATUS_CONTEXT_REDIRECT_OUT,
    STATUS_CONTEXT_REDIRECT_APPEND,
    STATUS_CONTEXT_PIPE,
    STATUS_CONTEXT_AMPS
} context_status;
```

The before mentioned execution contexts are represented by the data type `execution_context_t`:

```c
typedef struct execution_context {
    context_type type;

    char **tokens;
    int argc;

    redirect_flags flags;
    char *input_file;
    char *output_file;
    char *append_file;

    int is_background;
    int pipeline_end;
} execution_context_t;
```

An instance of this struct holds all the information the executor of the next stage requires:

- `char **tokens`: This array of char pointers represents the command and its parameters. It is generated by simply copying the \\ `char *str` reference of the `WORD` tokens. At the last index of the array a `NULL` pointer is placed. This is done because the executors will make use of the `execve` function. This function expects a parameter of type `char **` with a terminating `NULL` pointer at its last position.
- `int argc`: This field holds the size of the previous array of char pointers.
- `redirect_flags flags`: This variable of type `redirect_flags` holds the information which redirection shall take place.

```c
typedef enum {
    REDIR_IN              = 1,
    REDIR_OUT             = 1 << 1,
    REDIR_APPEND          = 1 << 2,
    REDIR_INTO_PIPE       = 1 << 3,
    REDIR_OUT_OF_PIPE     = 1 << 4
} redirect_flags;
```

The value of `flags` is the result of bit-wise OR-ed `|`. The least significant five bytes of the value encode a redirection operation respectivley

executor needs to know which binary or shell builtin to run, which files to open for redirection operations, where to put the output of commands e.g. let the ran binaries print their output to the terminal or redirect it to the input stream of another program.
