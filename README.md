# smallsh
smallsh is a basic shell interface written in C. 

Commands cd, exit, and status are built-in, while other commands are handled by spawning a child process to execute execvp() and perform the desired action. 

### To compile 
Use the provided makefile and run:
make sh

### To run 
Use the provided makefile and run:
make runSh