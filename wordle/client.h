#ifndef WORDLE_CLIENT_H
#define WORDLE_CLIENT_H

#define DEFAULT_PORT 27993
#define DEFAULT_PORT_TLS 27994
#define FILE_NAME "project1-words.txt"
#define FIRST_GUESS "crane"
#define LEN 5

/**
 * Parse command line arguments.
 * @param argc number of arguments
 * @param argv array of arguments
 * @param port reference to the port number variable
 * @param secure reference to use TLS connection
 * @param hostname reference to the host name
 * @param username reference to the northeastern user name
 * @return true if success, false on error.
 */
bool parse_argv(int argc, char* argv[], int& port, bool& secure, std::string& hostname, std::string& username);

/**
 * Client attempts to establish a connection with the server.
 * @param hostname domain name of the server.
 * @param port_number the TCP port in decimal that the server is listening on.
 * @param tls a boolean value indicating whether to use TLS encrypted socket connection.
 * @return the socket descriptor if success, -1 on error.
 */
int connect_server(std::string& hostname, int port_number, bool tls);

/**
 * Read all words from the specified txt file to a vector for searching.
 * File name is defined as macro in this header file.
 * @return a vector of strings.
 */
std::vector<std::string> read_from_file();

/**
 * Start the game by sending a hello message and receiving a start message.
 * @param sockfd the socket descriptor used by I/O system calls.
 * @param username northeastern username.
 * @return the game id received from the server.
 */
std::string start_game(int sockfd, const std::string& username);

/**
 * Send a message to the server; exit the program if failed.
 * @param sockfd the socket descriptor used by I/O system calls.
 * @param message a reference to the string message.
 */
void send_message(int sockfd, const std::string& message);

/**
 * Receive a message from server. The message must ends with a '\n' character.
 * @param sockfd the socket descriptor used by I/O system calls.
 * @return the string message received by the client.
 */
std::string receive_message(int sockfd);

/**
 * Choose a word based on known constraints.
 * @param all_words a vector of strings containing all the word options
 * @param correct
 * @param nonexist
 * @param position
 * @return
 */
std::string choose_word(std::vector<std::string>& all_words, std::map<char, std::vector<int>>& correct,
                        std::set<char>& nonexist, std::map<char, std::vector<int>>& position);

void handle_marks(std::string& word, std::array<int, LEN>& marks, std::map<char, std::vector<int>>& correct,
                  std::set<char>& nonexist, std::map<char, std::vector<int>>& position);


#endif
