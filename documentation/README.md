# shelly - a definetely not POSIX compliant shell 
This shell is a fun project for learning some linux operating system principles - mostly about processes, process groups and signaling. 

### Motivation
When looking for something to program I stumbled accross the [blog entry](https://brennan.io/2015/01/16/write-a-shell-in-c/) of Stephen Brennan on how to write a shell in C. He presents a very basic shell which is capable of printing a small cli, executing typed commands and wait for the spawned process termination. After reprogramming the shell I decided to extend it and transf


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

