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
5. job control stage. The started jobs are finally delivered to the job control. shelly waits for jobs to finish independent whether they are in fore- or background.

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

This stage has two main goals. First every input the user provides must be checked whether it makes up a valid shell command according to the features shelly supports. In other words: It must be checked whether it follows the syntax of the language shelly works with. Since I wasn't interrested in implementing features like subshells (`$(..)`) or higher level control structures like if statements or for/while loops, a regular language was fine for my needs. The following grammar in 'Extended Backus Naur Form' (EBNF) defines the language $L$ of shelly:

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
- `int is_background`: This flag indicates whether the command shall be run in the fore- or background.
- `int pipeline_end`: This flag indicates whether the command is the final one in the current chain of commands.

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
- `int is_background`: Flag indicating whether the job is run in the fore- or background of the terminal.
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

#### Linux process groups

When running commands in a shell the user can interact with the started process(es). One can try to terminate the process(es) by hitting `CTRL + C` or suspend it by hitting `CTRL + Z` or just wait until it is finished. To accomplish this for shelly too, it makes use of the concept of process groups. Each process started in Linux has a process group id inherited of its parent. One can then group launched processes together by choosing one leading process whose pid becomes the process group id of all processes which are part of the process group. Additionally signals in Linux can not only be delivered to single processes but rather be delivered to whole process groups. In case one sents a signal to a process group the signal gets delivered to all processes of this process group. In the job control stage we will take advantage of this and some builtin features of the terminal shelly runs in. Then we will discuss what it really means for a process to be in the foreground. Our goal for now is to group processes together. All comands of a pipeline shall be put in a process group. Single commands get their own private process group. Let's analyze the code.

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

So in this case `cmd1` gets launched first and becomes the leader of a process group which only incorporates its own process. Since this command is a non-pipeline command the value of `job_pgid` inside the executor gets set back to zero. With the launch of `cmd2` again a new process group is created which is made up of `cmd2`'s own process. But now the value of `job_pgid` is not set back to zero because `cmd3` must also be member of `cmd2`'s process group. After the launch of `cmd3` and assigning the process to the process group of `cmd2` the value `job_pgid` again gets set back to zero so that `cmd4` is going to have its own process group again. So now every process has his or her process group id, the loop inside of the executor keeps track of the current pgid so everyone should be happy right? With setting the pgid for itself inside the `fork`-ed child we unfortunately can run into a race condition.

Imagine process A arises from the call to `fork`. By mischance the os doesn't schedule A for some time after it got spawned meaning that A's call to `child_set_pgid` is postponed. Hence a process group with the id of A's pid is not created. Meanwhile the parent of A updates the `job_pgid` value, exits the function and initiates a new call to `launch_command` from which the forked process B arises. B is supposed to be in the process group of A. Now B gets scheduled and makes the call to `child_set_pgid` with the variable `job_pgid` which holds the assumed existing process group id. But since A's call to `child_set_pgid` did not happen yet there is no process group with the value of `job_pgid`. Consequently B's call to `set_pgid` trying to set its pgid to the non existing pgid of A will fail. In order to prevent this race condition one is forced to set the pgid of the child inside the parent too. This guarantees the existence of A's process group for upcomming proccesses trying to join A's process group.

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

With this race condition handled we are done with grouping processes in the execution stage. We are going to use these grouped processes later on at the job control stage. For now they are just running processes grouped together. After a successfull call to `launch_command` the executor updates the `pgid` value in the job struct the launched process belongs to. Let's now have a look at the handling of input/output redirections and pipelining.

#### |-ing and redirections

Input/ouput redirections and pipelining are closely related to each other. They both boil down to manipulating the file descriptors a command uses for standard input and standard output. Normally a command reads input from file descriptor `0` (`STDIN`) and writes its output to file descriptor `1` (`STDOUT`). By changing what these file descriptors refer to, the executed program itself does not need to know whether its input comes from the terminal, a file or the output of another process. The same applies to its output. From the perspective of the executed program it simply continues reading from STDIN and writing to STDOUT.

For the `launch_command` the information whether and what kind of redirections are intended are stored inside of the `flags` value of an execution context. After entering the function and before `fork`-ing one must decide whether we need a pipeline for the current command to write into. This check is performed via the `create_pipe` function:

```c
static int create_pipe(execution_context_t *context, int *pipe_fds) {
    if ((context->flags & REDIR_INTO_PIPE) == REDIR_INTO_PIPE && pipe(pipe_fds) == -1) {
        return -1;
    }

    return 0;
}
```

In case the `REDIR_INTO_PIPE` flag is set (meaning the current command wants to redirect its output tino another command), a fresh pipe is ordered from the system. `launch_command` can then access this pipe by referencing the read and write end of the pipe. These two file descriptors are saved to the integer array reference `int *pipe_fds` the `launch_command` function provided as arguments for `create_pipe`. After this `launch_command` performs the known `fork` leaving us again with two execution paths to have a look at.

The child calls the function `manipulate_fds`. It receives the current execution context, the file descriptors of a possibly newly created pipe and a reference to `prev_pipe_read`. `manipulate_fds` uses these flags to decide which file descriptors need to be changed. Let's first have a look at the redirection to files:

```c
static int manipulate_fds(execution_context_t *context, int *pipe_fds, int *prev_pipe_read) {
...
    if ((context->flags & REDIR_IN ) == REDIR_IN) {
        fd = open(context->input_file, O_RDONLY);

        if (fd == -1) {
            handle_error(errno, context->input_file);
            return -1;
        }
        dup2(fd, 0);
    }
...
}
```

In case the `REDIR_IN` flag is set, the file referenced by `context->input_file` is opened for reading. The resulting file descriptor is then passed to `dup2(fd, 0)`. The `dup2` call makes file descriptor 0 refer to the same open file as fd. Since file descriptor 0 represents the standard input of a process, reads from `STDIN` performed by the program executed later on will now read from the specified file instead of the terminal. Redirecting standard output works in the same fashion:

```c
static int manipulate_fds(execution_context_t *context, int *pipe_fds, int *prev_pipe_read) {
...
    if ((context->flags & REDIR_OUT ) == REDIR_OUT) {
            fd = open(context->output_file, (O_WRONLY | O_CREAT | O_TRUNC), mode);

            if (fd == -1) {
                handle_error(errno, context->output_file);
                return -1;
            }
            dup2(fd, 1);
        }
    ...
}
```

In case the `REDIR_OUT` flag is present, the specified output file is opened using `O_WRONLY | O_CREAT | O_TRUNC`. Hence the file is opened for writing, created if it does not exist yet and truncated in case it already exists. The call to `dup2(fd, 1)` then makes the standard output of the process refer to this file. Consequently everything the executed program writes to `STDOUT` will end up inside the specified file. The `REDIR_APPEND` case works almost identically. The main difference is the use of `O_APPEND` instead of `O_TRUNC`. Existing content is therefore preserved and new output is appended to the end of the file. In both output cases newly created files use the mode `0644`.

Note that this file descriptor manipulations happens before the child calls `execvp`. When `execvp` replaces the child process image, the configured file descriptors remain available to the newly executed program. Let's now get to the pipelining stuff.

As already mentioned a Linux pipe consists of two file descriptors. One side is used for reading and the other side is used for writing. Now consider the following pipeline:

```
cmd1 | cmd2
```

Now the the standard output of `cmd1` must be connected to the write end of the pipe. This happens inside manipulate_fds when the `REDIR_INTO_PIPE` flag is set:

```c
static int manipulate_fds(execution_context_t *context, int *pipe_fds, int *prev_pipe_read) {
...
    if ((context->flags & REDIR_INTO_PIPE) == REDIR_INTO_PIPE) {
            ...
            close(pipe_fds[0]);
            dup2(pipe_fds[1], 1);
        }
...
}
```

After the call to `dup2(pipe_fds[1], 1)`, `STDOUT` refers to the write end of the pipe. Everything `cmd1` writes to its standard output can consequently be read from the other end of the pipe. The read end of the pipe is no longer needed and can be closed via `close(pipe_fds[0])`. The other side of this connection is established for commands which receive their input from a previous command in the pipeline. Such commands have the `REDIR_OUT_OF_PIPE` flag set. In this case `manipulate_fds` performs the following operation:

```c
static int manipulate_fds(execution_context_t *context, int *pipe_fds, int *prev_pipe_read) {
...
    if ((context->flags & REDIR_OUT_OF_PIPE) == REDIR_OUT_OF_PIPE) {
        dup2(*prev_pipe_read, 0);
    }
...
}
```

The variable `prev_pipe_read` contains the read end of the pipe belonging to the previous command. By calling `dup2(*prev_pipe_read, 0)`, `STDIN` is made to refer to this read end. From the perspective of the child process the pipeline is now completely transparent. The first command simply writes to its `STDOUT`, while the following command simply reads from its `STDIN`. The fact that these two file descriptors are connected through a pipe is of no concern to the programs executed later on.

So far we have seen how `manipulate_fds` connects the standard input and output of child processes to files and corresponding pipe ends. What happens in the parent code after `fork`-ing? The parents code is responsible for solving a remaining problem. When launching the first command of a pipeline, the following command does not exist yet. The read end of the pipe therefore has to survive the return from `launch_command` so that it can later be used as the standard input of the following command. This is the purpose of the `prev_pipe_read` variable. The variable is initialized with a value of 0 inside the executor and passed as a reference through the individual calls to `launch_command`. After a child process has been launched, the parent calls `pipe_cleanup_parent`:

```c
static void pipe_cleanup_parent(execution_context_t *context, int *pipe_fds, int *prev_pipe_read) {
    ...
    if ((context->flags & REDIR_INTO_PIPE) == REDIR_INTO_PIPE) {
        close(pipe_fds[1]);
        *prev_pipe_read = pipe_fds[0];
    }
}
```

If the current command writes into a pipe, the parent itself does not need the write end of that pipe anymore and therefore closes `pipe_fds[1]`. The read end however is still needed for the next command of the pipeline. Its file descriptor is therefore stored inside `prev_pipe_read`. Since `prev_pipe_read` is passed as a reference and lives inside the executor, its value survives the return from the current call to `launch_command`. When the executor proceeds to the next execution context, the stored read end is passed to the next call to `launch_command` and consequently to `manipulate_fds`. A command which receives its input from the previous command can then read from the file via `dup2(*prev_pipe_read, 0)` as already discussed. After the read end stored in `prev_pipe_read` has been used for the next command, the parent does not need to keep this file descriptor open anymore. Therefore, on the next call to `pipe_cleanup_parent`, the previous read end is closed:

```c
    // close read fds to previous pipe
    if (*prev_pipe_read) {
        close(*prev_pipe_read);
    }
```

In case the current command itself redirects its output into another pipe, the same procedure starts again. The parent closes the write end of the newly created pipe and stores its read end inside `prev_pipe_read`.

With this we have reached the end of the execution stage. The list of execution contexts has been processed and the corresponding commands have been launched. During this process the necessary input/output redirections and pipelines have been set up. The launched processes have on top been assigned to their corresponding process groups. At the same time the information contained in the execution contexts has been transferred into jobs and job commands so that the execution context list itself is no longer needed for keeping track of the launched processes. After processing the final execution context, the `executor` returns a reference to the last job it worked on and shelly finally free's the context and token list. From now on the job control is responsible for running processes.

# job control stage

After launching processes shelly needs a mechanism to keep track of their further execution and allow the user to interact with them. This is the responsibility of the job control. Once a process has been launched, its further execution is mostly independent of the control flow of shelly itself. A process may terminate on its own, it may be stopped or terminated through a signal or it may continue running while shelly itself proceeds with other tasks. The job control therefore has to keep track of changes to the state of these processes and update the corresponding jobs and job commands. Signals form an important mechanism for controlling and observing the launched processes. They allow processes to be interrupted, stopped or continued and also provide shelly with a way of being informed about changes to its child processes. Another important responsibility of the job control is collecting child processes after they have terminated or changed their state. Starting a process alone is not enough. Shelly eventually has to obtain information about what happened to the process and store this information inside the corresponding job structures. Jobs which are no longer needed can then be cleaned up and their resources released. With these general responsibilities in mind, we can now take a closer look at what happens after the executor has finished launching the processes of the current iteration.

### `job_control_after_launch`

The last stage finished with the executor returning the last `job_t` data structure it operated on. In case the command(s) of this job miss the `&` sign shelly cannot directly continue with a new iteration of the main loop. The missing ampersand means the job is supposed to run in the foreground and the user is supposed to be able to interact with it. To accomplish this shelly runs the function `job_control_after_launch` with the job as its argument which then calls the function `tcsetpgrp(0, job->pgid)`. This command hands the terminal foreground over to the process group of the job. Before shelly itself was in control of the terminal. The first argument 0 to `tcsetpgrp` refers to shelly's controlling terminal while the second argument specifies the process group which shall become the foreground process group of this terminal. By passing `job->pgid`, the complete process group of the job becomes the terminal's foreground process group. Now the user can begin interacting with the running job. Hitting `CTRL + C` for example causes the terminal to generate `SIGINT`, while pressing `CTRL + Z` causes it to generate `SIGTSTP`. These signals are not simply sent to one arbitrary process. The terminal delivers them to the processes belonging to its current foreground process group. With the help of process groups the terminal can treat all processes of a pipeline or single commands as one unit. Execution now continues with a call to the `foreground_job_wait` function discussed in the next section.

In case the last job has a trailing ampersand there isn't a change of the foreground process groups. The last started job's process group remains in the background and the `job_control_after_launch` function returns immediatelly. In this case shelly can directly start a new iteration of the main loop.

### `foreground_job_wait`

```c
void foreground_job_wait(job_t *job) {
    int child_pid;
    int child_status;
    job_command_t *job_command;

    child_status = 0;

    while (job_running(job)) {
        child_pid = waitpid(-job->pgid, &child_status, WUNTRACED);

        for (int j = 0; j < job->job_cmd_counter; ++j) {
            ...
        }
    }

...
}
```

The main purpose of this function is to wait for state changes of the processes belonging to the foreground job and update the corresponding job_command structures. The function keeps doing this as long as job_running(job) reports that at least one command of the job is still running. The actual waiting is performed by the call to

```c
child_pid = waitpid(-job->pgid, &child_status, WUNTRACED);
```

Usually `waitpid` can be used to wait for a specific child process by providing its PID as the first argument. In our case however we are not interested in one particular process. Recall that a job may consist of multiple processes in case it represents a pipeline. The negative value `-job->pgid` tells `waitpid` to wait for any child process whose process group id equals `job->pgid`. Once again the process groups created during the execution stage allow us to treat all processes belonging to one job as a unit. The second argument is a reference to `child_status`. Once `waitpid` returns because the state of one of the processes changed, information about what happened to this process is stored inside this variable. The return value itself contains the PID of the child whose state changed.

Normally waitpid waits for terminated child processes. For job control this alone is not enough. A process running in the foreground does not necessarily have to terminate before shelly shall regain control. Consider a user hitting `CTRL + Z` on a running job. In this case the terminal sends `SIGTSTP` to its foreground process group. With this the process is consequently stopped but it still exists. Shelly nevertheless has to notice this state change because it cannot continue waiting for the process to terminate while the process itself is suspended. This is the reason for passing `WUNTRACED` to `waitpid`. With this option `waitpid` also returns when one of the waited-for child processes gets stopped. Shelly can therefore react not only to terminated processes but also to processes which have been suspended. After `waitpid` returns, `child_pid` tells us which process caused the return. The corresponding `job_command` still has to be found because the status information maintained by the job control is stored per command. The function therefore iterates over the commands belonging to the job and compares their stored PIDs with the PID returned by `waitpid`:

```c
...
while (job_running(job)) {
    child_pid = waitpid(-job->pgid, &child_status, WUNTRACED);

    for (int j = 0; j < job->job_cmd_counter; ++j) {
        job_command = &job->job_commands[j];

        if (job_command->pid == child_pid) {
            update_job_command_status(child_status, job_command);
        }
    }

    ...
}
```

Once the matching command is found, the status returned by `waitpid` is used to update the corresponding `job_command`. If the process was stopped, for example as a result of receiving `SIGTSTP`, the command is marked as `CMD_SUSPENDED`. If the process has terminated, its exit status is stored inside the `job_command` and its state is changed accordingly. A return value of 127, which is used by shelly when the execution of a command fails, results in `CMD_FAILURE`, while any other terminated process is marked as `CMD_TERMINATED`. With this, the information obtained from the operating system through waitpid has been transferred into shelly's own representation of the process state. The job control can now use the states of the individual `job_commands` to determine the state of the job as a whole. After updating the command the condition of the surrounding loop `job_running(job)` is checked again. The predicate function `job_running` iterates over all commands of the job and returns 1 as long as at least one of them still has the state `CMD_RUNNING`. This is especially important for pipelines. The termination or suspension of one process alone does not necessarily mean that shelly is done waiting for the complete job. `job_running` returning 0 means the foreground job has reached a state in which shelly can continue handling it. This does not necessarily mean that the complete job has terminated. All commands may have terminated, but they may also have been suspended. This distinction is handled back inside `job_control_after_launch` after leaving `foreground_job_wait`.

Back inside `job_control_after_launch`, shelly therefore first checks whether the complete job has terminated:

```c
if (job_complete(job)) {
    return_value = job->job_commands[job->job_cmd_counter - 1].return_value;
    cleanup_job(job);
}
```

If all `job_commands` of a job have reached a terminated state, the job is finished. The return value of the last command is stored as the return value of the job and the job itself can be cleaned up since shelly no longer has to keep track of it. If the job is not complete, the processes were suspended instead. In this case the job must not be cleaned up since it still represents existing processes which may later be continued. The job is therefore marked as `SUSPENDED` and kept inside the job list:

```c
if (!job_running(job)) {
    job->status = SUSPENDED;
    job->is_background = 1;

    job_display_print_ctrlz(job);
}
```

At this point shelly is done handling the foreground execution of the job. Regardless of whether the job terminated or was suspended, the process group of the job must no longer remain the foreground process group of the terminal. Remember that before entering `foreground_job_wait`, shelly handed the terminal foreground to the job using `tcsetpgrp`. Shelly now takes it back by making its own process group the foreground process group again via `tcsetpgrp(0, global_shell_pgid)`. `global_shell_pgid` is shelly's process group id set at the startup of shelly.

This completes the handling of a foreground job via `job_control_after_launch`. If the job terminated, it has already been cleaned up. If it was suspended, its state remains stored inside the job list and it can be interacted with again later. The terminal is controlled by shelly's process group again.

With the handling of a foreground job covered, there is still another case left to consider. Shelly does not always wait for a launched job before continuing its main loop. Processes may continue running while shelly is either waiting for further user input or waiting for another job currently running in the foreground. There needs to be a way to notice and collect state changes of these processes without actively waiting for them.
