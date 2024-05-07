# smallsh
Smallsh is a simple Unix shell program written in C. It provides basic shell functionalities such as executing commands, managing processes, and handling signals.


## Features

### Commands

Smallsh supports the following built-in commands:

- `cd [directory]`: Change the current working directory. If no directory is provided, it changes to the home directory.
- `exit [status]`: Exit the shell with an optional exit status.
- Other system commands: Smallsh can execute other system commands available in the PATH environment variable.

### I/O Redirection

Smallsh supports input and output redirection using the following symbols:

- `<`: Redirects standard input from a file.
- `>`: Redirects standard output to a file, overwriting existing contents.
- `>>`: Redirects standard output to a file, appending to existing contents.

### Background Processes

Commands can be run in the background by appending `&` at the end of the command line. This allows the shell to continue accepting new commands while the background process runs.

### Signal Handling

Smallsh handles the following signals:

- `SIGINT (Ctrl+C)`: Ignores the interrupt signal for background processes and terminates foreground processes.
- `SIGTSTP (Ctrl+Z)`: Ignores the suspend signal for the shell and background processes.

## Implementation Details

### Word Splitting

Smallsh splits input lines into words, recognizing whitespace as delimiters. It also supports comments indicated by `#` at the beginning of a word and backslash escapes.

### Parameter Expansion

The shell performs parameter expansion on words, replacing occurrences of `$`, `$!`, `$?`, and `${param}` with their respective values.

### Child Processes

When executing commands, Smallsh creates child processes using `fork()` and executes commands in them using `execvp()`. It supports foreground and background execution based on the presence of `&` at the end of the command line.

### Signal Handling

Smallsh sets up signal handlers for `SIGINT` and `SIGTSTP` to manage interrupts and suspensions gracefully.

## Examples

### Changing Directory
$ cd /path/to/directory 

### Redirecting Input and Output
$ command < input.txt > output.txt

### Redirecting Input and Output
$ long_running_task &

### Exiting the Shell
$ exit

## Notes

- Smallsh assumes a maximum word count of 512. You can adjust this by modifying the `MAX_WORDS` macro in the source code.
