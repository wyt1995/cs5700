# FTP Client

This project implements a FTP client that supports the following six operations:
directory listing, making directories, file deletion, directory deletion, copying files to and from the FTP server, and moving files to and from the FTP server.

Usage: `./4700ftp [operation] [param1] [param2]`

For more help information, use `./4700 --help`; to read FTP server responses, add an optional `--verbose` flag to the command-line arguments. 

## Implementation
- The program starts by parsing command-line arguments and the URL for the FTP server. The info is stored in a `FTP` struct for later use.
- We proceed to establish a socket connection to the FTP server, sending USER, PASS, TYPE, MODE, and STRU commands for logging in.
- Once we are done with preparation, the program executes the command specified by the user. For `mkdir`, `rmdir`, and `rm` command, the program send one more message to the server. 
- For `ls`, `cp`, and `mv`, we need to send a `PASV` command, enter passive mode, and open a data channel for file uploading or downloading. Each command is handled by one or more functions.
- If all operations are successful, the program sends a `QUIT` command through the control channel, and then closes both control channel and data channel (if applies).

## Challenges
The program needs extensive error checking. If anything goes wrong, we need to close all files and connection made before exiting the program.
This leads to some errors that are hard to debug during my development process.

## Testing
- I built this project incrementally. The first version only establishes a simple socket connection with the FTP server without implementing any command, and then implement basic commands that don't need an extra data channel. 
The list command is used for basic data downloading, followed by copy and move commands. Along the way, I use the verbose mode to print server responses, which is helpful for debugging. 
- Once the required implementation is complete, I tested all commands under different environments. First, test `mkdir` or `rmdir` followed by `ls` to check if the program is able to create or delete folders under both the root directory and some subdirectory. 
Then, I tried to upload and download text files and binary files, using the file size information returned by `ls` to ensure the remote file is complete. These commands are also tested under both root directory and subdirectory, since the user can rename file in the command.
Finally, I tested `mv` command to make sure that it deletes the source file after the transfer is complete. 
