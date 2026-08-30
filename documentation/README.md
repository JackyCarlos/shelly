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

This stage has two main goals. First every input the user provides must be checked wether it makes up a valid shell command according to the features shelly supports. In other words: It must be checked wether it follows the syntax of the language shelly works with. Since I wasn't interrested in implementing features like subshells (`$(..)`) or higher level control structures like if statements or for/while loops, a regular language was fine for my needs. The following grammar in 'Extended Backus Naur Form' (EBNF) defines the language $L$ of shelly:

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

Elements of $L$ follow the classic shell syntax every POSIX compliant shell supports. For shelly a command is made up of at least one word and may include an arbitrary amount of parameters and redirection operators followed by a filename in any order. Commands may be connected via pipelines and may have a trailing ampersand. In case the input is not element of $L$, shelly informs the user, performs a cleanup and initiates a fresh iteration of shelly. The second goal of this stage is to turn the token list from the last stage into execution information the next stage operates on. It therefore interpretes the tokens and groups together all information each command is made of into a so called execution context. One could say an execution context is the static semantics of a command. The implementation does not operate sequentially on the two goals. It rather simultaniously checks the syntax and updates the current execution context.

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

The before mentioned execution contexts are represented by the `execution_context_t` data type:

```c
typedef struct execution_context {
    context_type type;

    char **tokens;
    int argc;

    int flags;
    char *input_file;
    char *output_file;
    char *append_file;

    int is_background;
    int pipeline_end;
} execution_context_t;
```

An instance of this struct holds all the information the executor of the next stage requires:

- `char **tokens`: An array of char pointers representing the command and its parameters. It is generated by simply copying the `char *str` reference of the `WORD` tokens. At the last index of the array a `NULL` pointer is placed. This is done because the executor will make use of the `execve` function. This function expects a parameter of type `char **` with a terminating `NULL` pointer at its last position.
- `int argc`: An integer which holds the size of the previous array of char pointers.
- `int flags`: A bitmask describing which redirection operations should be performed. The value is formed by bitwise OR-ing one or more values from `redirect_flags`. Each set bit corresponds to a specific redirection operation.

```c
typedef enum {
    REDIR_IN              = 1,
    REDIR_OUT             = 1 << 1,
    REDIR_APPEND          = 1 << 2,
    REDIR_INTO_PIPE       = 1 << 3,
    REDIR_OUT_OF_PIPE     = 1 << 4
} redirect_flags;
```

- `char *input_file, *output_file, *append_file`: Each of these char pointers reference the target filename of an intended redirection operation. The default value for each of these is `NULL`.
- `int is_background`: This flag indicates wether the command shall be run in the fore- or background.
- `int pipeline_end`: This flag indicates wether the command is the final one in the current chain of commands.

This parser or contexter is available under the function `execution_context_t *get_context(token_t *)`. It turns a list of tokens into a list of execution contexts. In case there is a syntax error the function returns `NULL`. With this stage done shelly is ready to run the commands.

## Execution stage

This stages' task is to turn execution contexts into running processes and hand over information of the started processes to the job control. It is important to point out that the stage for starting processes is separated from the tracking of these processes. This separation of concerns is meaningful. At the time the execution stage starts a process it is not able to predict anything about the lifetime of the process. A started process may exit immediately after doing its thing. It may run in the foreground for some time and the user may decide to put it in the background or suspend it. The execution stage within the shelly main loop should not be responsible for keeping track of the status of the started processes and simply deal with launching them according to the information of the execution context list. This way the data of the control flow of the main loop can stay clean. The data of the previous stages e.g. the read line, the token list and the list of execution contexts are only of interest for the current iteration of the main loop. They should therefore be cleaned up after completing an iteration. This way the main loop can start a fresh clean iteration not bothering of what happened to the started processes and their corresponding data. Keeping the execution context list as a reference to running processes in following iterations of the main loop would introduce a complicated cleanup and tracking logic. This stems from the fact that the list of contexts simply is not an appropriate data structure to manage possible togetherness of processes. Imagine running `cmd1 | cmd2 & cmd3` with `cmd1` and `cmd3` exiting immediatly leaving only `cmd2` running in the background. Now it would not be possible to clean up the context list since `cmd2` is still running although `cmd3` is finished and has nothing to do with `cmd1` and `cmd2`. One would be forced to keep the context list over the course of possibly multiple upcomming iterations of the shelly main loop. Hence it is smarter to group `cmd1 | cmd2` and `cmd3` into separate jobs, start the commands and hand the jobs to the job control. After starting the processes the execution stage can simply forget about the processes it justed started. The starting of processes is done via the `executor` function which makes use of API functions of the job control. Although the job control is a separate stage we first need to take a look at some job control data structures.

### `job_t` and `job_command` data structures

```c
typedef struct job {
    int id;
    int pgid;
    int is_background;
    job_status status;

    int job_cmd_counter;
    job_command_t *job_commands;
    int job_cmds_size;
} job_t;
```

This struct holds the following information:

- `int id`: The id of the job inside the job list.
- `int pgid`: The id of the process group of the job (to be discussed later in this chapter).
- `int is_background`: Flag indicating wether the job is run in the fore- or background of the terminal.
- `job_status status`: Enum value representing the status of the job. Possible values include `RUNNING`, `SUSPENDED` and `TERMINATED`.

Each job is made up of one or more commands. These commands are called job-commands and reside inside the `job_command_t *job_commands` list. The remaining two variables hold information about the size and the capacity of the list. The `job_command_t` structs is very similar to the execution context struct. It simply adds variables to hold information about the execution information of the command:

```c
typedef struct job_command {
    char **tokens;
    int argc;

    redirect_flags flags;
    char *input_file;
    char *output_file;
    char *append_file;

    int pid;
    job_cmd_status job_stat;
    int return_value;
} job_command_t;
```

- `int pid`: Holds the pid of the process.
- `job_cmd_status`: Enum value representing the status of a process. Possible values include `CMD_FAILURE`, `CMD_SUSPENDED`, `CMD_TERMINATED` and `CMD_FAILURE`.
- `int return_value`: The exit value of the process in case it terminates.

Now we can understand the `executor`.

### `executor` functionality

The `executor` function iterates over the list of `execution_context_t` structs it receives as an argument. It first calls the job control function `job_control_get_job` which looks for a job slot inside the job list. In case the context represents the beginning of a set of commands (e.g. a pipeline command) a reference to a new job is returned. If the command on the other hand belongs to an already existing job because it is part of a pipeline the already existing job is returned. Recall the `pipeline_end` field of the `execution_context_t` struct which provides the information for this decision. Next the `job_control_add_job_command` function is called. This function adds the data of the `execution_context_t` representing a command to the job. The `context->tokens` as well as the references to files for redirecting purposes are hard-copied from the `execution_contest` to a new entry inside the list of `job_command`'s. The `job_stat` value of the job command is set to `CMD_RUNNING` except the command is a shell builtin in which case the value is set to `CMD_TERMINATED`. So far only passing information to job control data structures took place. What follows are the functions which will actually launch the command(s). For each entry in the list of execution contexts `launch_command` or `launch_builtin` is called.

#### `launch_command`

The `launch_command` function takes the current context, a reference to a process group id and a reference to a flag `prev_pipe_read` which purpose is going to be discussed soon. The function `launch_builtin` which is obviously responsible for launching shelly builtins acts kind of similar to `launch_command`. The focus for this part though is on the `launch_command` function.

```c
static int launch_command(execution_context_t *context, pid_t *job_pgid, int *prev_pipe_read) {
    pid_t pid;
    int pipe_fds[2];

    create_pipe(context, pipe_fds);
    pid = fork();

    if (pid == 0) {
        if(manipulate_fds(context, pipe_fds, prev_pipe_read) < 0) {
            exit(1);
        }

        child_set_pgid(job_pgid);

        execvp(*context->tokens, context->tokens);

        fprintf(stderr, "exec error\n");
        exit(127);
    } else if (pid < 0) {
        fprintf(stderr, "fork error\n");
    } else {
        parent_set_pgid(pid, job_pgid);

        pipe_cleanup_parent(context, pipe_fds, prev_pipe_read);
    }

    return pid;
}
```

Soon after being called, this function performs a `fork()` system call, creating a child process from the calling process. After the call to fork, execution continues in both the parent and the newly created child process. The return value of fork() is used to distinguish between the two execution paths: a return value of 0 indicates that the code is running in the child, while a positive return value contains the PID of the child and therefore indicates the parent process. The code in the two sections makes sure the user can later interact with the started process(es) via process groups and pipelining as well as redirecting data is handled properly. Inside the child process the call to `execvp(..)` replaces the child process image with the program specified by `context->tokens[0]`. The first argument to `execvp()` specifies the executable to run, while the second argument, `context->tokens`, is the `NULL`-terminated argument vector that is passed to the new program. Apart from the call to execvp(), there is still quite a bit happening in both the parent and the child process. To understand these parts in more detail, we will now take a closer look at process groups, input/output redirections, and pipelines.

##### Linux process groups

When running commands in a shell the user can interact with the started process(es). One can try to terminate the process(es) by hitting `CTRL + C` or suspend it by hitting `CTRL + Z` or just wait until it is finished. To accomplish this for shelly too, it makes use of the concept of process groups. Each process started in Linux has a process group id inherited of its parent. One can then group launched processes together by choosing one leading process whose pid becomes the process group id of all processes which are part of the process group. Additionally signals in Linux can not only be delivered to single processes but rather be delivered to whole process groups. In case one sents a signal to a process group the signal gets delivered to all processes of this process group. In the job control stage we will take advantage of this and some builtin features of the terminal shelly runs in. Our goal for now is to group processes together. All comands of a pipeline shall be put in a process group. Single commands get their own private process group. Let's analyze the code.

Before the first call to `launch_command` the executor creates the variable `job_pgid` with the initial value of `0`. This variables' purpose is to hold the process group id of the current process group we are working with. `launch_command` receives this variable as a reference, so the value does not lose its value during multiple calls to `launch_command` from the executor. After `fork`-ing inside `launch_command` the child calls the function `child_set_pgid` providing the `job_pgid` variable as an argument. In case the value of the argument is `0` the current process is meant to be the leader of a new process group (since there is no current process group) and calls `setpgid(0, 0)`. This function creates a new process group with the calling process as the leader of that group. This means that the process will join a new process group where its process id will also be its process group id (pgid). In case the value of `job_pgid` is unequal to `0`, `setpgid(0, *job_pgid)` gets called which sets the pgid of the calling process to the already existing pgid inside of `job_pgid`. This happens in case a command is not the first command of a pipelined command. After `fork`-ing inside `launch_command` the parent receives the pid of the created process and updates the reference to the `job_pgid` variable via the `parent_set_pgid` function.
The main loop of the executor guarantees that the value of the `job_pgid` variable is set back to zero as soon as the final command of a set of commands is encountered. For a following execution context the `launch_command` function will then create a new fresh process group for the calling process. The resetting of `job_pgid` can be seen in the following part of the while loop of the executor:

```c
...
if (context_list->pipeline_end) {
    ...
    job_pgid = 0;
}
...
```

The following example illustrates how the grouping of processes is performed:

```
cmd1 & cmd2 | cmd3 & cmd4
```

So in this case `cmd1` gets launched first and becomes the leader of a process group which only incorporates its own process. Since this command is a non-pipeline command the value of `job_pgid` inside the executor gets set back to zero. With the launch of `cmd2` again a new process group is created which is made up of `cmd2`'s own process. But now the value of `job_pgid` is not set back to zero because `cmd3` must also be member of `cmd2`'s process group. After the launch of `cmd3` and assigning the process to the process group of `cmd2` the value `job_pgid` again gets set back to zero so that `cmd4` is going to have its own process group again. So now every process has his or her process group id, the loop inside of the executor keeps track of the current pgid so everyone should be happy right? With setting the pgid for itself inside the `fork`-ed child we unfortunately can run into a race condition. Imagine process A arises from the call to `fork`. By mischance the os doesn't schedule A for some time after it got spawned meaning that A's call to `child_set_pgid` is postponed. Hence a process group with the id of A's pid is not created. Meanwhile the parent of A updates the `job_pgid` value, exits the function and initiates a new call to `launch_command` from which the forked process B arises. B is supposed to be in the process group of A. Now B gets scheduled and makes the call to `child_set_pgid` with the variable `job_pgid` which holds the assumed existing process group id. But since A's call to `child_set_pgid` did not happen yet there is no process group with the value of `job_pgid`. Consequently B's call to `set_pgid` trying to set its pgid to the non existing pgid of A will fail. In order to prevent this race condition one is forced to set the pgid of the child inside the parent too. This guarantees the existence of A's process group for upcomming proccesses trying to join A's process group.

```c
    ...
    if (pid == 0) {
        ...
        child_set_pgid(job_pgid);
        ...
    ...
    } else {
        // set the pgid variable as parent too in order to prevent race conditions
        parent_set_pgid(pid, job_pgid);
        ...
    }
    ...
```

With this race condition handled we are done with grouping processes in the execution stage. We are going to use these grouped processes later on at the job control stage. Let's now have a look at the handling of input/output redirections and pipelining.

##### |-ing and redirections
