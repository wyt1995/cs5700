# Wordle Project

This project implements a client program that plays a variant of the Wordle game. The client and the server exchanges information in JSON objects. Once the client correctly guesses the word, the server will return a secret flag.

Note: I am a CS align student in the second year. I only know C previously but try to teach myself some C++ programming in this course. 
Please let me know if you notice any incorrect or unconventional syntax in my code.


## Implementation
- The program starts by parsing command-line arguments. A `parse_argv` function sets all relevant variable references, and then return true if successful. If any required argument is missing, the main function will exit with an error message.
- The hostname and username are then used to make a connection with the server. I used the `getaddrinfo` function to update a list of socket address structures, and attempts to call `socket` and `connect` while walking the list. The function will return a socket file descriptor is the connection is successful.
- If the user specified a "secure" argument, the client program will attempt to connect to the server using an encrypted TLS socket. This is handled by the OpenSSL library.
- After the connection is established, the client program will start playing the Wordle gaming by sending and receiving messages. The program maintains three data structures for making the next guess: a set of letters that do not appear in the secret word, a map from character to its correct positions, and a map from character to its incorrect positions. The `choose_word` function finds a word that satisfies all known information using linear search.
- Once a bye message is received, the `play_game` function will return the secret flag to `main`, which will terminate the program after closing the socket connection.

## Reference
1. Randal E. Bryant, David R. O'Hallaron, _Computer Systems: A Programmer's Perspective_, 3rd Edition. 
Pearson, 2016, pp. 932-948.
2. OpenSSL documentation, https://wiki.openssl.org/index.php/.
