TEAM:
Parker Patterson  patte591
Brandon Schenk	  schen259
Isaac Glover 	  glove080

Parker: Coded the client process forking and designed the message queue scheme for client and server. Also coded the threading and server logging including testing and bug fixes for all.
Isaac: Coded the client path traversal/partitioning and server code concurrency as well as troubleshooting bugs.
Brandon: Contributed to building server, bug fixes, logging code, and writing the README

To compile the program, run make in the p4 directory.
To run, first navigate to the server directory and type ./server <number of clients>.
Next, navigate to the client directory and type ./client <input folder name> <number of clients>

Purpose of Program:

This program uses a client program and a server program that communicate with each other using a messaging queue. Depending on how many clients are created, the server will create a thread for each client and will receive requests from each client using the messaging queue.

What exactly our program does:

The Client side of this program takes in an absolute path to a folder and a number of clients. After it traverses the given folder and gets the paths of the text files, it partitions the paths of the files into equal groups and puts the list of paths into text files and then puts them into a folder called ClientInput. Our code then generates client processes and each process sends each line in the client txt file to the server. Afterwards, the client process waits for an acknowledgment message from the server before continuing. The output of the client is a string representing the global word counts of the files.

The server side takes in the number of client processes, this is so the server can make a thread for each process. After reading a file sent from the client, the server sends an acknowledgment message. After it completes all of the requests, it receives the message "END" from the client which signals the server to stop. Before stopping the server waits for all threads to complete and then converts the global integer array to a char array and sends that back to the client before the thread exits.
