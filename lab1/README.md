## Lab 1: System Programming Review
* This lab is to focus on the review of C system programming and the basis of the following projects.
* The goal of this lab is to get familar with concurrent programming and server/client apps.

### Part 1: Basic Shell
* Split arguments given by the command line.
* Fork a new process and execute with these splited arguments by `execvp()` system call.

### Part 2: Basic Shell with server and clients
* Extend part 1 with server and client processes running on the different windows but the same machine.
* Create a FIFO pipe that transfers command line arguments from clients.

### Part 3: Basic Shell with server and clients
* Based on part 2, use `dup2()` to direct the stdout to FIFO pipe.

### Part 4: Retransimission
* The server has 50% possibility to drop a request from clients.
* The clients will set a alarm and retransmit when timeout up to 3 times.
