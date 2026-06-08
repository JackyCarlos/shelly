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
On each iteration shelly prints a colorful cli string consisting of the username, the host name and the current working directory. From there it waits for input. In the tutorial I followed in the beginning the classic `getchar` function from the standard IO library was used to read input. The downside with this approach is the fact that `getchar` simply does not have built-in mechanisms to handle asynchronous events like signals, which interrupt the underlying `read` system call the function is using. So I first switched to using the raw `read` system call realizing soon that this solution was not suitable either. Since I wanted some features like reprinting the already typed in characters before handling them in my own code I decided to switch to already existing ways of handling input. Here my first choice was the `readline` function which I could not make friends with. With linenoise I finally found a library to handle signals and print as well as reprint strings in the terminal. Inside my own function `shelly_linenoise` linenoise library functions are called to read lines and return them without the trailing `\n` byte at the end. Additionally an `error` value is set for each call to the function. In case the user hits `CTRL + C` the error is set to indicate the occurence of the `SIGINT` signal so shelly discards the read input, prints a fresh cli line and calls `shelly_linenoise` again. In case the user hits `CTRL + D` (which writes `EOF` to stdin) and no input has been read the error is set to indicate the will to terminate shelly - shelly then prints exit and terminates. Finally if everything goes well `shelly_linenoise` just returns a `char *` ready to be processed in the next stage. To inform users about finished background jobs the function also handles occurences of the `SIGCHLD` signal and calls a job control api function. This is further discussed in the job control section.  


## Tokenizing stage 






regular language:

```
word          = [a-z0-9]+

redirect      = "<" word | ">"  word | ">>" word

redirects     = redirect*

command       = word+

pipeline_op   = "|" | "&"

pipeline      = command redirects (pipeline_op command redirects) "&"?

input         = pipeline | ε
```

