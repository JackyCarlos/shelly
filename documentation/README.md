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
In its first 






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

